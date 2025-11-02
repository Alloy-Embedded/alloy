# ESP32 WiFi Station Example

Este exemplo demonstra como conectar o ESP32 a uma rede WiFi e fazer requisições HTTP usando o Alloy com ESP-IDF.

## O Que Este Exemplo Faz

1. **Inicializa o WiFi** em modo Station (cliente)
2. **Conecta a um Access Point** (roteador WiFi)
3. **Obtém endereço IP** via DHCP
4. **Faz uma requisição HTTP GET** para `http://example.com`
5. **Exibe os resultados** no monitor serial

## Estrutura ESP-IDF

Este exemplo usa a estrutura de projeto ESP-IDF correta:

```
esp32_wifi_station/
├── CMakeLists.txt           # Projeto ESP-IDF root
├── sdkconfig.defaults        # Configurações otimizadas
├── main/
│   ├── CMakeLists.txt       # Componente principal
│   └── main.cpp             # Código do exemplo
└── README.md                # Este arquivo
```

Esta estrutura permite que o ESP-IDF gerencie componentes automaticamente.

## Detecção Automática de Componentes

O Alloy detecta automaticamente os componentes ESP-IDF necessários baseado nos `#include`:

```cpp
#include <esp_wifi.h>         // → esp_wifi, esp_netif, nvs_flash
#include <esp_http_client.h>  // → esp_http_client, esp-tls
```

Você pode ver quais componentes foram detectados durante a compilação.

## Como Usar

### 1. Configurar Credenciais WiFi

Edite `main/main.cpp` e altere as credenciais:

```cpp
#define WIFI_SSID      "YOUR_WIFI_SSID"      // Seu SSID
#define WIFI_PASSWORD  "YOUR_WIFI_PASSWORD"  // Sua senha
```

### 2. Compilar

```bash
cd examples/esp32_wifi_station
source ~/esp/esp-idf/export.sh
idf.py build
```

### 3. Gravar e Monitorar

```bash
# Descobrir porta
ls /dev/cu.usbserial-*  # macOS
ls /dev/ttyUSB*         # Linux

# Gravar e monitorar
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

Ou em um comando:
```bash
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

## Saída Esperada

```
I (1234) wifi_station: ========================================
I (1234) wifi_station:   Alloy ESP32 WiFi Station Example
I (1234) wifi_station: ========================================
I (1234) wifi_station:
I (1234) wifi_station: SSID: MyWiFiNetwork
I (1234) wifi_station:
I (1234) wifi_station: WiFi started, connecting to AP...
I (2345) wifi_station: Connected! IP Address: 192.168.1.100
I (2345) wifi_station:
I (2345) wifi_station: WiFi connected successfully!
I (2345) wifi_station: Now performing HTTP GET request...
I (2345) wifi_station:
I (3456) wifi_station: HTTP_EVENT_ON_CONNECTED
I (3456) wifi_station: HTTP_EVENT_HEADER_SENT
I (3500) wifi_station: HTTP_EVENT_ON_HEADER, key=Content-Type, value=text/html
I (3550) wifi_station: HTTP_EVENT_ON_DATA, len=1256
Received data: <!doctype html>
<html>
<head>
    <title>Example Domain</title>
...
I (3600) wifi_station: HTTP_EVENT_ON_FINISH
I (3600) wifi_station: HTTP GET Status = 200, content_length = 1256
I (3600) wifi_station:
I (3600) wifi_station: Example complete! WiFi is connected and working.
I (3600) wifi_station:
I (13600) wifi_station: Still connected...
```

## Troubleshooting

### Erro: "Failed to connect to WiFi"

Verifique:
1. SSID e senha corretos em `main.cpp`
2. ESP32 está dentro do alcance do roteador
3. Roteador usa WPA2 (não WEP ou aberto)

### Erro: "HTTP GET request failed"

Verifique:
1. ESP32 está conectado ao WiFi (veja mensagem "Connected!")
2. Roteador permite acesso à internet
3. Firewall não está bloqueando

### Erro ao compilar: "idf.py not found"

Configure o ambiente ESP-IDF:
```bash
source ~/esp/esp-idf/export.sh
```

### Porta USB não encontrada

**macOS**: Instale driver CP2102
- https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

**Linux**: Adicione permissões
```bash
sudo usermod -a -G dialout $USER
# Faça logout/login
```

## Personalização

### Alterar Número de Tentativas

Em `main.cpp`:
```cpp
#define MAX_RETRY_COUNT 5  // Altere para mais/menos tentativas
```

### Alterar URL do HTTP GET

Em `main.cpp`, função `http_get_task()`:
```cpp
config.url = "http://example.com";  // Altere para outro URL
```

### Adicionar HTTPS

O exemplo já suporta HTTPS! Basta mudar a URL:
```cpp
config.url = "https://example.com";  // HTTPS funciona automaticamente
```

## O Que Aprender Deste Exemplo

1. **Estrutura de projeto ESP-IDF** - Como organizar arquivos
2. **Inicialização WiFi** - Sequência completa de setup
3. **Event handlers** - Como responder a eventos WiFi
4. **HTTP client** - Como fazer requisições HTTP/HTTPS
5. **Detecção automática** - Como o Alloy detecta componentes

## Próximos Passos

Depois deste exemplo funcionar, você pode:

- ✅ Criar um servidor HTTP (`esp_http_server.h`)
- ✅ Conectar a um broker MQTT (`mqtt_client.h`)
- ✅ Implementar BLE (`esp_bt.h`)
- ✅ Adicionar OTA updates (`esp_ota_ops.h`)
- ✅ Fazer parsing JSON (adicionar biblioteca cJSON)

## Componentes ESP-IDF Usados

Este exemplo usa os seguintes componentes ESP-IDF:

- `esp_system` - Sistema base
- `esp_wifi` - Driver WiFi
- `esp_netif` - Interface de rede
- `nvs_flash` - Armazenamento não-volátil
- `esp_event` - Sistema de eventos
- `esp_http_client` - Cliente HTTP/HTTPS
- `esp-tls` - TLS/SSL
- `mbedtls` - Criptografia

Todos são detectados automaticamente pelos includes!

## Arquivos Importantes

- `CMakeLists.txt` - Configuração do projeto ESP-IDF
- `main/CMakeLists.txt` - Configuração do componente main
- `main/main.cpp` - Código principal
- `sdkconfig.defaults` - Configurações otimizadas
- `sdkconfig` (gerado) - Configurações completas

## Referências

- [ESP-IDF WiFi Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [ESP-IDF HTTP Client](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_client.html)
- [Alloy ESP32 Automatic Components](../../docs/ESP32_AUTOMATIC_COMPONENTS.md)
- [ESP-IDF Build System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html)
