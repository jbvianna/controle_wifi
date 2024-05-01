/** @file app_config.h - Configuração do aplicativo
*/
#pragma once

#define MAX_SSID_LEN 32         ///< Tamanho máximo do ssid
#define MAX_PASSWORD_LEN 63     ///< Tamanho máximo da senha
#define MAX_HOSTNAME_LEN 63     ///< Tamanho máximo do nome do servidor


/** Ler configuração do aplicativo da memória FLASH.

  NOTA: Se o pino de voltar à configuração original estiver ligado (ON),
        ignora a configuração gravada e carrega um padrão 'de fábrica'.
*/
void app_config_ler(void);


/** Gravar configuração do aplicativo na memória FLASH.

    Se a configuração não foi alterada, nada será gravado.
*/
esp_err_t app_config_gravar(void);


/** Indica se modo Wifi deve ser Access Point (AP)
*/
bool app_config_softap(void);


/** Obtém ssid da conexão Wifi.
*/
const char *app_config_wifi_ssid(void);


/** Obtém senha da conexão Wifi.
*/
const char *app_config_wifi_password(void);


/** Obtém nome do servidor para mDNS.
*/
const char *app_config_hostname(void);


/** Altera modo da conexão Wifi.

    @param modo 0 para modo Station, outro para modo Access Point
*/
void app_config_set_softap(int modo);


/** Altera ssid da conexão Wifi.

    @param ssid ssid a utilizar
    @see MAX_SSID_LEN
    
    NOTA: Se o tamanho do parâmetro exceder o limite, nada será alterado.
*/
void app_config_set_wifi_ssid(const char *ssid);


/** Altera senha conexão Wifi.

    @param pwd senha a utilizar
    @see MAX_PASSWORD_LEN
    
    NOTA: Se o tamanho do parâmetro exceder o limite, nada será alterado.
*/
void app_config_set_wifi_password(const char *pwd);


/** Altera o nome do servidor HTTP (mDNS).

    @param nome Nome do servidor
    @see MAX_HOSTNAME_LEN
 */
void app_config_set_hostname(const char *nome);

