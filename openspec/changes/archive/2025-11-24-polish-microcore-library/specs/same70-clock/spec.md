# Spec: SAME70 Clock Configuration Fix

## MODIFIED Requirements

### Requirement: SAME70 runs at 300 MHz maximum frequency
SAME70 board MUST configure PLL to achieve 300 MHz core clock from 12 MHz crystal.

#### Scenario: Boot SAME70 Xplained board
```cpp
// boards/same70_xplained/board_config.cpp
void SystemClock::configure() {
    // Configure PLL: 12 MHz crystal → 300 MHz core
    // PLLA: (12 MHz / 1) * 25 = 300 MHz
    configure_pll(
        crystal_freq_hz: 12'000'000,
        pll_mul: 25,
        pll_div: 1,
        target_freq_hz: 300'000'000
    );

    // Wait states for 300 MHz @ 3.3V
    configure_flash_wait_states(6);  // Per datasheet Table 59-50
}
```

**Expected**:
- `SystemCoreClock` variable equals 300'000'000
- Peripheral clocks calculated correctly
- Flash wait states appropriate for frequency

**Rationale**: Board capable of 300 MHz, currently misconfigured at 12 MHz (25x slower)

#### Scenario: Verify blink timing matches expected
```cpp
// examples/blink/main.cpp running on SAME70
void delay_ms(uint32_t ms) {
    // At 300 MHz: expect accurate timing
    auto start = SysTick::get_tick();
    while ((SysTick::get_tick() - start) < ms);
}

int main() {
    led::on();
    delay_ms(1000);  // Should be exactly 1 second
    led::off();
}
```

**Expected**: LED blinks at exactly 1 Hz (measured with oscilloscope or timer)
**Rationale**: Validates clock configuration is correct

### Requirement: Clock configuration documented in comments
PLL configuration MUST include detailed comments explaining calculations.

#### Scenario: Developer reads board_config.cpp
```cpp
/**
 * SAME70 Clock Configuration
 *
 * Crystal: 12 MHz (external)
 * Target: 300 MHz CPU clock
 *
 * PLL Configuration:
 *   Input:  12 MHz / DIVA(1) = 12 MHz
 *   VCO:    12 MHz × MULA(25) = 300 MHz
 *   Output: 300 MHz / DIVA(1) = 300 MHz
 *
 * Peripheral Clocks:
 *   - Master Clock (MCK): 150 MHz (CPU/2)
 *   - USB Clock: 48 MHz (separate UPLL)
 *
 * Flash Wait States: 6 cycles (for 300 MHz @ 3.3V)
 *   See ATSAME70 datasheet Table 59-50
 */
```

**Expected**: Clear explanation of clock tree
**Rationale**: Maintainability and educational value

## ADDED Requirements

### Requirement: Board specifications reflect actual performance
Board listing MUST show 300 MHz not 12 MHz.

#### Scenario: List boards with ucore CLI
```bash
$ ./ucore list boards
...
same70_xplained    ATSAME70Q21B Cortex-M7 300MHz
```

**Expected**: Shows "300MHz" not "12MHz"
**Rationale**: Accurate advertising of capabilities

## REMOVED Requirements

None. This fixes existing misconfiguration.
