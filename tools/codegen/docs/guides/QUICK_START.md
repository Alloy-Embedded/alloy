# Quick Start - Como Usar o Code Generator AGORA

## Status: ✅ PRONTO PARA USO

A refatoração está completa e funcional. Você pode usar os geradores agora!

## Geradores Disponíveis

### 1. Gerar Startup Code

```bash
cd tools/codegen

# Gerar para todos os MCUs com placas configuradas
python3 cli/generators/generate_startup.py

# Gerar com output verbose
python3 cli/generators/generate_startup.py --verbose

# Gerar para SVD específico
python3 cli/generators/generate_startup.py --svd STMicro/STM32F103.svd
```

**Gera**:
- `src/hal/vendors/{vendor}/{family}/{mcu}/startup.cpp`
- `src/hal/vendors/{vendor}/{family}/{mcu}/peripherals.hpp`

**MCUs suportados**:
- ATSAMD21G18A (arduino_zero)
- STM32F103 (bluepill)
- ESP32 (esp32_devkit)
- RP2040 (rp_pico)
- ATSAME70Q21 (same70_xpld)
- ATSAMV71Q21 (samv71_xult)
- STM32F407 (stm32f407vg)
- STM32F746 (stm32f746disco)

### 2. Gerar Pins para STM32

```bash
cd tools/codegen

# Gerar para todas as famílias ST configuradas
python3 cli/vendors/st/generate_all_st_pins.py
```

**Gera** para cada MCU:
- `pins.hpp` - Definições de pinos
- `gpio.hpp` - GPIO config
- `hardware.hpp` - Hardware abstractions
- `traits.hpp` - Type traits
- `pin_functions.hpp` - Funções alternativas

**Famílias suportadas**:
- STM32F0 (F030)
- STM32F1 (F103)
- STM32F4 (F401, F405, F407, F411, F429)
- STM32F7 (F722, F745, F746, F765, F767)

### 3. Gerar Pins para Atmel/Microchip

```bash
cd tools/codegen

# SAMD21
python3 cli/vendors/atmel/generate_samd21_pins.py

# SAME70
python3 cli/vendors/atmel/generate_same70_pins.py
```

**Gera**:
- `pins.hpp`
- `gpio.hpp`
- `hardware.hpp`
- Etc.

### 4. Ver Status

```bash
# Status geral
python3 cli/commands/status.py

# Status de vendors
python3 cli/commands/vendors.py
```

### 5. Limpar Arquivos Gerados

```bash
# Ver estatísticas (sem deletar)
python3 cli/commands/clean.py --stats

# Ver o que seria deletado (dry-run)
python3 cli/commands/clean.py --dry-run

# Validar checksums
python3 cli/commands/clean.py --validate

# Limpar arquivos de um gerador específico
python3 cli/commands/clean.py --generator st_pins

# Limpar tudo (CUIDADO!)
python3 cli/commands/clean.py
```

## Workflow Típico

### Primeira Vez

```bash
cd tools/codegen

# 1. Gerar startup para todos MCUs de placas
python3 cli/generators/generate_startup.py

# 2. Gerar pins para ST
python3 cli/vendors/st/generate_all_st_pins.py

# 3. Gerar pins para Atmel (se usar)
python3 cli/vendors/atmel/generate_samd21_pins.py

# 4. Verificar que arquivos foram criados
ls -la src/hal/vendors/st/stm32f1/stm32f103c8/
```

### Adicionando Novo MCU

**Exemplo**: Adicionar STM32F411

1. **Adicionar à lista de board MCUs** (se for uma placa):
   ```python
   # Editar: cli/core/config.py
   BOARD_MCUS = [
       # ... existing ...
       "STM32F411",  # Adicionar
   ]
   ```

2. **Adicionar configuração de pins** (se for ST):
   ```python
   # Editar: cli/vendors/st/generate_all_st_pins.py
   ST_FAMILY_CONFIGS = {
       # ... existing ...
       "STM32F411": {
           "variants": {
               "STM32F411CE": {"package": "LQFP48", "ports": {...}},
           }
       }
   }
   ```

3. **Gerar**:
   ```bash
   python3 cli/generators/generate_startup.py
   python3 cli/vendors/st/generate_all_st_pins.py
   ```

### Adicionando Novo Vendor

**Exemplo**: Adicionar Nordic nRF52

1. **Adicionar vendor mapping**:
   ```python
   # Editar: cli/core/config.py
   VENDOR_NAME_MAP = {
       # ... existing ...
       "nordic semiconductor": "nordic",
   }
   ```

