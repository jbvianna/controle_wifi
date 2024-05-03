/** @file app_web_server.c - Servidor HTTP
 *
    Um aplicativo cliente controla um módulo contendo atuadores,
    sensores, etc, utilizando um protocolo HTTP com o servidor.
    
    @author João Vianna (jvianna@gmail.com)
    @version 0.82

    As solicitações seguem o protocolo:
    
    GET /status
    
      Para obter um texto indicando o status do controlador.
      
    GET /sensor?id=(identificador)
    
      Para ler o estado de um sensor (porta de entrada do módulo),
      Onde id indica o sensor para o qual se deseja obter o valor.
      
    POST /atuador(id)
    
      action=("off", "on", "toggle" ou "pulse")
      [duration=(tempo em ms)]
      
      Para alterar o estado de um atuador (porta de saída do módulo),
      Onde off desliga, on liga, toggle alterna o estado e
      pulse liga e desliga após duração de tempo determinada.
    
    POST /config
    
      ssid=(ssid do Wifi)
      password=(senha do Wifi)
      wifi_mode=(STA/AP)
      hostname=(nome do servidor HTTP)

      Para alterar a configuração do módulo.

    Base do código - Exemplos da biblioteca esp-idf
    Derivado de Simple HTTPD Server Example e RESTful Server
    
    Histórico:
      - Versão 0.82 modificação do protocolo, aproximando-se mais do modelo RESTful,
        com a diferença de que o conteúdo das mensagens é em texto (text/plain).            

    @see app_config.h
 */
 
/* Controle de Periféricos através do Wifi
  
  Header do exemplo:

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_tls_crypto.h"
#include "esp_http_server.h"
#include "esp_vfs.h"
// #include "cJSON.h"

#include "utilitarios.h"
#include "controle_gpio.h"
#include "app_config.h"
#include "app_web_server.h"

static const char *TAG = "app-web-server";

// Mensagens específicas do módulo
enum msg_local {
  MSGL_0,
  MSGL_1,
  MSGL_OK,
  MSGL_CREATED,
  MSGL_FALTAM_PARAMETROS,
  MSGL_PARAMETRO_INVALIDO
};

// Nota: as mensagens abaixo têm que corresponder ao enumerado msg_local
static const char *mensagens_locais[] = {
  "0",
  "1",
  "200 Ok",
  "201 Created",
  "400 Faltam Parametros",
  "400 Parametro invalido"
};


/*  Função auxiliar, converte valor lido do sensor na mensagem apropriada
 */
static int valor_sensor_para_msg(int valor) {
  if (valor < 0) {
    return (MSGL_PARAMETRO_INVALIDO);
  } else if (valor == 0) {
    return (MSGL_0);
  } else {
    return (MSGL_1);
  }
}


#if CONFIG_EXAMPLE_BASIC_AUTH

/*  Código de autenticação básica, caso seja configurada.
 */

typedef struct {
    char    *username;
    char    *password;
} basic_auth_info_t;

#define HTTPD_401      "401 UNAUTHORIZED"                   // HTTP Response 401

#define BASIC_REALM    "Basic realm=\"controle Wifi\""

static char *http_auth_basic(const char *username, const char *password)
{
    int out;
    char *user_info = NULL;
    char *digest = NULL;
    size_t n = 0;
    asprintf(&user_info, "%s:%s", username, password);
    if (!user_info) {
        ESP_LOGE(TAG, "No enough memory for user information");
        return NULL;
    }
    esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));

    /* 6: The length of the "Basic " string
     * n: Number of bytes for a base64 encode format
     * 1: Number of bytes for a reserved which be used to fill zero
    */
    digest = calloc(1, 6 + n + 1);
    if (digest) {
        strcpy(digest, "Basic ");
        esp_crypto_base64_encode((unsigned char *)digest + 6, n, (size_t *)&out, (const unsigned char *)user_info, strlen(user_info));
    }
    free(user_info);
    return digest;
}

