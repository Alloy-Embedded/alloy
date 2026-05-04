# alloy lite HAL — Driver Tier Reference

The lite HAL (`hal/*/lite.hpp`) provides zero-overhead, header-only, direct-MMIO
drivers that require only the v2.1 flat-struct device artifact.  No descriptor-runtime
dependency, no virtual calls, no heap allocation.

---

## Tier Taxonomy

### Tier 0 — Core-fixed

| Driver | Header | Address |
|--------|--------|---------|
| NVIC   | `hal/nvic/lite.hpp`    | Cortex-M hardcoded (`0xE000E100`) |
| SysTick | `hal/systick/lite.hpp` | Cortex-M hardcoded (`0xE000E010`) |

- Cortex-M core peripherals only; address is fixed by the ARM spec.
- No `PeripheralSpec` required.
- No vendor-scope guard needed (Cortex-M is the vendor).

---

### Tier 1 — Address-template

`template<std::uintptr_t Base>` — caller supplies the peripheral base address.
**These drivers are implicitly STM32-specific.**  Each header documents the supported
family and register layout.  Define `ALLOY_ASSERT_VENDOR_STM32` in your build to
activate a `static_assert` that fires when `ALLOY_DEVICE_VENDOR_STM32` is not defined.

| Driver  | Header               | Supported family / layout |
|---------|----------------------|---------------------------|
| DMA     | `hal/dma/lite.hpp`   | STM32 DMA v1 (F0/F1/F3/G0/G4/L4/WB) |
| DMAMUX  | `hal/dma/lite.hpp`   | STM32 DMAMUX (G0/G4/L4/WB/H7) |
| EXTI    | `hal/exti/lite.hpp`  | STM32 EXTI (F0/F1/F3/G0/G4/L4/WB/H7) |
| Flash   | `hal/flash/lite.hpp` | STM32 Flash interface (F0/F1/F3/G0/G4/L4/WB/H7) |
| CRC     | `hal/crc/lite.hpp`   | STM32 CRC calculation unit |
| RNG     | `hal/rng/lite.hpp`   | STM32 RNG (G0/G4/L4/WB/H7) |
| PWR     | `hal/pwr/lite.hpp`   | STM32 PWR controller (G0/G4/L4/WB/H7) |
| SYSCFG  | `hal/syscfg/lite.hpp`| STM32 SYSCFG / EXTICR routing |
| OPAMP   | `hal/opamp/lite.hpp` | STM32 OPAMP (G4/H7) |
| COMP    | `hal/comp/lite.hpp`  | STM32 COMP (G4/H7) |

**Usage:**
```cpp
#include "hal/dma/lite.hpp"

// STM32G0: DMA1 at 0x40020000
using Dma1 = alloy::hal::dma::lite::controller<0x40020000u>;
Dma1::configure_channel(1u, cfg);
```

---

### Tier 2 — PeripheralSpec-gated

`template<typename P> requires SomeVendorConcept<P>` — P is a namespace from the
v2.1 `peripheral_traits.h` flat-struct.  The vendor concept (`StModernUsart`,
`StSpi`, etc.) verifies `kTemplate` + `kIpVersion` at compile time.

All Tier 2 drivers expose the **device-data bridge**:
```cpp
static constexpr auto irq_number(std::size_t idx = 0u) noexcept -> std::uint32_t;
static constexpr auto irq_count() noexcept -> std::size_t;
```
Values come from `P::kIrqLines[]` and `P::kIrqCount` (v2.1 flat-struct).
Callers no longer need to hard-code IRQ numbers from the reference manual.

| Driver    | Header                  | Concept(s) |
|-----------|-------------------------|------------|
| UART      | `hal/uart/lite.hpp`     | `StModernUsart`, `StLegacyUsart` |
| SPI       | `hal/spi/lite.hpp`      | `StSpi` |
| I2C       | `hal/i2c/lite.hpp`      | `StI2c` |
| GPIO      | `hal/gpio/lite.hpp`     | `StGpio` |
| Timer     | `hal/timer/lite.hpp`    | `StTimer` |
| ADC       | `hal/adc/lite.hpp`      | `StModernAdc`, `StSimpleAdc` |
| DAC       | `hal/dac/lite.hpp`      | `StDac` |
| RTC       | `hal/rtc/lite.hpp`      | `StRtcA`, `StRtcB` |
| Watchdog  | `hal/watchdog/lite.hpp` | `StIwdg`, `StWwdg` |
| LPTIM     | `hal/lptim/lite.hpp`    | `StLptim` |

**Usage:**
```cpp
#include "hal/uart/lite.hpp"
#include "hal/nvic/lite.hpp"

namespace dev = alloy::device::traits;
using Uart1 = alloy::hal::uart::lite::port<dev::usart1>;

dev::peripheral_on<dev::usart1>();          // clock enable
Uart1::configure({.baudrate=115200, .clock_hz=16'000'000u});

// IRQ number comes from generated device data — no manual lookup needed:
alloy::hal::nvic::lite::enable_irq(Uart1::irq_number());
```

---

## Adding a New Vendor (Tier 2 extension guide)

To add support for a new vendor's peripheral (e.g. SAME70 UART):

1. **Add a `consteval` detector** in the driver's `detail` namespace:
   ```cpp
   [[nodiscard]] consteval auto is_sam_uart(const char* tmpl) -> bool {
       return std::string_view{tmpl} == "uart";
   }
   ```

2. **Add a named concept**:
   ```cpp
   template <typename P>
   concept SamUart =
       device::PeripheralSpec<P> &&
       requires { { P::kTemplate } -> std::convertible_to<const char*>; } &&
       detail::is_sam_uart(P::kTemplate);
   ```

3. **Widen the port constraint** (e.g. from `StUsart<P>` to `AnyUart<P>`):
   ```cpp
   template <typename P>
   concept AnyUart = StUsart<P> || SamUart<P>;
   ```

4. **Add an `if constexpr` dispatch branch** in each method:
   ```cpp
   static void configure(const Config& cfg) noexcept {
       if constexpr (StModernUsart<P>) { /* SCI3 path */ }
       else if constexpr (StLegacyUsart<P>) { /* SCI2 path */ }
       else if constexpr (SamUart<P>) { /* SAME70 path */ }
   }
   ```

5. **Keep the same public method surface** so call sites are vendor-portable.

Rules:
- New concepts go in the driver's header, not in `device/concepts.hpp`.
- Vendor-specific methods (e.g. nRF52 DMA start) use `requires NrfUarte<P>` so
  they are compile-errors on other vendors, not silent wrong behavior.
- `irq_number()` / `irq_count()` must remain present and correct on all vendors.

---

## Vendor Guard Macro Reference

| Macro | Value | Effect |
|-------|-------|--------|
| `ALLOY_ASSERT_VENDOR_STM32` | defined | Enable `static_assert` in Tier 1 headers |
| `ALLOY_DEVICE_VENDOR_STM32` | `1` | Declares the project target is an STM32 device |

Both are off by default to avoid breaking cross-vendor CMake configurations.
Define them together in CMakeLists.txt for pure-STM32 projects:
```cmake
target_compile_definitions(my_app PRIVATE
    ALLOY_ASSERT_VENDOR_STM32
    ALLOY_DEVICE_VENDOR_STM32=1
)
```
