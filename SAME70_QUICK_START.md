# SAME70 Xplained - Quick Start Guide

Guia rápido para compilar e gravar o exemplo de blink LED no SAME70 Xplained Ultra.

## 🚀 Início Rápido (3 passos)

### 1. Build
```bash
make same70-build
```

### 2. Flash
```bash
make same70-flash
```

### 3. Veja o LED piscando! ✨

O LED0 (verde) começará a piscar a 1 Hz!

---

## 📋 Pré-requisitos

### 1. ARM GCC Toolchain (xPack)

**Instalação automática:**
```bash
./scripts/install-xpack-toolchain.sh
```

**Ou instalação manual:**
```bash
brew install --cask gcc-arm-embedded
```

### 2. OpenOCD (para flash)

```bash
brew install openocd
```

### 3. Ninja Build System

```bash
brew install ninja
```

---

## 🔨 Comandos do Makefile

### Build

```bash
# Build completo
make same70-build

# Rebuild (clean + build)
make same70-rebuild

# Clean
make same70-clean
```

### Flash

```bash
# Build e flash
make same70-flash

# Ou use os aliases
make same70-blink-flash
```

---

## 📂 Estrutura de Arquivos

```
Alloy/
├── boards/same70_xplained/
│   ├── board.hpp              # Board support package
│   ├── ATSAME70Q21.ld         # Linker script
│   └── README.md              # Documentação completa
│
├── examples/same70_xplained_blink/
│   ├── main.cpp               # Código do exemplo
│   ├── CMakeLists.txt         # Build configuration
│   └── README.md              # Instruções detalhadas
│
└── src/hal/vendors/atmel/same70/atsame70q21/
    ├── startup.cpp            # Startup code + vector table
    ├── peripherals.hpp        # Base addresses
    └── register_map.hpp       # Register definitions
```

---

## 💻 Processo de Build

O comando `make same70-build` executa:

1. **Verifica o ARM toolchain**
   - Procura em `~/Library/xPacks/@xpack-dev-tools/arm-none-eabi-gcc/`
   - Se não encontrar, mostra instruções de instalação

2. **Configura o CMake**
   ```bash
   cmake -B build-same70 \
       -DALLOY_BOARD=same70_xpld \
       -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
       -DLINKER_SCRIPT=boards/same70_xplained/ATSAME70Q21.ld
   ```

3. **Compila**
   - Compila `main.cpp`
   - Compila `startup.cpp` (vector table + Reset_Handler)
   - Linka tudo junto
   - Gera `.elf`, `.hex` e `.bin`

4. **Mostra informações**
   - Tamanho dos arquivos
   - Uso de memória (Flash/RAM)

---

## 📥 Processo de Flash

O comando `make same70-flash` executa:

1. **Build** (se necessário)

2. **Verifica OpenOCD**
   - Se não estiver instalado, mostra instrução

3. **Conecta ao board**
   - Usa config `board/atmel_same70_xplained.cfg`
   - Conecta via EDBG (debugger on-board)

4. **Programa**
   - Apaga flash
   - Grava o `.elf`
   - Verifica
   - Reset do MCU

5. **Confirmação**
   - Mostra mensagem de sucesso
   - LED deve começar a piscar imediatamente!

---

## 🎯 Saída do Build

### Build Completo

```
========================================
Building SAME70 Xplained Example
========================================

Checking ARM toolchain...
✓ ARM GCC found: arm-none-eabi-gcc (xPack GNU Arm Embedded GCC x86_64) 14.2.1

Configuring CMake for SAME70...
[CMake output...]

Building...
[Ninja output...]

✨ Build complete!

Output files:
-rwxr-xr-x  12K same70_xplained_blink.elf
-rw-r--r--  2.4K same70_xplained_blink.hex
-rwxr-xr-x  2.1K same70_xplained_blink.bin

Memory usage:
   text    data     bss     dec     hex filename
   2048     256     128    2432     980 same70_xplained_blink.elf
```

### Flash Completo

```
========================================
Flashing SAME70 Xplained
========================================

Connecting to board...
Open On-Chip Debugger 0.12.0
Info : CMSIS-DAP: SWD supported
Info : CMSIS-DAP: Atomic commands supported
Info : CMSIS-DAP: FW Version = 01.00.0011
...
** Programming Started **
** Programming Finished **
** Verify Started **
** Verified OK **
** Resetting Target **

✅ Flash complete! LED should be blinking now.
```

---

## 🔧 Troubleshooting

### Build Errors

**❌ ARM toolchain not found**
```
Solution: Run ./scripts/install-xpack-toolchain.sh
```

**❌ Ninja not found**
```
Solution: brew install ninja
```

**❌ Linker errors**
```
Solution: Check that ATSAME70Q21.ld exists in boards/same70_xplained/
```

### Flash Errors

