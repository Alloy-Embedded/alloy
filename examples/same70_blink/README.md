# ATSAME70 LED Blink Example

Minimal bare-metal LED blink example for ATSAME70Q21B (Cortex-M7) on SAME70 Xplained board.

## Quick Start

```bash
# Build
make same70-standalone

# Flash to board
make same70-flash
```

The LED on PC8 should blink with a 2-second ON, 2-second OFF pattern.

## Hardware

- **Board**: ATSAME70 Xplained (ATSAME70Q21B)
- **MCU**: ARM Cortex-M7 @ 300MHz (starts at 12MHz RC oscillator)
- **LED**: Green LED on PC8 (active LOW)
- **Debug Interface**: CMSIS-DAP

## Critical SAME70 Initialization

The SAME70 has **TWO watchdog timers** that MUST both be disabled:

1. **WDT** (Watchdog Timer) at `0x400E1854`
2. **RSWDT** (Reinforced Watchdog Timer) at `0x400E1D54`

```cpp
// CRITICAL: Disable BOTH watchdogs!
WDT_MR = WDT_WDDIS;      // Disable normal watchdog
RSWDT_MR = RSWDT_WDDIS;  // Disable reinforced watchdog
```

**If RSWDT is not disabled, the MCU will reset periodically!**

## Key Initialization Steps

1. **Disable watchdogs** (WDT and RSWDT)
2. **Enable PIOC clock** via PMC (Power Management Controller)
3. **Configure GPIO**:
   - Enable PIO control (`PIOC_PER`)
   - Set as output (`PIOC_OER`)
   - Enable ODSR access (`PIOC_OWER`)
4. **Toggle LED** using `PIOC_SODR` (set) and `PIOC_CODR` (clear)

## Files

- `standalone_blink.cpp` - Minimal working example (196 bytes!)
- `rswdt_fixed_blink.cpp` - Reference implementation showing the fix
- `same70_minimal.ld` - Linker script for SAME70Q21B
- `SOLUTION_FOUND.md` - Complete debugging history and solution
- `DEBUG_SUMMARY.md` - All troubleshooting attempts

## Register Addresses

```c
// Watchdogs
#define WDT_MR       0x400E1854  // Normal Watchdog Mode Register
#define RSWDT_MR     0x400E1D54  // Reinforced Watchdog Mode Register
#define WDDIS        (1U << 15)  // Watchdog Disable bit

// Power Management
#define PMC_PCER0    0x400E0610  // Peripheral Clock Enable Register 0
#define ID_PIOC      12          // PIOC Peripheral ID

// GPIO Port C
#define PIOC_PER     0x400E1200  // PIO Enable Register
#define PIOC_OER     0x400E1210  // Output Enable Register
#define PIOC_OWER    0x400E1220  // Output Write Enable Register
#define PIOC_SODR    0x400E1230  // Set Output Data Register
#define PIOC_CODR    0x400E1234  // Clear Output Data Register
```

## Memory Map

```
Flash (ROM):  0x00400000 - 0x005FFFFF (2MB)
RAM:          0x20400000 - 0x2045FFFF (384KB)
Peripherals:  0x40000000 - 0x5FFFFFFF
```

## Compilation

The example uses:
- **Toolchain**: xPack ARM GCC 14.2.1
- **CPU**: Cortex-M7 with FPU (`-mfpu=fpv5-d16`)
- **Optimization**: `-O1` (balance size/debug)
- **No C++ runtime**: `-fno-exceptions -fno-rtti -nostdlib`

## Troubleshooting

### LED not blinking?

1. **Check RSWDT disable** - Most common issue!
2. **Verify PMC clock** - PIOC clock must be enabled
3. **Check pin** - LED is on PC8 (confirmed in working code)
4. **Active LOW** - LED turns ON when pin is LOW

### Previous issues encountered:

- ❌ Only disabling WDT (forgot RSWDT) → MCU resets
- ❌ Missing PMC clock enable → GPIO not working
- ❌ Wrong timing assumptions → Used 300MHz instead of 12MHz
- ✅ **Solution**: Disable both watchdogs + enable PIOC clock

## Reference

Solution found by comparing with working code from previous repository.

Key discovery: `RSWDT->RSWDT_MR = RSWDT_MR_WDDIS_Msk;`

## License

Part of the Corezero framework.
