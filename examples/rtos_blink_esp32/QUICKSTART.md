# 🚀 ESP32 RTOS Quick Start (5 minutos)

Guia super rápido para rodar o Alloy RTOS no seu ESP32.

## Passo 1: Instalar Ferramentas (Uma vez)

### macOS
```bash
# Instalar esptool para gravar
pip3 install esptool

# Instalar screen para monitor serial
# (já vem instalado no macOS)
```

### Linux (Ubuntu/Debian)
```bash
# Instalar ferramentas
sudo apt-get update
sudo apt-get install -y python3-pip screen

# Instalar esptool
pip3 install esptool

# Adicionar usuário ao grupo dialout (para acesso USB)
sudo usermod -a -G dialout $USER
# IMPORTANTE: Fazer logout e login depois deste comando!
```

## Passo 2: Conectar ESP32

1. Conecte o ESP32 ao computador via USB
2. A placa deve ser reconhecida automaticamente

**Verificar conexão:**
```bash
# macOS
ls /dev/cu.usbserial-*

# Linux
ls /dev/ttyUSB*
```

Se não aparecer nada, pode precisar instalar driver USB:
- **CP2102**: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
- **CH340**: Pesquise "CH340 driver" para seu sistema

## Passo 3: Compilar e Gravar (Método Mais Fácil)

```bash
# Entre no diretório do exemplo
cd examples/rtos_blink_esp32

# Execute o script (detecta porta automaticamente)
./flash_esp32.sh --monitor
```

**Pronto!** O script vai:
1. ✅ Compilar o código
2. ✅ Detectar a porta USB automaticamente
3. ✅ Gravar no ESP32
4. ✅ Abrir monitor serial

### Se o script não funcionar

**Método manual:**

```bash
# 1. Voltar para raiz do projeto
cd /Users/lgili/Documents/01\ -\ Codes/01\ -\ Github/corezero

# 2. Compilar
cmake -B build -DALLOY_BOARD=esp32_devkit
cmake --build build --target rtos_blink_esp32

# 3. Gravar (substitua PORT pela sua porta)
esptool.py --chip esp32 \
    --port /dev/cu.usbserial-XXXX \
    --baud 921600 \
    write_flash -z 0x1000 \
    build/examples/rtos_blink_esp32/rtos_blink_esp32.bin

# 4. Monitorar (substitua PORT)
screen /dev/cu.usbserial-XXXX 115200
```

## O Que Você Vai Ver

### LED
- Pisca rápido e irregular (200ms padrão predominante)
- Mostra que múltiplas tasks estão rodando

### Monitor Serial (apertar RESET no ESP32 se não aparecer nada)
```
I (123) RTOS_BLINK: Alloy RTOS ESP32 Demo
I (125) RTOS_BLINK: Starting RTOS with 3 tasks...
I (130) RTOS_BLINK: Task1 started (High priority)
I (135) RTOS_BLINK: Task2 started (Normal priority)
I (140) RTOS_BLINK: Idle task started
I (500) RTOS_BLINK: Task2: LED on
D (300) RTOS_BLINK: Task1 tick
D (500) RTOS_BLINK: Task1 tick
I (1500) RTOS_BLINK: Task2: LED off
```

## Troubleshooting Rápido

### "Port not found"
```bash
# Verificar se ESP32 está conectado
# Tentar outra porta USB
# Instalar driver USB (CP2102 ou CH340)
```

### "Permission denied" (Linux)
```bash
# Adicionar permissão:
sudo chmod 666 /dev/ttyUSB0

# Ou adicionar usuário ao grupo:
sudo usermod -a -G dialout $USER
# Fazer logout/login
```

### "Failed to connect"
```bash
# Segurar botão BOOT no ESP32 enquanto tenta gravar
# Ou usar baud rate menor:
esptool.py --baud 115200 ...
```

### LED não pisca
- Verifique se seu ESP32 tem LED no GPIO2
- Ou conecte LED externo: GPIO2 → LED → Resistor → GND

### Sair do monitor serial
- **screen**: Aperte `Ctrl+A` depois `K` depois `Y`
- **minicom**: Aperte `Ctrl+A` depois `X`

## Próximos Passos

1. ✅ Funcionou? Parabéns! 🎉
2. Leia o [README.md](README.md) completo para entender o código
3. Modifique `main.cpp` para experimentar
4. Adicione suas próprias tasks

## Especificações do Exemplo

- **Tasks**: 3 (Fast, Slow, Idle)
- **Prioridades**: High, Normal, Idle
- **Memory**: ~2KB RTOS + ~6KB stacks
- **Performance**: Context switch ~10-20µs

## Links Úteis

- 📖 [README Completo](README.md)
- 🔧 [Platform Documentation](../../src/rtos/platform/README.md)
- 📝 [Source Code](main.cpp)

---

**Problemas?** Abra uma issue no GitHub ou consulte o README completo.
