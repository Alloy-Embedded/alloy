# Proposal: Close USB HAL Gaps

## Status
`open` — USB HAL compiles but cannot enumerate; endpoint config and DPRAM
allocation are missing.

## Problem

The current USB HAL (`src/hal/usb/`) has:
- Basic `enable()` / `disable()` using device enable field
- Connection detection via VBUS field
- Power-on / D+ pull-up control

Missing, blocking real USB enumeration:

| Feature                     | Status | Blocker                              |
|-----------------------------|--------|--------------------------------------|
| Endpoint 0 config (control) | absent | EP0 register fields not in IR        |
| Non-zero endpoint config    | absent | EPnR fields not in IR                |
| DPRAM allocator             | absent | Alloc logic not implemented          |
| Buffer descriptor table     | absent | BDT base address not in traits       |
| Mode force (device/host/OTG)| absent | USB_BCDR/CMOD fields not in IR      |
| Suspend / resume            | absent | ESOFM / SUSPM fields not in IR       |
| Remote wakeup               | absent | RWUP field not in IR                 |
| Endpoint stall/unstall      | absent | STAT bits not in IR                  |

Without endpoint 0, the USB device stack (including TinyUSB integration) cannot
function.

## Proposed Solution

### Endpoint descriptor model

```cpp
// src/hal/usb/usb_endpoint.hpp
namespace alloy::hal::usb {

enum class EpType : uint8_t { control, bulk, interrupt, isochronous };
enum class EpDir  : uint8_t { out, in, bidir };

struct EndpointConfig {
    uint8_t  ep_num;
    EpType   type;
    EpDir    dir;
    uint16_t max_packet_size;
};

}
```

### DPRAM allocator

STM32 USB uses a packet memory area (PMA / DPRAM) starting at
`USB_PMAADDR`. Each endpoint needs TX and/or RX buffer regions allocated
within this 512- or 1024-byte DPRAM.

```cpp
// src/hal/usb/detail/dpram_allocator.hpp
namespace alloy::hal::usb::detail {

template <uint32_t DpramSize>
class DpramAllocator {
public:
    auto alloc(uint16_t size) -> core::Result<uint16_t, core::ErrorCode>;
    auto reset() -> void;
private:
    uint16_t _top = 0;
};

}
```

### Endpoint configuration API

```cpp
// src/hal/usb/usb_handle.hpp (extensions)
auto configure_endpoint(const EndpointConfig& cfg)
    -> core::Result<void, core::ErrorCode>;

auto stall_endpoint(uint8_t ep_num, EpDir dir) -> core::Result<void, core::ErrorCode>;

auto unstall_endpoint(uint8_t ep_num, EpDir dir) -> core::Result<void, core::ErrorCode>;

auto write_endpoint(uint8_t ep_num, std::span<const std::byte> data)
    -> core::Result<uint16_t, core::ErrorCode>;

auto read_endpoint(uint8_t ep_num, std::span<std::byte> buf)
    -> core::Result<uint16_t, core::ErrorCode>;
```

### IR additions (STM32 USB FS)

Fields to add to STM32G0/F4 IR:
- `ep0r_reg`, `ep1r_reg` ... `ep7r_reg` (EPnR registers)
- `ep_type_field`, `ep_stat_tx_field`, `ep_stat_rx_field`, `ep_addr_field`
- `usb_pma_base_address` (constant in IR)
- `usb_pma_size` (512 or 1024 bytes)
- `btable_field` (buffer descriptor table offset)
- `suspend_field`, `resume_field`, `rwup_field`

### Mode force (OTG targets)

For STM32 with USB OTG (F4, H7):
```cpp
enum class UsbMode : uint8_t { device, host, otg };

auto set_mode(UsbMode mode) -> core::Result<void, core::ErrorCode>;
```

IR field: `usb_mode_field` in `USB_OTG_GCCFG` or `USB_BCDR`.

### TinyUSB backend

Once endpoints work, `src/middleware/tinyusb/alloy_usb_hal.hpp` (from
`add-middleware-integrations` spec) can provide a working `dcd_` port.

## Impact

- Unblocks USB CDC-ACM (virtual serial port) — most requested feature.
- Enables HID devices (keyboard, mouse, joystick).
- Enables USB MSC (mass storage for SDMMC-backed devices).
