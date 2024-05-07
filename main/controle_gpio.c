/** @file controle_gpio.c - Controle dos atuadores e sensores.

    Camada de abstração que esconde os detalhes das ligações das portas
    do micro-controlador.
    
    Para o aplicativo, existem apenas atuadores, sensores, contadores...
    O código é específico para o ESP32, mas pode ser re-escrito para outros
    módulos, mantendo a mesma interface.
    
    Na abstração, o identificador de cada pino vai de 1 até o limite da classe,
    independente do pino GPIO a que está conectado.
    Assim, temos atuador 1, atuador 2, ...; sensor 1, sensor 2, ...
    
    Um pino especial oferece ao aplicativo um indicador de reconfiguração,
    para que o sistema possa ser retornado à configuração de fábrica.
    
    @author João Vianna (jvianna@gmail.com)
    @version 0.82 03/05/2024
    
    Código criado a partir do exemplo:
    .../esp-idf/examples/peripherals/gpio/generic_gpio/main/gpio_example_main.c  
    
    @todo Código dos contadores
*/

/*  Parte do comentário original

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include <esp_log.h>

#include "controle_gpio.h"

/*
 * Comentário no código original
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output (ESP32C2/ESP32H2 uses GPIO8 as the second output pin)
 * GPIO19: output (ESP32C2/ESP32H2 uses GPIO9 as the second output pin)
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Note. These are the default GPIO pins to be used in the example. You can
 * change IO pins in menuconfig.
 *
 * Test:
 * Connect GPIO18(8) with GPIO4
 * Connect GPIO19(9) with GPIO5
 * Generate pulses on GPIO18(8)/19(9), that triggers interrupt on GPIO4/5
 *
 */
 
 #define TAG "controle_gpio"

// Um pino é usado para forçar configuração de fábrica
#define GPIO_RECONFIG   GPIO_NUM_35

// Auxiliares para configuração do micro-controlador
#define MASCARA_ATUADORES  ((1ULL << GPIO_NUM_18) | (1ULL << GPIO_NUM_19) | (1ULL << GPIO_NUM_22) | (1ULL << GPIO_NUM_23))
#define MASCARA_CONTADORES ((1ULL << GPIO_NUM_4))
#define MASCARA_SENSORES   ((1ULL << GPIO_NUM_32) | (1ULL << GPIO_NUM_33))
#define MASCARA_READ_ONLY  ((1ULL << GPIO_RECONFIG))

// Duração do tick do temporizador local em ms
#define INTERVALO_TICK_MS     100

// Tabelas de periféricos

// Mapeia pinos GPIO de entrada para id dos sensores
static const int mapa_sensores[MAX_SENSORES + 1] = {
  0,
  GPIO_NUM_32,
  GPIO_NUM_33
};


typedef struct {
  int   gpio;                   //< Porta do ESP32 onde está ligado o contador 
  int   contagem;               //< Contagem do número de eventos disparados
  int   valor_anterior;          //< Valor do nível da entrada da última vez que foi lida.
} estado_contador_t;


// Mapeia pinos GPIO de entrada para id dos contadores
static estado_contador_t mapa_contadores[MAX_CONTADORES + 1] = {
  {0},
  { .gpio = GPIO_NUM_4,
    .contagem = 0,
    .valor_anterior = 1
  }
};


typedef struct {
  int   gpio;                   //< Porta do ESP32 onde está ligado o atuador 
  int   valor;                  //< Último valor atribuído ao atuador
  int   tempo_restante;         //< Tempo restante para ser desligado em TICKS
} estado_atuador_t;


// Mapeia pinos GPIO de saída para id dos atuadores
static estado_atuador_t mapa_atuadores[MAX_ATUADORES + 1] = {
  {0},
  { .gpio = GPIO_NUM_18},
  { .gpio = GPIO_NUM_19},
  { .gpio = GPIO_NUM_22},
  { .gpio = GPIO_NUM_23}
};


// Documentação das funções públicas no arquivo header.


/* Inicializa periféricos do ESP32
 */
void controle_gpio_iniciar(void) {

    // Configuração dos atuadores (portas de saída binária)
    gpio_config_t io_conf = {
      .pin_bit_mask = MASCARA_ATUADORES,
      .mode = GPIO_MODE_OUTPUT,
      .intr_type = GPIO_INTR_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .pull_up_en = GPIO_PULLUP_DISABLE
    };
  
    gpio_config(&io_conf);

    // Configuração dos contadores
    io_conf.pin_bit_mask = MASCARA_CONTADORES;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    // Contador implementado por software - os pinos serâo simplesmente pinos de entrada
    io_conf.intr_type = GPIO_INTR_DISABLE,
    gpio_config(&io_conf);
    
    // Configuração dos sensores (entradas binárias simpĺes, com pullup)
    io_conf.pin_bit_mask = MASCARA_SENSORES;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    
    // Sensores Read Only e Pino de reconfiguração
    // O circuito PULL_UP/PULL_DOWN deve ser implementado em hardware externo.
    io_conf.pin_bit_mask = MASCARA_READ_ONLY;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}


