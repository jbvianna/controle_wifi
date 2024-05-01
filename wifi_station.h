/** @file wifi_station.h - Inicia Wifi no modo Station.
*/
#pragma once


/** Iniciar serviço de Wifi no modo Station.

    @param ssid  Identificador do serviço Wifi com que conectar.
    @param pwd   Senha a utilizar para se conectar.
 */
void wifi_init_sta(const char *ssid, const char *pwd);