**❌ OpenOCD not found**
```
Solution: brew install openocd
```

**❌ No device found**
```
Checklist:
- USB cable conectado na porta DEBUG (não na porta TARGET)
- Board ligado (LED power aceso)
- Drivers instalados (macOS: automático)
```

**❌ Permission denied**
```
Solution (Linux): sudo usermod -a -G dialout $USER
Logout and login again
```

**❌ Flash protection error**
```
Checklist:
- Verificar se todos os jumpers estão no lugar
- Tentar desconectar e reconectar USB
```

### Runtime Issues

**❌ LED não pisca após gravar (placa nova ou após chip erase)**
```
Causa: GPNVM1 não está configurado (boot do ROM ao invés da Flash)

Solução:
1. Desconecte o cabo da porta DEBUG
2. Conecte o cabo na porta TARGET
3. Execute: ./scripts/same70-set-boot-flash.sh
4. Reconecte na porta DEBUG e grave: make same70-flash

NOTA: Isso só precisa ser feito UMA VEZ por placa!
```

**❌ LED não pisca (GPNVM1 já configurado)**
```
Checklist:
1. Verificar se o flash foi bem sucedido
2. Pressionar botão RESET no board
3. Verificar se todos os jumpers estão no lugar
4. Re-flash: make same70-clean same70-flash
```

### GPNVM1 (Boot Configuration)

O SAME70 usa GPNVM1 para controlar o boot:
- **GPNVM1 = 0**: Boot do ROM (SAM-BA bootloader) - LED não pisca
- **GPNVM1 = 1**: Boot da Flash (seu programa) - LED pisca!

**Quando configurar GPNVM1?**
- Placa nova (primeira vez)
- Após chip erase completo
- Se LED não piscar após gravar

**Como configurar:**
```bash
./scripts/same70-set-boot-flash.sh
```

---

## 📊 Uso de Memória

### Typical Usage

| Component | Flash | RAM |
|-----------|-------|-----|
| Startup code | 512 B | 0 B |
| Main application | 1.5 KB | 256 B |
| Stack | 0 B | 8 KB |
| **Total** | **~2 KB** | **~8.3 KB** |

### Limites do SAME70Q21

- Flash: 2 MB (2048 KB)
- RAM: 384 KB
- **Uso atual**: 0.1% Flash, 2% RAM

Muito espaço para crescer! 🚀

---

## 🎓 Próximos Passos

Depois de ter o blink funcionando:

1. **Explore o Board Support Package**
   - Veja `boards/same70_xplained/README.md`
   - Teste outros periféricos (UART, I2C, SPI, ADC)

2. **Modifique o código**
   - Mude a velocidade do blink
   - Adicione o segundo LED
   - Use botões para controlar

3. **Adicione UART debug**
   ```cpp
   board::uart0.init(115200);
   board::uart0.write("Hello, SAME70!\r\n");
   ```

4. **Experimente outros exemplos**
   - `examples/same70_board_test/` - Teste completo
   - `examples/same70_uart_hello/` - UART demo
   - `examples/same70_blink_leds/` - Multi-LED

---

## 📚 Documentação Adicional

- [Board Support README](boards/same70_xplained/README.md)
- [Example README](examples/same70_xplained_blink/README.md)
- [SAME70 Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/SAM-E70-S70-V70-V71-Family-Data-Sheet-DS60001527D.pdf)
- [SAME70 Xplained User Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-44050-Cortex-M7-Microcontroller-SAM-E70-SAME70-Datasheet.pdf)

---

## ⚡ Comandos Rápidos (Cheat Sheet)

```bash
# Build e flash (tudo de uma vez)
make same70-flash

# Apenas build
make same70-build

# Rebuild do zero
make same70-rebuild

# Limpar build
make same70-clean

# Ver help do Makefile
make help
```

---

## 🐛 Debug com GDB

Para debug com GDB:

**Terminal 1: OpenOCD**
```bash
openocd -f board/atmel_same70_xplained.cfg
```

**Terminal 2: GDB**
```bash
arm-none-eabi-gdb build-same70/examples/same70_xplained_blink/same70_xplained_blink.elf

(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) break main
(gdb) continue
```

---

## ✅ Checklist de Sucesso

- [ ] ARM toolchain instalado e funcionando
- [ ] OpenOCD instalado
- [ ] Board conectado via USB na porta EDBG
- [ ] `make same70-build` executado com sucesso
- [ ] `make same70-flash` executado sem erros
- [ ] LED0 piscando a 1 Hz
- [ ] Pronto para começar a desenvolver! 🎉

---

## 🆘 Precisa de Ajuda?

1. Verifique a seção [Troubleshooting](#troubleshooting)
2. Leia o [README completo](examples/same70_xplained_blink/README.md)
3. Abra uma issue no GitHub

---

**Boa programação! 🚀**
