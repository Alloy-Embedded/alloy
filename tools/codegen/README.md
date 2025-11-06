# Alloy Code Generator

Sistema de geração automática de código para MCUs a partir de arquivos CMSIS-SVD.

## 🚀 Quick Start

```bash
cd tools/codegen

# Gerar tudo (startup + registers + enums + pins)
./codegen generate

# Ver status
./codegen status

# Help
./codegen --help
```

**Novo!** CLI unificada com geração completa de registros, bit fields e enumerações. Ver [CLI_GUIDE.md](CLI_GUIDE.md) para detalhes.

## Estrutura

```
tools/codegen/
├── cli/                    # CLI principal
│   ├── core/              # Infraestrutura central
│   │   ├── config.py      # Configuração centralizada
│   │   ├── paths.py       # Gerenciamento de caminhos
│   │   ├── logger.py      # Sistema de logs
│   │   ├── progress.py    # Rastreamento de progresso
│   │   └── manifest.py    # Gerenciamento de manifesto
│   ├── parsers/           # Parsers SVD
│   │   └── generic_svd.py # Parser SVD genérico
│   ├── generators/        # Geradores de código
│   │   └── generate_startup.py
│   └── vendors/           # Código específico por vendor
│       ├── st/
│       ├── atmel/
│       ├── raspberrypi/
│       └── espressif/
└── upstream/              # Arquivos SVD externos
```

## Comandos Principais

**Nova CLI Unificada!** Use `./codegen` para tudo:

### 1. Gerar Código

```bash
# Gerar tudo (startup + registers + enums + pins para todos vendors)
./codegen generate

# Apenas startup
./codegen generate --startup

# Apenas registros e bitfields
./codegen generate --registers

# Apenas enumerações
./codegen generate --enums

# Apenas pins
./codegen generate --pins

# Pins para vendor específico
./codegen generate --pins --vendor st

# Com verbose
./codegen generate --verbose

# Modo quiet (mais rápido)
./codegen generate --quiet
```

Aliases: `gen`, `g`
```bash
./codegen gen              # Mesmo que generate
./codegen g --startup      # Atalho
```

**MCUs suportados**:
- ATSAMD21G18A (arduino_zero)
- STM32F103 (bluepill)
- ESP32 (esp32_devkit)
- RP2040 (rp_pico)
- ATSAME70Q21 (same70_xpld)
- ATSAMV71Q21 (samv71_xult)
- STM32F407 (stm32f407vg)
- STM32F746 (stm32f746disco)

### 2. Ver Status

```bash
./codegen status           # Status geral
./codegen vendors          # Info de vendors
```

### 3. Limpar Arquivos

```bash
./codegen clean --stats    # Ver estatísticas
./codegen clean --dry-run  # Simular limpeza
./codegen clean            # Limpar (cuidado!)
```

### 4. Testar Parser

```bash
./codegen test-parser STMicro/STM32F103.svd --verbose
```

### 5. Ver Configuração

```bash
./codegen config --test    # Ver e testar config
```

## Usando o Parser Genérico

```python
from cli.parsers.generic_svd import parse_svd

device = parse_svd(Path("STM32F103.svd"))

print(f"Device: {device.name}")
print(f"Vendor: {device.vendor_normalized}")  # Normalizado!
print(f"Family: {device.family}")              # Auto-detectado!
print(f"Peripherals: {len(device.peripherals)}")
print(f"Interrupts: {len(device.interrupts)}")
```

## Usando a Configuração

```python
from cli.core.config import normalize_vendor, detect_family

vendor = normalize_vendor("STMicroelectronics")  # → "st"
family = detect_family("STM32F103C8")            # → "stm32f1"
```

## Estrutura de Saída

```
src/hal/vendors/{vendor}/{family}/{mcu}/
├── startup.cpp
├── peripherals.hpp
├── pins.hpp
├── gpio.hpp
└── ...
```

## Vendors Suportados

60+ variações incluindo:
- ST Microelectronics
- Microchip/Atmel
- NXP/Freescale
- Nordic Semiconductor
- Texas Instruments
- Silicon Labs
- Espressif
- Raspberry Pi

## Detecção de Família

```python
"STM32F103C8"  → "stm32f1"
"ATSAMD21G18A" → "samd21"
"nRF52840"     → "nrf52"
"ESP32-C3"     → "esp32_c3"
"RP2040"       → "rp2040"
```

## Adicionando Novo Vendor

Edite `cli/core/config.py`:

```python
VENDOR_NAME_MAP = {
    "new vendor inc.": "newvendor",
}

FAMILY_PATTERNS = [
    (re.compile(r'newchip(\d+)'), r'newchip\1', 'newvendor'),
]
```

## Testes

```bash
# Testar parser
python3 cli/parsers/generic_svd.py STMicro/STM32F103.svd -v

# Testar config
python3 -c "from cli.core.config import *; print(detect_family('STM32F407'))"
```

## Documentação

- `REFACTORING_REPORT.md` - Detalhes técnicos
- `ANALYSIS_SUMMARY.md` - Análise completa
- `cli/core/config.py` - Configuração central
- `cli/parsers/generic_svd.py` - Parser genérico

---

**Última atualização**: 2025-11-05
