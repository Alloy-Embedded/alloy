# Design: Extend CAN Coverage

## Context

CAN HAL today covers init-mode + nominal-timing + FD-enable +
RxFIFO0 interrupt. Descriptor publishes data-rate timing, filter
banks, error counters, test modes, message-RAM layout.

## Goals

1. Match modm-class CAN completeness on every device the descriptor
   declares CAN for.
2. Stay schema-agnostic via `if constexpr` capability gates.
3. Compose with `complete-async-hal`'s pattern.
4. Additive only — existing `enable_fd_operation` etc. keep working.

## Key Decisions

### Decision 1: `CanFrame` as a typed struct

```cpp
struct CanFrame {
    std::uint32_t id;
    bool          extended_id;
    bool          remote_frame;
    bool          fd_format;
    bool          bit_rate_switch;
    std::uint8_t  dlc;       // data length code (0..15)
    std::array<std::byte, 64> data; // FD max payload
};
```

Used by both `transmit` and `receive`. The HAL converts to/from
the device's mailbox layout internally.

### Decision 2: Filters take raw bank index

Until codegen ships `add-can-filter-bank-typed-enum`, the HAL
accepts `std::uint8_t bank` clamped to `kFilterBankCount` from the
descriptor. Out-of-range returns `InvalidArgument`.

### Decision 3: `TestMode` consolidates ST CCCR.MON / TEST + SAM TEST

`Normal`, `BusMonitor`, `LoopbackInternal`, `LoopbackExternal`,
`Restricted` — the cross-vendor superset. Each backend gates per-
mode (e.g. `Restricted` is bxCAN-only on STM32 classic).

### Decision 4: `request_bus_off_recovery` is explicit

The CAN spec requires 128 × 11 recessive bits before bus-off
recovers. The HAL exposes the request as an explicit method
rather than auto-recovering — applications need agency over when
to rejoin (some safety-critical systems require operator
intervention).

## Risks

- **FD timing math is involved.** `set_data_timing` accepts raw
  field values; users compute prescaler / SJW / tseg1 / tseg2 from
  CAN-bit-time formulae per the device datasheet. Documented;
  helper `Bittime::from_baud(baud, kernel_hz, sample_point)` lives
  in a future change.
- **Error counter overflow.** Counts saturate at 256 (CAN spec).
  Documented.

## Migration

No source changes for existing users.
