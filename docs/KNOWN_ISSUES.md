# Known Issues

## SAME70 Platform

### PLL Not Locking (CRITICAL)

**Status:** Under Investigation
**Affects:** ATSAME70Q21B and related devices
**Severity:** High

**Description:**

The PLLA (Phase-Locked Loop A) configuration is not successfully locking when attempting to configure high-speed clocks (144 MHz or 150 MHz). Both external crystal and internal RC oscillator configurations fail to achieve PLL lock.

**Symptoms:**

- `Clock::initialize()` returns `ErrorCode::HardwareError` when using `MasterClockSource::PLLAClock`
- The `PMC_SR.LOCKA` bit never gets set, causing timeout
- System falls back to 12 MHz RC oscillator (no PLL)

**Tested Configurations:**

1. ❌ **Crystal + PLL (150 MHz)**: External 12 MHz crystal → 300 MHz PLLA → 150 MHz MCK
   - Configuration: `CLOCK_CONFIG_150MHZ`
   - MUL=24, DIV=1, Prescaler=DIV_2
   - **Result:** PLL lock timeout

2. ❌ **RC + PLL (144 MHz)**: Internal RC 12 MHz → 288 MHz PLLA → 144 MHz MCK
   - Configuration: `CLOCK_CONFIG_144MHZ_RC`
   - MUL=23, DIV=1, Prescaler=DIV_2
   - **Result:** PLL lock timeout

3. ✅ **RC Simple (12 MHz)**: Internal RC 12 MHz → 12 MHz MCK (no PLL)
   - Configuration: `CLOCK_CONFIG_12MHZ_RC`
   - **Result:** Works correctly

**Current Implementation:**

The PLL configuration follows the Atmel ASF (Advanced Software Framework) approach:
1. Disable PLLA (MULA=0)
2. Configure PLLA with new multiplier/divider values
3. Wait for LOCKA bit in PMC_SR register
4. **ISSUE:** LOCKA bit never gets set

**Debugging Attempts:**

- [x] Verified bitfield definitions match datasheet
- [x] Tried both bitfield API and direct bit manipulation
- [x] Increased timeout from 1M to 10M cycles
- [x] Added delay after disabling PLL
- [x] Verified main clock (crystal/RC) is stable before PLL config
- [x] Tested both external crystal and internal RC as PLL source
- [ ] Check if main clock needs to be selected before PLL init
- [ ] Verify PMC register write permissions/sequence
- [ ] Test with different PLL multiplier values

**Workaround:**

Use the 12 MHz RC oscillator without PLL:

```cpp
auto result = Clock::initialize(CLOCK_CONFIG_12MHZ_RC);
```

**References:**

- SAME70 Datasheet: Section 28 (Power Management Controller)
- Atmel ASF: `sam/drivers/pmc/pmc.c` - `pmc_enable_pllack()`
- File: `src/hal/platform/same70/clock.hpp` (lines 215-239)

**Next Steps:**

1. Review complete PMC initialization sequence from Atmel examples
2. Check if switching to Main Clock source is required before PLL configuration
3. Verify if there are any undocumented register dependencies
4. Test with CMSIS device headers to compare register access patterns
5. Consider using Atmel START or ASF code generation for comparison

---

## ✅ Completed Issues

### 1. Clock Configuration Generator - RESOLVED

**Status:** ✅ Complete (Resolved 2025-11-14)
**Severity:** Medium
**Affects:** SAME70 Platform

**Description:**

O arquivo `clock.hpp` agora é gerado automaticamente a partir de metadata JSON usando template Jinja2.

**Resolution:**

✅ **Template criado:** `tools/codegen/cli/generators/templates/clock.hpp.j2`
✅ **Generator funcionando:** `tools/codegen/cli/generators/platform_generator.py`
✅ **Metadata completo:** `tools/codegen/cli/generators/metadata/platform/same70_clock.json`

**Features Implementadas:**
- ✅ Geração de enums a partir de `additional_enums`
- ✅ Geração de structs a partir de `additional_structs`
- ✅ Geração de configs predefinidos a partir de `predefined_configs`
- ✅ Injeção de implementação custom a partir de `custom_implementations`
- ✅ Geração de métodos a partir de `operations`
- ✅ Uso de endereços gerados: `alloy::generated::atsame70q21b::peripherals::PMC`

**Geração:**
```bash
python3 cli/generators/platform_generator.py --family same70 --peripheral clock --mcu atsame70q21b
```

---

### 2. Magic Numbers Elimination - RESOLVED

**Status:** ✅ Complete (Resolved 2025-11-14)
**Severity:** High
**Affects:** All SAME70 Peripherals

**Description:**

Todos os endereços hardcoded (números mágicos) foram eliminados e substituídos por referências aos endereços gerados automaticamente do `peripherals.hpp`.

