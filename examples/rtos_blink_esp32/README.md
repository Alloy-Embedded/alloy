# Alloy RTOS Blink Example for ESP32

Este exemplo demonstra o Alloy RTOS rodando no ESP32 (arquitetura Xtensa) com múltiplas tasks em diferentes prioridades.

## Hardware Necessário

- **ESP32 DevKit** (qualquer variante: ESP32-WROOM-32, ESP32-WROVER, etc.)
- Cabo USB para conexão
- LED built-in (GPIO2) ou LED externo

## O Que Este Exemplo Faz

O exemplo cria 3 tasks rodando simultaneamente:

1. **Task 1 (High Priority)**: Pisca LED rápido (200ms on/off)
2. **Task 2 (Normal Priority)**: Pisca LED lento (1000ms on/off) com logs
3. **Idle Task (Idle Priority)**: Roda quando nenhuma outra task está pronta

Demonstra:
- ✅ Preemptive multitasking (alta prioridade interrompe baixa prioridade)
- ✅ Context switching entre tasks
- ✅ Task delays com `RTOS::delay()`
- ✅ Integração com ESP-IDF logging

## Opção 1: Compilar com Alloy Build System (Recomendado para Teste Rápido)

### Pré-requisitos

```bash
# 1. Instalar Xtensa toolchain
# No macOS:
brew install xtensa-esp32-elf-gcc

# No Linux:
sudo apt-get install gcc-xtensa-esp32-elf

# 2. Verificar instalação
xtensa-esp32-elf-gcc --version
```

### Compilar

```bash
# 1. Voltar para raiz do projeto
cd /Users/lgili/Documents/01\ -\ Codes/01\ -\ Github/corezero

# 2. Limpar build anterior (se houver)
rm -rf build

# 3. Configurar para ESP32
cmake -B build \
    -DALLOY_BOARD=esp32_devkit \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xtensa-esp32-elf.cmake

# 4. Compilar
cmake --build build --target rtos_blink_esp32

# 5. Verificar binário gerado
ls -lh build/examples/rtos_blink_esp32/rtos_blink_esp32.bin
```

### Gravar no ESP32

```bash
# 1. Conectar ESP32 via USB
# No macOS, normalmente aparece como /dev/cu.usbserial-*
# No Linux, normalmente /dev/ttyUSB0

# 2. Descobrir porta USB
ls /dev/cu.usbserial-* # macOS
ls /dev/ttyUSB*        # Linux

# 3. Gravar usando esptool.py
pip install esptool

esptool.py --chip esp32 \
    --port /dev/cu.usbserial-XXXX \
    --baud 921600 \
    write_flash -z 0x1000 \
    build/examples/rtos_blink_esp32/rtos_blink_esp32.bin

# 4. Monitorar serial (ver logs)
screen /dev/cu.usbserial-XXXX 115200
```

## Opção 2: Usar ESP-IDF (Recomendado para Desenvolvimento)

### Pré-requisitos

```bash
# 1. Instalar ESP-IDF (v4.4 ou superior)
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32

# 2. Configurar ambiente
. ~/esp/esp-idf/export.sh
```

### Criar Projeto ESP-IDF

```bash
# 1. Copiar exemplo para diretório ESP-IDF
cd /Users/lgili/Documents/01\ -\ Codes/01\ -\ Github/corezero
mkdir -p ~/esp/alloy_rtos_example
cp -r examples/rtos_blink_esp32/* ~/esp/alloy_rtos_example/
cp -r src/rtos ~/esp/alloy_rtos_example/components/
cp -r src/hal ~/esp/alloy_rtos_example/components/
cp -r src/core ~/esp/alloy_rtos_example/components/
cp -r boards ~/esp/alloy_rtos_example/components/

# 2. Criar CMakeLists.txt para ESP-IDF
cd ~/esp/alloy_rtos_example
cat > CMakeLists.txt <<'EOF'
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(alloy_rtos_esp32)
EOF

# 3. Configurar projeto
idf.py set-target esp32

# 4. Compilar
idf.py build

# 5. Gravar e monitorar
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

## Opção 3: Compilação Manual Rápida

```bash
# Se você só quer testar rapidamente no hardware

# 1. Instalar esptool
pip install esptool

# 2. Baixar binário pré-compilado (se disponível) ou compilar
cd /Users/lgili/Documents/01\ -\ Codes/01\ -\ Github/corezero
cmake -B build -DALLOY_BOARD=esp32_devkit
cmake --build build --target rtos_blink_esp32

