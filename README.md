| Supported Targets | ESP32 | ESP32-C3 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- |

# Controle de periféricos através do Wifi

Derivado de Simple HTTPD Server Example
Derivado de Example: GPIO

Um servidor HTTPD recebe comandos GET, e repassa para os periféricos do ESP32.

  Periféricos implementados:
    Atuador (saída com estado 0 - desativado; 1 - ativado)
    Alarme (conta e registra eventos)
    Sensor (entradas com estado 0 - desativado; 1 - ativado)

  Protocolo:  
    1. URI \status
        devolve o status da placa
    2. URI \atuar?at=<id>&valor=<0 ou 1>
        ativa ou desativa um atuador
    3. URI \ler?<perif>=<id>:
        lê informações do periférico indicado, onde perif = at, al ou sn
    4. URI \reset
        limpa estado da placa
       URI \reset?al=<id>
        limpa estado de um único alarme

## How to use example

    http://192.168.0.10/status
    http://192.168.0.10/ler?sn=1
    http://192.168.0.10/atuar?at=1&v=0
    http://192.168.0.10/atuar?at=1&v=1
  

### Hardware Required

* A development board with ESP32/ESP32-S2/ESP32-C3 SoC (e.g., ESP32-DevKitC, ESP-WROVER-KIT, etc.)
* A USB cable for power supply and programming

### Configure the project

* Run set-target once, to clear or create the build files, and menuconfig to change project settings.

``` 
idf.py set-target esp32
```
```
idf.py menuconfig
```
* Open the project configuration menu (`idf.py menuconfig`) to configure Wi-Fi.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py build
idf.py -p <PORT> flash monitor
```

(Replace <PORT> with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

### Test the example :
        * TODO

## Example Output
```
I (9580) example_connect: - IPv4 address: 192.168.194.219
I (9580) example_connect: - IPv6 address: fe80:0000:0000:0000:266f:28ff:fe80:2c74, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (9590) example: Starting server on port: '80'
I (9600) example: Registering URI handlers
I (66450) example: Found header => Host: 192.168.194.219
I (66460) example: Request headers lost
```

## Troubleshooting
* If the server log shows "httpd_parse: parse_block: request URI/header too long", especially when handling POST requests, then you probably need to increase HTTPD_MAX_REQ_HDR_LEN, which you can find in the project configuration menu (`idf.py menuconfig`): Component config -> HTTP Server -> Max HTTP Request Header Length
