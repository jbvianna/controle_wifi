/** @file app_web_server.h - Servidor HTTP 
*/
#pragma once

/** Iniciar servidor http.

    Devolve o handle para o servidor, ou NULL, se houve erro.
*/
httpd_handle_t start_webserver(void);

/** Terminar servidor http.
*
* @param server Handle para o servidor
*/
esp_err_t stop_webserver(httpd_handle_t server);

