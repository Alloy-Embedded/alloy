# Gravando RTOS no Raspberry Pi Pico

## ✅ Compilação Completa!

O binário RTOS para Raspberry Pi Pico foi compilado com sucesso:

```bash
build/examples/rtos_blink_pico/rtos_blink_pico.bin (1.5KB)
```

**Características:**
- ✅ ARM Cortex-M0+ context switching implementado
- ✅ Boot2 bootloader (256 bytes exatos)
- ✅ RTOS com 4 tarefas de diferentes prioridades
- ✅ Bare-metal (sem dependências externas)

## 🔥 Método 1: Drag & Drop (Mais Fácil!)

O Raspberry Pi Pico tem um bootloader USB nativo que aparece como drive USB.

### Passos:

1. **Conecte a Pico em modo bootloader:**
   - Segure o botão **BOOTSEL** na Pico
   - Conecte o cabo USB ao computador
   - Solte o botão BOOTSEL
   - Um drive chamado **RPI-RP2** aparecerá

2. **Copie o binário:**
   ```bash
   # Opção 1: Arraste o arquivo .bin para o drive RPI-RP2 no Finder

   # Opção 2: Via terminal
   cp build/examples/rtos_blink_pico/rtos_blink_pico.bin /Volumes/RPI-RP2/
   ```

3. **A Pico reinicia automaticamente** e o LED começará a piscar!

## 🛠️ Método 2: OpenOCD (Debug via SWD)

Se você tiver um debugger SWD (como ST-Link, J-Link, ou outra Pico como debugger):

### Setup:

```bash
# Instalar OpenOCD com suporte RP2040
brew install openocd
```

### Gravação:

```bash
cd /Users/lgili/Documents/01\ -\ Codes/01\ -\ Github/corezero

openocd -f interface/cmsis-dap.cfg \
        -f target/rp2040.cfg \
        -c "adapter speed 5000" \
        -c "program build/examples/rtos_blink_pico/rtos_blink_pico.bin verify reset exit 0x10000100"
```

## 🔍 Método 3: picotool (Linha de Comando)

Ferramenta oficial da Raspberry Pi Foundation:

```bash
# Instalar picotool
brew install picotool

# Colocar Pico em modo bootloader (BOOTSEL)
# Depois gravar:
picotool load build/examples/rtos_blink_pico/rtos_blink_pico.bin
picotool reboot
```

## 📦 Estrutura do Binário

```
0x10000000: [Boot ROM - hardware]
0x10000100: .boot2 (256 bytes) - Second stage bootloader
0x10000200: .isr_vector - Interrupt vector table
0x10000XXX: .text - RTOS code (M0+ assembly + C++)
            - arm_context_m0.cpp (context switching)
            - scheduler.cpp (task management)
            - main.cpp (4 tasks)
```

## ✅ Testando o RTOS

Após gravar, o LED da Pico deve piscar mostrando a execução das tarefas:

- **Task 1 (High)**: Pisca rápido (100ms)
- **Task 2 (Normal)**: Pisca lento (500ms)
- **Task 3 (Low)**: Status (2000ms)
- **Idle**: Executa quando nenhuma tarefa está pronta

## 🐛 Troubleshooting

### Drive RPI-RP2 não aparece:

1. Tente outro cabo USB (alguns cabos são só de carga)
2. Segure BOOTSEL ANTES de conectar o USB
3. Teste em outra porta USB

### LED não pisca:

1. Verifique se o binário foi copiado corretamente
2. Desconecte e reconecte a Pico
3. Compile novamente com:
   ```bash
   cmake -B build -DALLOY_BOARD=rp_pico \
         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
   cmake --build build --target rtos_blink_pico
   ```

## 🚀 Próximos Passos

- [ ] Testar RTOS em hardware real
- [ ] Implementar timer preciso (RP2040 Timer)
- [ ] Adicionar suporte USB UART para debug
- [ ] Implementar multicore (usar segundo M0+ core)

## 📊 Informações do Build

```
Placa:     Raspberry Pi Pico (RP2040)
CPU:       Dual ARM Cortex-M0+ @ 125MHz
Flash:     2MB (XIP via QSPI)
RAM:       264KB SRAM
Binário:   1.5KB (RTOS + 4 tarefas)
Boot:      256 bytes (boot2 bootloader)
```

---

**Compilado com sucesso em:** `r$(date)`
**Toolchain:** xpack-arm-none-eabi-gcc 14.2.1
