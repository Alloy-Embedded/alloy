# 🚀 ESP32 Quick Start (2 Comandos!)

Guia super simplificado para compilar e gravar no ESP32.

## Pré-requisitos (Uma Vez)

### 1. Instalar ESP-IDF

```bash
# Criar diretório ESP
mkdir -p ~/esp
cd ~/esp

# Clonar ESP-IDF (pode demorar alguns minutos)
git clone --recursive https://github.com/espressif/esp-idf.git

# Instalar toolchain ESP32
cd esp-idf
./install.sh esp32
```

### 2. Instalar esptool (para gravar)

```bash
pip3 install esptool
```

**Pronto!** Você só precisa fazer isso uma vez.

---

## Compilar ESP32 (SUPER SIMPLES!)

### Método 1: Script Automático ⭐ (RECOMENDADO)

```bash
cd /caminho/para/corezero

# Compile! (O script faz tudo sozinho)
./build-esp32.sh
```

**Pronto!** O script:
- ✅ Detecta e configura ESP-IDF automaticamente
- ✅ Configura toolchain Xtensa
- ✅ Compila o projeto
- ✅ Cria o binário .bin

### Método 2: Manual (Se quiser controle total)

```bash
# 1. Configurar ambiente ESP-IDF
source ~/esp/esp-idf/export.sh

# 2. Configurar CMake (importante: especificar toolchain!)
cmake -B build \
    -DALLOY_BOARD=esp32_devkit \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xtensa-esp32-elf.cmake

# 3. Compilar
cmake --build build --target rtos_blink_esp32
```

---

## Gravar no ESP32

```bash
# 1. Conectar ESP32 via USB

# 2. Descobrir porta
ls /dev/cu.usbserial-*  # macOS
ls /dev/ttyUSB*         # Linux

# 3. Gravar
esptool.py --chip esp32 \
    --port /dev/cu.usbserial-XXXX \
    --baud 921600 \
    write_flash -z 0x1000 \
    build/examples/rtos_blink_esp32/rtos_blink_esp32.bin

# 4. Monitorar serial
screen /dev/cu.usbserial-XXXX 115200
```

**Ou use o script automático**:
```bash
cd examples/rtos_blink_esp32
./flash_esp32.sh --monitor
```

---

## O Que Você Vai Ver

**LED**: Piscando rápido (200ms)

**Serial Monitor** (115200 baud):
```
[INFO] =================================
[INFO] Alloy RTOS ESP32 Demo
[INFO] =================================
[INFO] Starting RTOS with 3 tasks...
[INFO] Task1 started (High priority)
[INFO] Task2 started (Normal priority)
[INFO] Idle task started
[INFO] Task2: LED on
```

**Para sair do screen**: `Ctrl+A`, depois `K`, depois `Y`

---

## Troubleshooting

### Erro: "unknown argument: '-mtext-section-literals'"

Você esqueceu de especificar o toolchain Xtensa!

**Solução**: Use o script `./build-esp32.sh` OU especifique o toolchain manualmente:
```bash
cmake -B build \
    -DALLOY_BOARD=esp32_devkit \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xtensa-esp32-elf.cmake
```

### Erro: "ESP-IDF not found"

```bash
# Instale ESP-IDF (veja Pré-requisitos acima)

# Ou configure manualmente:
export IDF_PATH=~/esp/esp-idf
source ~/esp/esp-idf/export.sh
```

### Erro: "xtensa toolchain not found"

```bash
# Configure ambiente ESP-IDF:
source ~/esp/esp-idf/export.sh

# Verifique se funcionou:
which xtensa-esp32-elf-gcc
```

### Porta USB não encontrada

**macOS**: Instale driver CP2102
- https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

**Linux**: Adicione permissões
```bash
sudo usermod -a -G dialout $USER
# Faça logout/login
```

---

## Resumo: Como Usar (TL;DR)

```bash
# 1. Primeira vez: instalar ESP-IDF (veja acima)

# 2. Compilar
./build-esp32.sh

# 3. Gravar
cd examples/rtos_blink_esp32
./flash_esp32.sh --monitor
```

**3 comandos e está rodando!** 🎉

---

## Próximos Passos

Funcionou? Agora você pode:

- ✅ Modificar `examples/rtos_blink_esp32/main.cpp`
- ✅ Adicionar novas tasks
- ✅ Experimentar com prioridades
- ✅ Criar seus próprios projetos ESP32!

**Dúvidas?** Leia o [README completo](../examples/rtos_blink_esp32/README.md) ou abra uma issue!