**Resolution:**

✅ **Platform Layer (5/5 principais periféricos):**
- `clock.hpp` - Usa `alloy::generated::atsame70q21b::peripherals::PMC`
- `gpio.hpp` - Usa `PIOA/B/C/D/E`
- `i2c.hpp` - Usa `TWIHS0/1/2`
- `spi.hpp` - Usa `SPI0/1`
- `uart.hpp` - Usa `UART0/1/2/3/4`

✅ **Hardware Policies (11/11 hardware policies):**
- `adc_hardware_policy.hpp` - Usa `AFEC0/1`
- `dma_hardware_policy.hpp` - Usa `XDMAC`
- `pwm_hardware_policy.hpp` - Usa `PWM0/1`
- `timer_hardware_policy.hpp` - Usa `TC0/1/2/3`
- + 7 outros hardware policies

**Templates Atualizados:**
- ✅ `platform/*.hpp.j2` - Todos incluem `peripherals.hpp`
- ✅ `hardware_policy.hpp.j2` - Inclui `peripherals.hpp` e usa peripheral_name

**Metadata Atualizado:**
- ✅ Todos os JSONs têm campo `"mcu": "atsame70q21b"`
- ✅ Instâncias têm campo `"peripheral_name"` quando necessário
- ✅ Campo `"examples"` adicionado em adc, dma, pwm, timer

---

### 3. UART Template Duplicate Includes - RESOLVED

**Status:** ✅ Complete (Resolved 2025-11-14)
**Severity:** Medium
**Affects:** UART generation

**Description:**

O template UART tinha 17 includes duplicados do `peripherals.hpp` espalhados pelo arquivo.

**Resolution:**

✅ Removidos todos os includes duplicados
✅ Mantido apenas 1 include no local correto (após bitfields)
✅ UART agora gera corretamente sem duplicações

---

### 2. Startup Code - Documentation Only

**Status:** Complete
**Severity:** Low

**Description:**

O arquivo `startup_same70.cpp` teve apenas mudanças de documentação. O código funcional está correto e não requer mudanças no gerador.

**Files Modified:**
- `src/hal/vendors/atmel/same70/startup_same70.cpp` (linhas 123-134)
- `src/hal/vendors/atmel/same70/startup_config.hpp` (linhas 129-135)

**Changes:**
- ✅ Melhorada documentação do `Reset_Handler()`
- ✅ Adicionado comentário explicando regiões de memória SAME70 (DTCM vs SRAM)

**No Action Required** - Generator já está correto.

---

### 3. Interrupt Handler - Already Correct

**Status:** Complete
**Severity:** N/A

**Description:**

O arquivo `interrupt.hpp` já implementa corretamente `enable_global()` e `disable_global()` usando inline assembly. Não há problemas com o gerador.

**Files:**
- `src/hal/platform/same70/interrupt.hpp` (linhas 46-60)

**Implementation:**
```cpp
static void enable_global() noexcept {
    __asm volatile ("cpsie i" ::: "memory");
}

static void disable_global() noexcept {
    __asm volatile ("cpsid i" ::: "memory");
}
```

**No Action Required** - Implementation is correct.

---

## Summary

### ❌ Active Issues

| Issue | Severity | Status | Impact |
|-------|----------|--------|--------|
| PLL Not Locking | **CRITICAL** | Under Investigation | Cannot use high-speed clocks (>12 MHz) |

### ✅ Completed Issues

| Issue | Severity | Status | Date Resolved |
|-------|----------|--------|---------------|
| Clock generator missing | Medium | ✅ **RESOLVED** | 2025-11-14 |
| Magic numbers in peripherals | High | ✅ **RESOLVED** | 2025-11-14 |
| UART template duplicates | Medium | ✅ **RESOLVED** | 2025-11-14 |
| Peripheral definitions | Medium | ✅ **RESOLVED** | 2025-11-14 |
| SysTick hardware policy | Low | ✅ **RESOLVED** | 2025-11-14 |
| Startup documentation | Low | ✅ **RESOLVED** | Previous |
| Interrupt handler | N/A | ✅ **RESOLVED** | N/A (already correct) |

### 🎯 Current Status

**Generator Infrastructure: 100% Complete**
- ✅ Platform generator: 5/5 main peripherals generating correctly
- ✅ Hardware policy generator: 11/11 policies generating correctly
- ✅ SVD to peripherals.hpp: Full automation working
- ✅ Zero magic numbers: All addresses from generated files

**Remaining Work:**
1. **PLL Configuration** - Critical hardware issue requiring investigation
   - Current workaround: Use 12 MHz RC oscillator without PLL
   - Requires deep dive into PMC register sequence

---

*Last Updated: 2025-11-14*