# 3. Gravar direto
esptool.py --chip esp32 \
    --port /dev/cu.usbserial-XXXX \
    --baud 921600 \
    write_flash -z 0x1000 \
    build/examples/rtos_blink_esp32/rtos_blink_esp32.bin

# 4. Conectar ao monitor serial
minicom -D /dev/cu.usbserial-XXXX -b 115200
# ou
screen /dev/cu.usbserial-XXXX 115200
```

## O Que Você Deve Ver

### LED
- LED piscando em padrão irregular (alta prioridade interrompendo baixa)
- Principalmente o padrão rápido (200ms) da task de alta prioridade

### Serial Monitor (115200 baud)
```
I (xxx) RTOS_BLINK: Alloy RTOS ESP32 Demo
I (xxx) RTOS_BLINK: Starting RTOS with 3 tasks...
I (xxx) RTOS_BLINK:   - Task1: High priority, 200ms blink
I (xxx) RTOS_BLINK:   - Task2: Normal priority, 1000ms blink
I (xxx) RTOS_BLINK:   - Idle: Idle priority, runs when others blocked
I (xxx) RTOS_BLINK: Task1 started (High priority)
I (xxx) RTOS_BLINK: Task2 started (Normal priority)
I (xxx) RTOS_BLINK: Idle task started
I (xxx) RTOS_BLINK: Task2: LED on
D (xxx) RTOS_BLINK: Task1 tick
D (xxx) RTOS_BLINK: Task1 tick
I (xxx) RTOS_BLINK: Task2: LED off
```

## Troubleshooting

### Erro: "Port not found"
```bash
# Verificar se driver USB está instalado
# Para ESP32 CP2102:
# macOS: Baixar de https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
# Linux: Normalmente já incluído no kernel

# Verificar portas disponíveis
ls /dev/{tty,cu}.*  # macOS
ls /dev/ttyUSB*     # Linux
```

### Erro: "Failed to connect"
```bash
# 1. Pressionar BOOT enquanto conecta
# 2. Tentar baud rate menor
esptool.py --port /dev/cu.usbserial-XXXX --baud 115200 ...
```

### Erro: "Permission denied"
```bash
# Linux: adicionar usuário ao grupo dialout
sudo usermod -a -G dialout $USER
# Fazer logout e login novamente

# macOS: dar permissão ao terminal
# System Preferences > Security & Privacy > Full Disk Access
```

### LED não pisca
```bash
# Verificar GPIO correto
# ESP32 DevKit normalmente usa GPIO2 para LED built-in
# Alguns boards usam GPIO diferente - verifique esquemático
```

### Crash/Reset contínuo
```bash
# Verificar logs:
idf.py monitor

# Possível stack overflow - aumentar stack size das tasks
# Editar main.cpp:
Task<4096, Priority::High> task1(...);  // aumentar de 2048 para 4096
```

## Modificando o Exemplo

### Mudar velocidade do LED
```cpp
// Em main.cpp, modifique os delays:
void task1_func() {
    while (1) {
        Board::Led::on();
        RTOS::delay(100);  // mudar de 200 para 100 (mais rápido)
        Board::Led::off();
        RTOS::delay(100);
    }
}
```

### Adicionar nova task
```cpp
void my_task_func() {
    while (1) {
        // Seu código aqui
        RTOS::delay(500);
    }
}

Task<2048, Priority::Normal> my_task(my_task_func, "MyTask");
```

### Mudar prioridades
```cpp
// Aumentar prioridade de uma task:
Task<2048, Priority::Highest> fast_task(...);

// Níveis disponíveis:
// Priority::Idle, Low, Normal, High, Higher, Highest, Critical
```

## Desempenho

- **Context Switch Time**: ~10-20µs @ 240MHz
- **Memory Usage**: ~2KB RTOS core + stacks das tasks
- **CPU Usage**: <1% para scheduler overhead

## Próximos Passos

1. ✅ Rode este exemplo básico
2. Adicione mais tasks
3. Experimente com prioridades
4. Implemente comunicação entre tasks (quando IPC estiver pronto)
5. Teste estabilidade (deixe rodando por horas)

## Recursos Adicionais

- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [Alloy RTOS Documentation](../../src/rtos/platform/README.md)