const char *controle_gpio_status(void) {
  return "Modulo de Controle\nVersao:1.0\nAtuadores:4\nSensores:2\nContadores:1\n\n";
}


int controle_gpio_ler_sensor(int id) {
  if((id > 0) && (id <= MAX_SENSORES)) {
    return gpio_get_level(mapa_sensores[id]);
  } else {
    // Condição de erro: id inválido
    return -1;
  }
}


int controle_gpio_ler_contador(int id) {
  if((id > 0) && (id <= MAX_CONTADORES)) {
    return mapa_contadores[id].contagem;
  } else {
    // Condição de erro: id inválido
    return -1;
  }
}


void controle_gpio_reiniciar_contador(int id) {
  if((id > 0) && (id <= MAX_CONTADORES)) {
    mapa_contadores[id].contagem = 0;
    mapa_contadores[id].valor_anterior = 1;
  }
}


void controle_gpio_mudar_atuador(int id, int valor) {
  if((id > 0) && (id <= MAX_ATUADORES)) {
    if ((valor >= 0) && (valor <= 1)) {
      mapa_atuadores[id].tempo_restante = 0;
      mapa_atuadores[id].valor = valor;
      gpio_set_level(mapa_atuadores[id].gpio, valor);
    }
  }
}


void controle_gpio_alternar_atuador(int id) {
  if((id > 0) && (id <= MAX_ATUADORES)) {
    int novo_valor = 1 - mapa_atuadores[id].valor; 

    mapa_atuadores[id].tempo_restante = 0;
    mapa_atuadores[id].valor = novo_valor;
    gpio_set_level(mapa_atuadores[id].gpio, novo_valor);
  }
}


void controle_gpio_pulsar_atuador(int id, int duracao) {
  int num_ticks;

  if((id > 0) && (id <= MAX_ATUADORES)) {
    // A duração é em milissegundos. Internamente, trabalhamos com ticks do timer.
    num_ticks = duracao / (int)INTERVALO_TICK_MS;
    
    if (num_ticks < 1) {
      num_ticks = 1;
    }
    mapa_atuadores[id].valor = 1;
    gpio_set_level(mapa_atuadores[id].gpio, 1);
    mapa_atuadores[id].tempo_restante = num_ticks;
  }
}


bool controle_gpio_reconfig(void) {
  // Um pino é usado para forçar modo Soft AP, com ssid de fábrica e senha vazia.
  if (gpio_get_level(GPIO_RECONFIG) == 0) {
    return false;
  } else {
    return true;
  }

}


/*  Código do timer, baseado no exemplo em Timer_test.c (https://gist.github.com/shirish47)
 */

static TimerHandle_t temporizador_local;

static int id_temporizador_local = 1;


void callback_temporizador(TimerHandle_t xTimer) {
  // Tarefas do temporizador
  // TODO: Utilizar semáforos para seções críticas
  int tempo;
  int valor;

  estado_atuador_t *p_atuador = NULL;
  estado_contador_t *p_contador = NULL;

  int id_perif = MAX_ATUADORES;

  // Trata atuadores que estão gerando um pulso
  while (id_perif > 0) {
    p_atuador = &mapa_atuadores[id_perif];
    tempo = p_atuador->tempo_restante;

    if (tempo > 0) {
      if (tempo <= 1) {
        // Tempo expirou para este atuador. Retornar ao estado desligado
        p_atuador->tempo_restante = 0;
        p_atuador->valor = 0;
        gpio_set_level(p_atuador->gpio, 0);
      } else {
        p_atuador->tempo_restante = tempo - 1;
      }
    }
    id_perif--;
  }
  
  id_perif = MAX_CONTADORES;

  /*  Trata contadores que mudaram de nível 1 para 0
      NOTA: Nesta implementação por software, embora se possam perder eventos
            com pulsos muito rápidos, fica resolvido o problema de 'debouncing'
            do sinal elétrico, pelo tempo grande entre medições (100ms).
   */
  while (id_perif > 0) {
    p_contador = &mapa_contadores[id_perif];

    valor = gpio_get_level(p_contador->gpio);
    
    if (p_contador->valor_anterior > 0 && valor == 0) {
      p_contador->contagem++;
    }
    p_contador->valor_anterior = valor;

    id_perif--;
  }
}


bool controle_gpio_ativar_timer(void) {
  // Dispara continuamente a cada intervalo
  ESP_LOGI(TAG, "Iniciando Timer GPIO com intervalo de %d ticks.",
                (int)pdMS_TO_TICKS(INTERVALO_TICK_MS));

  temporizador_local = xTimerCreate("GPIOTimer", pdMS_TO_TICKS(INTERVALO_TICK_MS), pdTRUE,
                                    (void *)id_temporizador_local, &callback_temporizador);

  if(temporizador_local == NULL) {
    return false;
  }
  // Aguarda 1 intervalo de tempo antes de iniciar o temporizador.
  if(xTimerStart(temporizador_local, pdMS_TO_TICKS(INTERVALO_TICK_MS) ) != pdPASS) {
    return false;
  }
  return true;
}