/* An HTTP GET handler */
static esp_err_t basic_auth_get_handler(httpd_req_t *req)
{
    char *buf = NULL;
    size_t buf_len = 0;
    basic_auth_info_t *basic_auth_info = req->user_ctx;

    buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if (buf_len > 1) {
        buf = calloc(1, buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "No enough memory for basic authorization");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Authorization: %s", buf);
        } else {
            ESP_LOGE(TAG, "No auth value received");
        }

        char *auth_credentials = http_auth_basic(basic_auth_info->username, basic_auth_info->password);
        if (!auth_credentials) {
            ESP_LOGE(TAG, "No enough memory for basic authorization credentials");
            free(buf);
            return ESP_ERR_NO_MEM;
        }

        if (strncmp(auth_credentials, buf, buf_len)) {
            ESP_LOGE(TAG, "Not authenticated");
            httpd_resp_set_status(req, HTTPD_401);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            httpd_resp_set_hdr(req, "WWW-Authenticate", BASIC_REALM);
            httpd_resp_send(req, NULL, 0);
        } else {
            ESP_LOGI(TAG, "Authenticated!");
            char *basic_auth_resp = NULL;
            httpd_resp_set_status(req, HTTPD_200);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            asprintf(&basic_auth_resp, "{\"authenticated\": true,\"user\": \"%s\"}", basic_auth_info->username);
            if (!basic_auth_resp) {
                ESP_LOGE(TAG, "No enough memory for basic authorization response");
                free(auth_credentials);
                free(buf);
                return ESP_ERR_NO_MEM;
            }
            httpd_resp_send(req, basic_auth_resp, strlen(basic_auth_resp));
            free(basic_auth_resp);
        }
        free(auth_credentials);
        free(buf);
    } else {
        ESP_LOGE(TAG, "No auth header received");
        httpd_resp_set_status(req, HTTPD_401);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Connection", "keep-alive");
        httpd_resp_set_hdr(req, "WWW-Authenticate", BASIC_REALM);
        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}

static httpd_uri_t basic_auth = {
    .uri       = "/basic_auth",
    .method    = HTTP_GET,
    .handler   = basic_auth_get_handler,
};

static void httpd_register_basic_auth(httpd_handle_t server)
{
    basic_auth_info_t *basic_auth_info = calloc(1, sizeof(basic_auth_info_t));
    if (basic_auth_info) {
        basic_auth_info->username = CONFIG_EXAMPLE_BASIC_AUTH_USERNAME;
        basic_auth_info->password = CONFIG_EXAMPLE_BASIC_AUTH_PASSWORD;

        basic_auth.user_ctx = basic_auth_info;
        httpd_register_uri_handler(server, &basic_auth);
    }
}
#endif

// Buffer que vai de servir de contexto para as mensagens, com área de rascunho.
#define SCRATCH_BUFSIZE (4096)

typedef struct rest_server_context {
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

static rest_server_context_t rest_context = { 0 };


/*  Preenche buffer com conteúdo de uma mensagem POST.

    Retorna o número de bytes lidos, ou -1 se houve erro.
 */
static int ler_conteudo_post(httpd_req_t *req, char *buf, int buf_size) {
  int total_len = req->content_len;
  int cur_len = 0;
  
  int received = 0;
  
  if (total_len >= buf_size) {
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long");
    return -1;
  }
  while (cur_len < total_len) {
    received = httpd_req_recv(req, buf + cur_len, total_len - cur_len);
    if (received <= 0) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Client failed to post request content");
        return -1;
    }
    cur_len += received;
  }
  buf[total_len] = '\0';

  return total_len;
}


/*  Preencher os cabeçalhos padrão de uma resposta de tipo texto.
    Acrescenta permissões de controle de acesso.
 */
static void preencher_cabecalho_text_plain(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET,HEAD,POST");
    httpd_resp_set_hdr(req, "Content-Type", "text/plain");
}


/*  Trata GET /status[?<dev>=<id>].

    Retorna texto descrevendo o status do micro-controlador.
 */
