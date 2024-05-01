/** @file main.c - Controle de Periféricos através do Wifi.
 *
 *  Este aplicativo cria um protocolo para controle de um módulo
 *  esp32 utilizando uma conexão Wifi e um servidor HTTP.
 
    Author: Joao Vianna (jvianna@gmail.com)

    Version: 0.8

    Base do código - Exemplos da biblioteca esp-idf
    
    @see app_web_server.c Protocolo HTTP no arquivo.
    @see controle_gpio.c Interface com o micro-controlador no arquivo.
 */
 
 /*
    Header dos exemplos:

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.

    Author: Joao Vianna (jvianna@gmail.com)
    Version: 0.8
    History:
      20240415 - Versão inicial, copiada dos exemplos
      20240416 - Versão 0.2, respondendo a get/status, atuar e ler
      20240418 - Versão 0.3, protocolo HTTP GET depurado
      20240422 - Versão 0.5, código do cliente Wifi separado em outro módulo.
      20240427 - Versão 0.7, configuração no FLASH, modo Wifi configurável: AP ou STA
      20240428 - Versão 0.8, gravando configuração

    Known problems, improvements needed...
      Telas HTML embutidas, para testar e configurar.
      Documentação de uso e do software
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>

#include <esp_err.h>
#include <esp_log.h>

#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_eth.h>
#include <esp_http_server.h>

#include "wifi_station.h"
#include "wifi_softap.h"

#include "controle_gpio.h"
#include "app_config.h"
#include "app_web_server.h"


static const char *TAG = "main.c";


#define USAR_MDNS 1

#if defined USAR_MDNS

/* Código para mDNS -----------------------------------------------------------
 */
#include "esp_netif_ip_addr.h"
#include "esp_mac.h"
#include "mdns.h"
#include "netdb.h"


static void iniciar_mdns(const char *hostname)
{
  ESP_LOGI(TAG, "Iniciando mDNS");

  //initialize mDNS
  ESP_ERROR_CHECK(mdns_init());
  
  //set mDNS hostname (required if you want to advertise services)
  ESP_ERROR_CHECK(mdns_hostname_set(hostname));
  ESP_LOGI(TAG, "mdns hostname set to: [%s]", hostname);

  //set default mDNS instance name
  ESP_ERROR_CHECK( mdns_instance_name_set(CONFIG_MDNS_INSTANCE) );

  //structure with TXT records
  mdns_txt_item_t serviceTxtData[3] = {
    {"board", "esp32"},
    {"u", "user"},
    {"p", "password"}
  };

  //initialize service
  ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 
                                   CONFIG_HTTP_SERVER_PORT, serviceTxtData, 3));
  ESP_ERROR_CHECK(mdns_service_subtype_add_for_host("ESP32-WebServer", "_http", "_tcp", 
                                   NULL, "_server"));

  //add another TXT item
  // ESP_ERROR_CHECK(mdns_service_txt_item_set("_http", "_tcp", "path", "/foobar"));
  //change TXT item value
  // ESP_ERROR_CHECK( mdns_service_txt_item_set_with_explicit_value_len("_http", "_tcp", "u", "admin", strlen("admin")) );
}


/* Fim do código para mDNS ----------------------------------------------------
 */
#endif


/*  Para o servidor HTTP quando a conexão Wifi é perdida.
 */
static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Parando servidor http");
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE(TAG, "Erro ao parar servidor http");
        }
    }
}


/*  Reinicia o servidor HTTP quando a conexão Wifi é restabelecida.
 */
static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Iniciando servidor http");
        *server = start_webserver();
    }
}

/** Função principal do aplicativo.

    Ativa os diversos módulos para:
      Configurar o micro-controlador;
      Ler a configuração de memória permantente;
      Iniciar a comunicação via Wifi no modo configurado;
      Iniciar o servidor HTTP.
 */
void app_main(void)
{
  // Declarado como static para continuar existindo quando app_main() terminar.
  static httpd_handle_t server = NULL;
  
  // Prepara portas para operação
  controle_gpio_iniciar();

  //Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Ler configuração gravada na memória permanente (FLASH)
  app_config_ler();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());

#if defined USAR_MDNS
  iniciar_mdns(app_config_hostname());
#endif

  if (app_config_softap()) {
    ESP_LOGI(TAG, "Iniciando Wifi Soft Access Point");
    wifi_init_softap(app_config_wifi_ssid());

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, &server));
  } else {
    ESP_LOGI(TAG, "Iniciando Wifi Station");
    wifi_init_sta(app_config_wifi_ssid(),
                  app_config_wifi_password());

    /* Register event handlers to stop the server when Wi-Fi is disconnected and
     * restart it upon connection.
     */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
  }

  /* Start the server for the first time */
  server = start_webserver();
  
  if (server == NULL) {
    ESP_LOGE(TAG, "Falha ao ativar servidor");
  }
}

