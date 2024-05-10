/** @file app_config.c - Configuração do aplicativo em memória permanente.

    Cria uma camada de abstração que esconde dos demais módulos a maneira
    como a configuração é armazenada.
    
    Recupera e armazena informações de configuração gravadas na memória FLASH
    utilizando o sistema de arquivos littlefs.
    
    NOTA:
      Existe um pino no micro-controlador que força o retorno à configuração
      original. Se este pino estiver ativado (ON), a configuração armazenada
      é ignorada, e os valores 'de fábrica' são utilizados. 
    
    @see https://github.com/littlefs-project/littlefs
    
    @author Joao Vianna (jvianna@gmail.com)
    @version 0.82 03/05/2024
      - Pino do micro-controlador força retorno às configurações de fábrica.
      - Adicionado campo hostname e pequenas mudanças nos nomes dos parâmetros.

    Código de armazenamento derivado de:
    .../esp-idf/examples/storage/littlefs/main/esp_littlefs_example.c
 */

/*  Header do exemplo:

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   
   History:
     20240427 - Código movido para módulo próprio
     Version 0.5 - Recuperando e salvando configuração para memória FLASH
     Versão 0.82

   Known problems, improvements needed...

*/
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_littlefs.h"

#include "controle_gpio.h"
#include "app_config.h"

#define TAG "app config"


static esp_vfs_littlefs_conf_t littlefs_conf = {
      .base_path = "/littlefs",
      .partition_label = "storage",
      .format_if_mount_failed = true,
      .dont_mount = false,
};

/*  Inicia e monta um sistema de arquivos LittleFS.
 */
static void iniciar_littlefs() {
  ESP_LOGI(TAG, "Initializing LittleFS");

  // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
  esp_err_t ret = esp_vfs_littlefs_register(&littlefs_conf);

  if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
          ESP_LOGE(TAG, "Failed to mount or format filesystem");
      } else if (ret == ESP_ERR_NOT_FOUND) {
          ESP_LOGE(TAG, "Failed to find LittleFS partition");
      } else {
          ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
      }
      return;
  }

  size_t total = 0, used = 0;
  ret = esp_littlefs_info(littlefs_conf.partition_label, &total, &used);
  if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
      esp_littlefs_format(littlefs_conf.partition_label);
  } else {
      ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
}


/*  Desmonta o sistema de arquivos após utilização.
 */
static void terminar_littlefs() {
  // All done, unmount partition and disable LittleFS
  esp_vfs_littlefs_unregister(littlefs_conf.partition_label);
  ESP_LOGI(TAG, "LittleFS unmounted");
}


#define MAX_CFG_PARAM_LEN 31

static const char* nomes_modo_wifi[2] = {
  "STA",
  "AP"
};


/*  Estrutura de configuração específica do aplicativo
 */
typedef struct {
  char wifi_ssid[MAX_SSID_LEN + 1];             // Identificador do serviço Wifi
  char wifi_password[MAX_CFG_VALUE_LEN + 1];     // Senha do Wifi

  char hostname[MAX_CFG_VALUE_LEN + 1];          // Nome do servidor HTTP
  int  modo_wifi;                               // Station ou AP
  
  bool  modified;                               // Indica se a configuração mudou
} app_config_t;


/*  Dados de configuração em memória.
    Estes dados só são gravados em memória permanente por app_config_gravar().
*/
static app_config_t app_config;

// Ver a descrição das funções públicas em app_config.h

enum modo_conexao_wifi app_config_modo_wifi(void) {
  return (app_config.modo_wifi);
}


const char *app_config_wifi_ssid(void) {
  return (const char *)app_config.wifi_ssid;
}


const char *app_config_wifi_password(void) {
  return (const char *)app_config.wifi_password;
}


const char *app_config_hostname(void) {
  return (const char *)app_config.hostname;
}


void app_config_set_modo_wifi(const char* modo) {
  if (strcmp(modo, "STA") == 0) {
    ESP_LOGI(TAG, "Modo wifi: STA");
    app_config.modo_wifi = MODO_WIFI_STA;
  } else if (strcmp(modo, "AP") == 0) {
    ESP_LOGI(TAG, "Modo wifi: AP");
    app_config.modo_wifi = MODO_WIFI_AP;
  } else {
    ESP_LOGE(TAG, "Modo Wifi desconhecido: %s", modo);
    return;  
  }
  app_config.modified = true;
}