static esp_err_t get_status_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    
    int periferico = PRF_NDEF;
    int id_perif = 0;

    // Resposta padrão, a que veio no contexto do pedido.
    const char* resp_str = (const char*) req->user_ctx;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            // ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            // ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];

            if (httpd_query_key_value(buf, "at", param, sizeof(param)) == ESP_OK) {
                ESP_LOGD(TAG, "Found URL query parameter => at=%s", param);
                if (periferico == PRF_NDEF ) {
                    periferico = PRF_ATUADOR;
                    id_perif = atoi(param);
                }
              }
            if (httpd_query_key_value(buf, "al", param, sizeof(param)) == ESP_OK) {
                ESP_LOGD(TAG, "Found URL query parameter => al=%s", param);
                if (periferico == PRF_NDEF ) {
                    periferico = PRF_ALARME;
                    id_perif = atoi(param);
                } else {
                    ESP_LOGE(TAG, "Apenas um periferico por vez!!!");
                    resp_str = mensagens_locais[MSGL_PARAMETRO_INVALIDO];
                }
            }
            if (httpd_query_key_value(buf, "id", param, sizeof(param)) == ESP_OK) {
                ESP_LOGD(TAG, "Found URL query parameter => id=%s", param);
                if (periferico == PRF_NDEF ) {
                    periferico = PRF_SENSOR;
                    id_perif = atoi(param);
                } else {
                    ESP_LOGE(TAG, "Apenas um periferico por vez!!!");
                    resp_str = mensagens_locais[MSGL_PARAMETRO_INVALIDO];
                }
            }
            ESP_LOGD(TAG, "Status de periferico: %d; id: %d", periferico, id_perif);
            
            if (periferico == PRF_SENSOR && id_perif > 0) {
              int valor = controle_gpio_ler_sensor(id_perif);
              
              ESP_LOGD(TAG, "Valor do sensor %d: %d", id_perif, valor);
              resp_str = mensagens_locais[valor_sensor_para_msg(valor)];
            }
        }
        free(buf);
    } else {
      /* GET /status (sem parâmetros)
         Devolvemos o status da placa de controle
      */
      resp_str = controle_gpio_status();
    }

    preencher_cabecalho_text_plain(req);

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t get_status_uri = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = get_status_handler,
    .user_ctx  = &rest_context
};


enum acao_atuador {
  ACAO_ATUADOR_OFF,             // Esta deve ser sempre a primeira
  ACAO_ATUADOR_ON,
  ACAO_ATUADOR_TOGGLE,
  ACAO_ATUADOR_PULSE,
  ACAO_ATUADOR_NOP              // Esta deve ser sempre a última
};


// Este array deve coincidir com o enumerado acima.
const char *acoes_conhecidas [] = {
  "off",
  "on",
  "toggle",
  "pulse",
  "no action"
};


/*  Trata POST /atuador(id)

    action=(on/off/toggle/pulse)
    duration=(tempo em ms)

    Modifica o estado de um atuador.
    id é o identificador do atuador, começando em 1.

    Parâmetros:
      id_perif - identificador do periférico.
      req - requisição de serviço vinda do cliente.
 */
static esp_err_t post_atuador_n_handler(int id_perif, httpd_req_t *req) {
  // Prepara área de rascunho
  char*  buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
  char*  saved_ptr;
  char*  param;
  
  int acao = ACAO_ATUADOR_NOP;
  int duracao = 0;
  
  int resposta = MSGL_CREATED;
  
  int received = ler_conteudo_post(req, buf, SCRATCH_BUFSIZE);
  
  if (received < 0) {
    return ESP_FAIL;
  }
  
  // Percorrendo linhas do conteúdo
  char *linha = strtok_r(buf, "\n", &saved_ptr);
  
  while (linha != NULL) {

    if (str_startswith(linha, "action=")) {
      param = linha + 7;
      // ESP_LOGI(TAG, "Ação sobre atuador: %s", param);
      
      acao = ACAO_ATUADOR_OFF;    // Começa da primeira ação conhecida
      
      while (acao < ACAO_ATUADOR_NOP) {
        if (strcmp(acoes_conhecidas[acao], param) == 0) {
          break;
        }
        acao++;
      }      
    } else if (str_startswith(linha, "duration=")) {
      param = linha + 9;
      // ESP_LOGI(TAG, "Duração: %s", param);
      duracao = atoi(param);
    } else if (strlen(linha) > 0) {
      ESP_LOGE(TAG, "Parâmetro desconhecido por atuador: %s", linha);
    }
    linha = strtok_r(NULL, "\n", &saved_ptr);
  }

  switch (acao) {
    case ACAO_ATUADOR_OFF:
        controle_gpio_mudar_atuador(id_perif, 0);
      break;
    case ACAO_ATUADOR_ON:
        controle_gpio_mudar_atuador(id_perif, 1);
      break;
    case ACAO_ATUADOR_TOGGLE:
        controle_gpio_alternar_atuador(id_perif);
      break;
    case ACAO_ATUADOR_PULSE:
        if (duracao > 0) {
          controle_gpio_pulsar_atuador(id_perif, duracao);
        } else {
          ESP_LOGE(TAG, "Duração inválida para pulso do atuador.");
        }
      break;
    case ACAO_ATUADOR_NOP:
      ESP_LOGE(TAG, "Ação desconhecida por atuador.");
      break;
  }
  // Preparar cabeçalhos da resposta
  preencher_cabecalho_text_plain(req);

  if (resposta >= MSGL_OK) {
    httpd_resp_set_status(req, mensagens_locais[resposta]);
  }
  httpd_resp_sendstr(req, mensagens_locais[resposta]);
  return ESP_OK;
}


