/** @file app_config.h - Configuração do aplicativo
*/
#pragma once

#define MAX_SSID_LEN 32
#define MAX_CFG_VALUE_LEN 63


/** Modo da conexão Wifi (STAtion ou Access Point).
 */
enum modo_conexao_wifi {
  MODO_WIFI_STA,
  MODO_WIFI_AP
};


/** Ler configuração do aplicativo da memória FLASH.

  NOTA: Se o pino de voltar à configuração original estiver ligado (ON),
        ignora a configuração gravada e carrega um padrão 'de fábrica'.
*/
void app_config_ler(void);


/** Gravar configuração do aplicativo na memória FLASH.

    Se a configuração não foi alterada, nada será gravado.
*/
esp_err_t app_config_gravar(void);


/** Indica se modo Wifi deve ser Access Point (AP) ou Station (STA)
*/
enum modo_conexao_wifi app_config_modo_wifi(void);


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

    @param modo "STA" para modo Station, ou "AP" para modo Access Point
*/
void app_config_set_modo_wifi(const char *modo);


/** Altera ssid da conexão Wifi.

    @param ssid ssid a utilizar
    @see MAX_SSID_LEN
    
    NOTA: Se o tamanho do parâmetro exceder o limite, nada será alterado.
*/
void app_config_set_wifi_ssid(const char *ssid);


/** Altera senha conexão Wifi.

    @param pwd senha a utilizar
    @see MAX_CFG_VALUE_LEN
    
    NOTA: Se o tamanho do parâmetro exceder o limite, nada será alterado.
*/
void app_config_set_wifi_password(const char *pwd);


/** Altera o nome do servidor HTTP (mDNS).

    @param nome Nome do servidor
    @see MAX_CFG_VALUE_LEN
 */
void app_config_set_hostname(const char *nome);

