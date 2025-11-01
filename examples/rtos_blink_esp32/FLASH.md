# 🚀 Como Gravar no ESP32 (Super Simples!)

Já temos um binário pronto para você testar! Siga estes passos:

## Passo 1: Instalar esptool

```bash
pip3 install esptool
```

## Passo 2: Conectar ESP32

Conecte seu ESP32 DevKit ao computador via USB.

**Descobrir a porta:**
```bash
# macOS
ls /dev/cu.usbserial-*
ls /dev/cu.wchusbserial*

# Linux
ls /dev/ttyUSB*
```

Se não aparecer nada, instale o driver USB:
- CP2102: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
- CH340: Pesquise "CH340 driver" para seu sistema

## Passo 3: Gravar!

```bash
cd examples/rtos_blink_esp32

# Use o binário pré-compilado
esptool.py --chip esp32 \
    --port /dev/cu.usbserial-XXXX \
    --baud 921600 \
    write_flash -z 0x1000 \
    ../../build/examples/rtos_blink_esp32/rtos_blink_esp32.bin
```

**Substitua `/dev/cu.usbserial-XXXX` pela porta que você descobriu!**

## Passo 4: Monitorar

```bash
screen /dev/cu.usbserial-XXXX 115200
```

**Apertar RESET no ESP32 se não aparecer nada.**

### Sair do screen

Aperte: `Ctrl+A`, depois `K`, depois `Y`

---

## O Que Você Vai Ver

**LED**: Piscando rápido (~200ms)

**Serial** (se estivesse usando ESP-IDF completo, veria logs):
```
[INFO] Alloy RTOS ESP32 Demo
[INFO] Task1 started (High priority)
[INFO] Task2 started (Normal priority)
```

**NOTA**: Esta é uma versão bare-metal simplificada. Para logs completos e funcionalidade total, use a versão com ESP-IDF (veja README.md).

---

## Recompilar (Avançado)

Para recompilar seu próprio código:

### Opção 1: Bare-Metal (Atual)

```bash
cd ../..

# Configure com ESP toolchain
cmake -B build \
    -DALLOY_BOARD=esp32_devkit \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xtensa-esp32-elf.cmake

# Compile
cmake --build build --target rtos_blink_esp32

# Criar .bin manualmente (se necessário)
~/.espressif/tools/xtensa-esp-elf/*/xtensa-esp-elf/bin/xtensa-esp32-elf-objcopy \
    -O binary \
    build/examples/rtos_blink_esp32/rtos_blink_esp32 \
    build/examples/rtos_blink_esp32/rtos_blink_esp32.bin
```

### Opção 2: Com ESP-IDF (Recomendado para Produção)

Veja o [README completo](README.md) para instruções de integração total com ESP-IDF.

---

## Troubleshooting

### "Permission denied" (Linux)
```bash
sudo chmod 666 /dev/ttyUSB0
# ou
sudo usermod -a -G dialout $USER  # Logout/login depois
```

### "Failed to connect"
- Segurar botão BOOT no ESP32 enquanto grava
- Usar baud rate menor: `--baud 115200`

### LED não pisca
- Verificar se seu ESP32 tem LED no GPIO2
- Conectar LED externo: GPIO2 → LED → Resistor 220Ω → GND

---

**Funcionou? 🎉 Agora modifique `main.cpp` e experimente!**