2. **Adicionar padrão de família** (se necessário):
   ```python
   # Editar: cli/core/config.py
   FAMILY_PATTERNS = [
       # ... existing ...
       (re.compile(r'nrf(\d{2})'), r'nrf\1', 'nordic'),
   ]
   ```

3. **Criar gerador de pins** (opcional):
   ```bash
   mkdir cli/vendors/nordic
   # Copiar template de outro vendor
   cp cli/vendors/atmel/generate_samd21_pins.py \
      cli/vendors/nordic/generate_nrf52_pins.py
   # Adaptar para nRF52
   ```

4. **Testar**:
   ```python
   from cli.core.config import normalize_vendor, detect_family
   
   print(normalize_vendor("Nordic Semiconductor"))  # → "nordic"
   print(detect_family("nRF52840"))  # → "nrf52"
   ```

## Verificando Outputs

### Startup Files

```bash
# Ver startup gerado
cat src/hal/vendors/st/stm32f1/stm32f103c8/startup.cpp

# Verificar vector table
grep -A 10 "vector_table\[\]" src/hal/vendors/st/stm32f1/stm32f103c8/startup.cpp
```

### Pin Headers

```bash
# Ver pins gerados
cat src/hal/vendors/st/stm32f1/stm32f103c8/pins.hpp

# Verificar GPIO
cat src/hal/vendors/st/stm32f1/stm32f103c8/gpio.hpp
```

### Verificar no Projeto

```cpp
// Em seu código
#include "hal/vendors/st/stm32f1/stm32f103c8/gpio.hpp"

using namespace gpio;
using LED = GPIOPin<pins::PC13>;

void setup() {
    LED::configureOutput();
    LED::set();
}
```

## Troubleshooting

### Problema: Vendor não reconhecido

```bash
# Testar normalização
python3 -c "from cli.core.config import normalize_vendor; print(normalize_vendor('Seu Vendor'))"

# Se não reconhecer, adicionar em config.py
```

### Problema: Família não detectada

```bash
# Testar detecção
python3 -c "from cli.core.config import detect_family; print(detect_family('SEUMCU'))"

# Se não detectar, adicionar pattern em config.py
```

### Problema: Arquivos não sendo gerados

```bash
# Rodar com verbose
python3 cli/generators/generate_startup.py --verbose

# Verificar se MCU está na lista
python3 -c "from cli.core.config import is_board_mcu; print(is_board_mcu('SEUMCU'))"
```

### Problema: Erro ao parsear SVD

```bash
# Testar parser diretamente
python3 cli/parsers/generic_svd.py STMicro/STM32F103.svd --verbose

# Ver logs de erro
```

## Testando o Parser Genérico

```bash
# Parse um SVD e veja as informações
cd tools/codegen
python3 cli/parsers/generic_svd.py \
    upstream/cmsis-svd-data/data/STMicro/STM32F103.svd \
    --verbose
```

Output esperado:
```
Successfully parsed STM32F103
  Vendor:        STMicroelectronics (st)
  Family:        stm32f1
  Board MCU:     True
  CPU:           CM3
  FPU:           No
  Peripherals:   45
  Interrupts:    60
```

## Usando Programaticamente

```python
#!/usr/bin/env python3
from pathlib import Path
from cli.core.config import normalize_vendor, detect_family
from cli.parsers.generic_svd import parse_svd
from cli.core.paths import get_mcu_output_dir

# Parse SVD
svd_path = Path("upstream/cmsis-svd-data/data/STMicro/STM32F103.svd")
device = parse_svd(svd_path)

print(f"Device: {device.name}")
print(f"Vendor: {device.vendor_normalized}")
print(f"Family: {device.family}")

# Get output directory
output_dir = get_mcu_output_dir(
    device.vendor_normalized,
    device.family,
    device.name.lower()
)

print(f"Output: {output_dir}")

# Access peripherals
for name, periph in list(device.peripherals.items())[:5]:
    print(f"  {name}: 0x{periph.base_address:08X}")

# Access interrupts
for irq in device.interrupts[:5]:
    print(f"  IRQ {irq.value}: {irq.name}")
```

## Próximos Passos

Depois que tudo estiver funcionando:

1. **Commit os arquivos gerados**:
   ```bash
   git add src/hal/vendors/
   git commit -m "chore: regenerate HAL files with new code generator"
   ```

2. **Testar no projeto real**:
   - Compile exemplos
   - Verifique que funciona
   - Teste GPIO, periféricos, etc.

3. **Contribuir melhorias** (opcional):
   - Adicionar testes
   - Melhorar documentação
   - Adicionar novos MCUs

---

**Documentação completa**: Ver `README.md` no mesmo diretório
**Detalhes técnicos**: Ver `REFACTORING_REPORT.md`
**Status**: Ver `REMAINING_WORK.md`