static esp_err_t post_atuador1_handler(httpd_req_t *req) {
  return post_atuador_n_handler(1, req);
}


static const httpd_uri_t post_atuador1_uri = {
    .uri       = "/atuador1",
    .method    = HTTP_POST,
    .handler   = post_atuador1_handler,
    .user_ctx  = &rest_context
};


static esp_err_t post_atuador2_handler(httpd_req_t *req) {
  return post_atuador_n_handler(2, req);
}


static const httpd_uri_t post_atuador2_uri = {
    .uri       = "/atuador2",
    .method    = HTTP_POST,
    .handler   = post_atuador2_handler,
    .user_ctx  = &rest_context
};


static esp_err_t post_atuador3_handler(httpd_req_t *req) {
  return post_atuador_n_handler(3, req);
}


static const httpd_uri_t post_atuador3_uri = {
    .uri       = "/atuador3",
    .method    = HTTP_POST,
    .handler   = post_atuador3_handler,
    .user_ctx  = &rest_context
};


static esp_err_t post_atuador4_handler(httpd_req_t *req) {
  return post_atuador_n_handler(4, req);
}


static const httpd_uri_t post_atuador4_uri = {
    .uri       = "/atuador4",
    .method    = HTTP_POST,
    .handler   = post_atuador4_handler,
    .user_ctx  = &rest_context
};


/*  Trata GET /sensor?id=<identificador>.

    Lê o valor de um sensor, retornando 0 se desligado e 1 se ligado.
    id é o identificador do sensor a ser lido, começando de 1
 */
static esp_err_t get_sensor_handler(httpd_req_t *req)
{
  char*  buf;
  size_t buf_len;
  
  int resposta = MSGL_OK;
  
  /* Read URL query string length and allocate memory for length + 1,
   * extra byte for null termination */
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = malloc(buf_len);

    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      char param[32];

      // ESP_LOGI(TAG, "Found URL query => %s", buf);

      /* Obter id do sensor a ser lido */
      if (httpd_query_key_value(buf, "id", param, sizeof(param)) == ESP_OK) {
        int id_sensor = atoi(param);
        int valor = controle_gpio_ler_sensor(id_sensor);
        
        // ESP_LOGI(TAG, "Valor do sensor %d: %d", id_sensor, valor);
        
        if (valor == 0 || valor == 1 ) {
          // Nota: Apenas 0 e 1 são valores válidos nesta implementação.
          resposta = valor_sensor_para_msg(valor);
        }
      } else {
        resposta = MSGL_FALTAM_PARAMETROS;
      }
    }
    free(buf);
  } else {
    resposta = MSGL_FALTAM_PARAMETROS;
  }

  // Preparar cabeçalhos da resposta
  preencher_cabecalho_text_plain(req);

  if (resposta > MSGL_OK) {
    httpd_resp_set_status(req, "400 BAD REQUEST");
  }
  httpd_resp_send(req, mensagens_locais[resposta], HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}


static const httpd_uri_t get_sensor_uri = {
    .uri       = "/sensor",
    .method    = HTTP_GET,
    .handler   = get_sensor_handler,
    .user_ctx  = &rest_context
};


/*  Trata POST /config

    ssid=<ssid do Wifi>
    password=<password do Wifi>
    hostname=<nome do servidor HTTP na rede>
    modo_wifi=<modo da conexão "AP" ou "STA">

    Salva a configuração em memória permanente.
 */
