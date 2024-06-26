/** @file controle_gpio.h - Controle dos atuadores e sensores.
*/

#pragma once

/** Tipos de perifericos controlados.
*/
enum periferico {
  PRF_NDEF,                     ///< Periférico não definido.
  PRF_ATUADOR,                  ///< Atuador binário (0 - Off, 1 - On).
  PRF_CONTADOR,                 ///< Contador de eventos (Alarme).
  PRF_SENSOR                    ///< Sensor binário (0 - Off, 1 - On).
};

#define MAX_CONTADORES  1       ///< Máximo de contadores no módulo.
#define MAX_ATUADORES   4       ///< Máximo de atuadores no módulo.
#define MAX_SENSORES    2       ///< Máximo de sensores no módulo.

/** Preparar a placa controladora para operar com o aplicativo.

    Deve ser ativada no início da lógica do aplicativo, antes da lógica
    que age sobre os diversos periféricos.
*/
void controle_gpio_iniciar(void);


/** Ativa um timer de baixa frequencia para controlar contadores e
    a duração dos pulsos nos atuadores. 
 */
bool controle_gpio_ativar_timer(void);


/** Obter status da placa controladora.

    @return Texto indicando o estado do módulo (número de dispositivos, etc.)
*/
const char *controle_gpio_status(void);


/** Ler valor de um sensor

    @param id Identificador do sensor (de 1 a MAX_SENSORES)

    @return 1 se o sensor está em nível ligado e 0 se está desligado.
    
    @todo Acrescentar sensores do tipo ADC, com valores inteiros.
*/
int controle_gpio_ler_sensor(int id);


/** Ler contagem de eventos de um contador

    Os eventos são disparados por uma transição de nível alto para
    nível baixo no pino do contador, que é iniciado como entrada
    com PULL_UP interno, para simplificar os circuitos eletrônicos.

    @param id Identificador do contador (de 1 a MAX_CONTADORES)

    @return a contagem
*/
int controle_gpio_ler_contador(int id);


/** Reiniciar a contagem de um contador para zero.

    @param id     Identificador do contador (de 1 a MAX_CONTADORES)
*/
void controle_gpio_reiniciar_contador(int id);


/** Mudar o valor de um atuador.

    @param id     Identificador do atuador (de 1 a MAX_ATUADORES)
    @param valor  0 para desligado; 1 para ligado
*/
void controle_gpio_mudar_atuador(int id, int valor);


/** Alternar o valor de um atuador entre 0 e 1 (toggle)

    @param id     Identificador do atuador (de 1 a MAX_ATUADORES)
 */
void controle_gpio_alternar_atuador(int id);


/** Cria um pulso positivo no valor de um atuador com certa duração

    O atuador é ligado por um tempo determinado, e depois desligado.

    @param id       Identificador do atuador (de 1 a MAX_ATUADORES)
    @param duracao  Duração do pulso em milissegundos
    
    @todo Completar implementação utilizando um temporizador.
 */
void controle_gpio_pulsar_atuador(int id, int duracao);


/** Indica se o sensor de reconfiguração está ativado.
 */
bool controle_gpio_reconfig(void);