void app_config_set_wifi_ssid(const char *ssid) {
  size_t len = strlen(ssid);
  
  if (len <= MAX_SSID_LEN ) {
    ESP_LOGI(TAG, "Wifi ssid: '%s'", ssid);
    strcpy(app_config.wifi_ssid, ssid);
    app_config.modified = true;
  }
}


void app_config_set_wifi_password(const char *pwd) {
  size_t len = strlen(pwd);
      
  if (len <= MAX_CFG_VALUE_LEN ) {
    ESP_LOGI(TAG, "Senha: '%s'", pwd);
    strcpy(app_config.wifi_password, pwd);
    app_config.modified = true;
  }
}


void app_config_set_hostname(const char *nome) {
  size_t len = strlen(nome);
      
  if (len <= MAX_CFG_VALUE_LEN ) {
    ESP_LOGI(TAG, "Hostname: '%s'", nome);
    strcpy(app_config.hostname, nome);
    app_config.modified = true;
  }
}


void app_config_ler(void) {
  // Iniciar com configuração padrão de fábrica
  app_config_set_wifi_ssid(CONFIG_ESP_WIFI_SSID);
  app_config_set_wifi_password(CONFIG_ESP_WIFI_PASSWORD);
  app_config_set_hostname(CONFIG_MDNS_HOSTNAME);
  app_config_set_modo_wifi("AP");

  if(controle_gpio_reconfig()) {
    ESP_LOGI(TAG, "Voltando a config original.");
    app_config.modified = false;
    return;
  }
  
  iniciar_littlefs();

  ESP_LOGI(TAG, "Lendo config na memória FLASH...");

  FILE *f = fopen("/littlefs/config.txt", "r");

  char line[128];
  char param[MAX_CFG_PARAM_LEN + 1];
  char value[MAX_CFG_VALUE_LEN + 1];
  
  if (f == NULL) {
    // Se arquivo principal apresenta falha, tenta backup.
    ESP_LOGI(TAG, "Erro abrindo arquivo. Tentando backup...");
    f =  fopen("/littlefs/config.bak", "r");
  }

  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return;
  }

  while(fgets(line, sizeof(line), f) != NULL) {
    // ESP_LOGI(TAG, "Read from file: '%s'", line);
    
    // Linhas de configuração válidas são da forma: nome = valor
    int count = sscanf(line, "%31[^= ] = %63[^\n]", param, value);
    
    if(count > 1) {
      if (strcmp(param, "password") == 0) {
        if (count == 1) {
          // Senha pode ser vazia
          value[0] = 0;
        }
        app_config_set_wifi_password(value);
      } else if (count == 2) {
        if (strcmp(param, "ssid") == 0) {
          app_config_set_wifi_ssid(value);
        } else if (strcmp(param, "hostname") == 0) {
          app_config_set_hostname(value);
        } else if (strcmp(param, "modo_wifi") == 0) {
          app_config_set_modo_wifi(value);
        }
      }
    }
  }
  fclose(f);
  
  if(app_config.wifi_ssid[0] == '\0') {
    strcpy(app_config.wifi_ssid, CONFIG_ESP_WIFI_SSID);
  }
  terminar_littlefs();

  app_config.modified = false;
}

esp_err_t app_config_gravar(void) {
  if (! app_config.modified) {
    // Se não houve modificação...
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Salvando configuração para memória FLASH");
  iniciar_littlefs();

  FILE *f = fopen("/littlefs/config.tmp", "w");
  
  fprintf(f, "ssid=%s\n",  app_config.wifi_ssid);
  fprintf(f, "password=%s\n",  app_config.wifi_password);
  fprintf(f, "hostname=%s\n",  app_config.hostname);
  fprintf(f, "modo_wifi=%s\n",  nomes_modo_wifi[app_config.modo_wifi]);
  fprintf(f, "\n");
  
  fclose(f);

  // Check if destination file exists before renaming
  struct stat st;

  if (stat("/littlefs/config.bak", &st) == 0) {
      // Delete it if it exists
      unlink("/littlefs/config.bak");
  }

  // Rename original file
  ESP_LOGI(TAG, "Renaming file");
  if (rename("/littlefs/config.txt", "/littlefs/config.bak") != 0) {
      ESP_LOGE(TAG, "Falha criando backup");
      return ESP_ERR_NOT_FINISHED;
  } else {
    if (rename("/littlefs/config.tmp", "/littlefs/config.txt") != 0) {
      ESP_LOGE(TAG, "Falha salvando arquivo de configuração.");
      return ESP_ERR_NOT_FINISHED;
    }
  }
  terminar_littlefs();

  app_config.modified = false;
  return ESP_OK;
}