static esp_err_t post_config_handler(httpd_req_t *req)
{
  char*  buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
  char*  saved_ptr;
  char*  param;

  int resposta = MSGL_CREATED;
  
  int received = ler_conteudo_post(req, buf, SCRATCH_BUFSIZE);
  
  if (received < 0) {
    return ESP_FAIL;
  }

  // Percorrendo linhas do conteúdo
  char *linha = strtok_r(buf, "\n", &saved_ptr);
  
  while (linha != NULL) {

    if (str_startswith(linha, "ssid=")) {
      param = linha + 5;
      // ESP_LOGI(TAG, "Novo ssid: %s", param);
      app_config_set_wifi_ssid(param);
    } else if (str_startswith(linha, "password=")) {
      param = linha + 9;
      // ESP_LOGI(TAG, "Nova senha: %s", param);
      app_config_set_wifi_password(param);
    } else if (str_startswith(linha, "hostname=")) {
      param = linha + 9;
      // ESP_LOGI(TAG, "Novo hostname: %s", param);
      app_config_set_hostname(param);
    } else if (str_startswith(linha, "modo_wifi=")) {
      param = linha + 10;
      // ESP_LOGI(TAG, "Modo Wifi: %s", param);
      app_config_set_modo_wifi(param);
    } else if (strlen(linha) > 0) {
      ESP_LOGE(TAG, "Parâmetro de configuração desconhecido: %s", linha);
    }
    linha = strtok_r(NULL, "\n", &saved_ptr);
  }

  // Preparar cabeçalhos da resposta
  preencher_cabecalho_text_plain(req);

  if (resposta >= MSGL_OK) {
    httpd_resp_set_status(req, mensagens_locais[resposta]);
  }

  httpd_resp_sendstr(req, mensagens_locais[resposta]);

  if (resposta == MSGL_CREATED) {
    if (app_config_gravar() == ESP_OK) {
      // Se alterou configuração na memória FLASH, é preciso reiniciar.
      ESP_LOGI(TAG, "Aguardando o módulo ser reiniciado");
      // sleep(1);
      // esp_restart();
    }
  }
  return ESP_OK;
}


static const httpd_uri_t post_config_uri = {
    .uri       = "/config",
    .method    = HTTP_POST,
    .handler   = post_config_handler,
    .user_ctx  = &rest_context
};


/*  Trata HEAD/

    Útil para IP scanners tentando encontrar o IP do sistema.
 */
static esp_err_t head_raiz_handler(httpd_req_t *req)
{
  int resposta = MSGL_OK;
  
  ESP_LOGI(TAG, "Tratando HEAD request na raiz");

  // Preparar cabeçalhos da resposta
  preencher_cabecalho_text_plain(req);

  httpd_resp_send(req, mensagens_locais[resposta], HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}


static const httpd_uri_t head_raiz_uri = {
    .uri       = "/",
    .method    = HTTP_HEAD,
    .handler   = head_raiz_handler,
    .user_ctx  = &rest_context
};


// Documentação das funções públicas no arquivo header.


httpd_handle_t start_webserver(void)
{
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  config.lru_purge_enable = true;
  config.server_port = CONFIG_HTTP_SERVER_PORT;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

  if (httpd_start(&server, &config) == ESP_OK) {
      // Set URI handlers
      ESP_LOGI(TAG, "Registering URI handlers");
      httpd_register_uri_handler(server, &get_status_uri);
      httpd_register_uri_handler(server, &get_sensor_uri);
      httpd_register_uri_handler(server, &post_atuador1_uri);
      httpd_register_uri_handler(server, &post_atuador2_uri);
      httpd_register_uri_handler(server, &post_atuador3_uri);
      httpd_register_uri_handler(server, &post_atuador4_uri);
      httpd_register_uri_handler(server, &post_config_uri);
      httpd_register_uri_handler(server, &head_raiz_uri);

      #if CONFIG_EXAMPLE_BASIC_AUTH
      httpd_register_basic_auth(server);
      #endif
      return server;
  }

  ESP_LOGE(TAG, "Error starting server!");
  return NULL;
}

esp_err_t stop_webserver(httpd_handle_t server)
{
  // Stop the httpd server
  return httpd_stop(server);
}

