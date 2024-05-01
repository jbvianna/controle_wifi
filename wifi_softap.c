/** @file wifi_softap.c - Inicia Wifi no modo soft Access Point.

    Código original em:
    .../esp-idf/examples/wifi/getting_started/softAP/
    
    NOTA: Houve modificações do código original do exemplo conforme outros
          exemplos encontrados na Internet para permitir o uso juntamente
          com um servidor HTTP.
 */

/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

// #include "esp_http_server.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_softap.h"

/* The examples use WiFi configuration that you can set via project configuration menu.
*/
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

#define USAR_IP_FIXO 1

static const char *TAG = "wifi softAP";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ip_event_ap_staipassigned_t* event = event_data;
        ESP_LOGI(TAG, "SoftAP client connected with IP: "IPSTR,
            IP2STR(&event->ip));
    }
}


void wifi_init_softap(const char *ssid) {
  esp_netif_t *my_wifi_ap = esp_netif_create_default_wifi_ap();

#ifdef USAR_IP_FIXO

    // Create fixed address Access Point

    esp_netif_ip_info_t ip_info;

    esp_netif_get_ip_info(my_wifi_ap, &ip_info);
    
    ip_info.ip.addr = ESP_IP4TOADDR(192, 168, 0, 10);  // Endereço IP do servidor em modo AP
    ip_info.gw.addr = ESP_IP4TOADDR(192, 168, 0, 10);
    ip_info.netmask.addr = ESP_IP4TOADDR(255, 255, 255, 0);

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(my_wifi_ap));

    ESP_ERROR_CHECK(esp_netif_set_ip_info(my_wifi_ap, &ip_info));

    esp_netif_dhcps_start(my_wifi_ap);
#endif
    // Start WIFI
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_AP_STAIPASSIGNED,
                                                        &wifi_event_handler,
                                                        NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(ssid),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = "",
            .max_connection = 1,                  // EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            /*
            .pmf_cfg = {
                    .required = false,
            }, */
        },
    };
    strcpy((char *)wifi_config.ap.ssid, ssid);
    
    if (strlen((const char *)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             wifi_config.ap.ssid, wifi_config.ap.password, EXAMPLE_ESP_WIFI_CHANNEL);
}

