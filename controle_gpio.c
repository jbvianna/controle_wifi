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
    
    Código criado a partir do exemplo:
    .../esp-idf/examples/peripherals/gpio/generic_gpio/main/gpio_example_main.c  
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
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "controle_gpio.h"

/*
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

// Um pino é usado para forçar configuração de fábrica
#define GPIO_RECONFIG   GPIO_NUM_35

// Auxiliares para configuração do micro-controlador
#define MASCARA_ATUADORES  ((1ULL << GPIO_NUM_18) | (1ULL << GPIO_NUM_19) | (1ULL << GPIO_NUM_22) | (1ULL << GPIO_NUM_23))
#define ALARME_1             GPIO_NUM_4
#define MASCARA_ALARMES    ((1ULL << ALARME_1))
#define MASCARA_SENSORES   ((1ULL << GPIO_NUM_32) | (1ULL << GPIO_NUM_33))
#define MASCARA_READ_ONLY    ((1ULL << GPIO_RECONFIG))


#define ESP_INTR_FLAG_DEFAULT 0

// Tabelas de periféricos

// Mapeia pinos GPIO de saída para id dos atuadores
static const int mapa_atuadores[MAX_ATUADORES + 1] = {
  0,
  GPIO_NUM_18,
  GPIO_NUM_19,
  GPIO_NUM_22,
  GPIO_NUM_23
};

// Mapeia pinos GPIO de entrada para id dos sensores
static const int mapa_sensores[MAX_SENSORES + 1] = {
  0,
  GPIO_NUM_32,
  GPIO_NUM_33
};

static QueueHandle_t gpio_evt_queue = NULL;

// AFAZER: Implementar contadores, com interrupções disparadas
//         por mudança de estado dos pinos. 
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_tratar_alarme(void* arg)
{
    uint32_t io_num;

    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

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
    
    // Configuração dos alarmes (entradas ligadas a interrupções)
    io_conf.pin_bit_mask = MASCARA_ALARMES;
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLDOWN_ENABLE;
    io_conf.intr_type = GPIO_INTR_POSEDGE;      // interrupt on rising edge
    gpio_config(&io_conf);

    //change gpio interrupt type for one pin
    // gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    //start gpio task
    xTaskCreate(gpio_tratar_alarme, "gpio_tratar_alarme", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(ALARME_1, gpio_isr_handler, (void*) ALARME_1);
    
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
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

}

// Documentação das funções públicas no arquivo header.

const char *controle_gpio_status(void) {
  return "Modulo de Controle\nVersao:1.0\nAtuadores:4\nSensores:2\nAlarmes:1\n\n";
}


int controle_gpio_ler_sensor(int id) {
  if((id > 0) && (id <= MAX_SENSORES)) {
    return gpio_get_level(mapa_sensores[id]);
  } else {
    // Condição de erro: id inválido
    return -1;
  }
}


void controle_gpio_mudar_atuador(int id, int valor) {
  if((id > 0) && (id <= MAX_ATUADORES)) {
    if ((valor >= 0) && (valor <= 1)) {
      gpio_set_level(mapa_atuadores[id], valor);
    }
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
