# Proposal: Close UART HAL Gaps

## Status
`open` — several fields missing from device IR; some HAL methods present but
not yet connected to traits.

## Problem

The UART HAL has working basics (baud, tx, rx, tx_complete, rx_complete flags)
but is missing several fields that real firmware needs:

| Feature               | Status    | Blocker                          |
|-----------------------|-----------|----------------------------------|
| USART_TIMINGR (I2C)   | absent    | Not in device IR                 |
| Kernel clock mux      | absent    | Requires clock-management HAL    |
| Multiprocessor / LIN  | absent    | Not in device IR                 |
| RS-485 DE pin control | absent    | Not in device IR                 |
| Half-duplex mode      | absent    | HDSEL field not in IR            |
| DMA RX/TX request     | absent    | Requires DMA HAL                 |
| Hardware flow control | absent    | CTS/RTS fields not in IR         |
| Overrun/framing error | partial   | ORE/FE flags present; clear path missing |

Additionally, `uart_handle::read()` blocks in a spin-loop (polling `rx_ready_flag`).
There is no async read or DMA-backed transfer.

## Proposed Solution

### Hardware flow control (CTS/RTS)

```cpp
// src/hal/uart/uart_handle.hpp
auto enable_hardware_flow_control() -> core::Result<void, core::ErrorCode>
{
    if constexpr (!semantic_traits::kCtseField.valid ||
                  !semantic_traits::kRtseField.valid)
        return core::Err(core::ErrorCode::NotSupported);
    // write CTSE + RTSE bits in CR3
}
```

IR fields to add: `cts_enable_field`, `rts_enable_field`.

### Half-duplex mode

```cpp
auto enable_half_duplex() -> core::Result<void, core::ErrorCode>
{
    if constexpr (!semantic_traits::kHdselField.valid)
        return core::Err(core::ErrorCode::NotSupported);
    // write HDSEL in CR3, configure single-wire direction
}
```

IR field: `hdsel_field`.

### RS-485 Driver Enable (DE)

```cpp
auto enable_rs485_de(bool active_high = true) -> core::Result<void, core::ErrorCode>
{
    // write DEM bit in CR3, DEP bit for polarity
}
```

IR fields: `dem_field`, `dep_field`.

### LIN mode

```cpp
auto enable_lin(bool use_long_break = false) -> core::Result<void, core::ErrorCode>
{
    // write LINEN in CR2, LBDL for break detection length
}
auto send_lin_break() -> core::Result<void, core::ErrorCode>;
```

IR fields: `linen_field`, `lbdl_field`, `sbkrq_field`.

### Error flag clearing

```cpp
struct UartErrors {
    bool overrun;
    bool framing;
    bool noise;
    bool parity;
};

auto read_and_clear_errors() -> UartErrors;
```

IR fields (ICR register on STM32): `orecf_field`, `fecf_field`, `necf_field`, `pecf_field`.
Some vendors require reading SR + DR to clear — abstracted via traits.

### DMA-backed async transfer (stub, DMA HAL is separate spec)

```cpp
// Forward declaration; full impl in add-dma-hal spec
template <typename DmaHandle>
auto read_dma(DmaHandle& dma, std::span<std::byte> buf)
    -> core::Result<void, core::ErrorCode>;

template <typename DmaHandle>
auto write_dma(DmaHandle& dma, std::span<const std::byte> buf)
    -> core::Result<void, core::ErrorCode>;
```

### Kernel clock mux

Depends on `add-clock-management-hal` spec.
After that spec lands, `set_baud` auto-queries `peripheral_frequency<P>()`.
No change to `uart_handle` API — just removes the `pclk_hz` argument.

## Impact

- Enables RS-485 industrial bus support (Modbus RTU).
- Enables LIN bus support (automotive body control).
- Enables power-efficient UART in half-duplex (single-wire debug, DALI).
- Unblocks DMA-backed UART for high-throughput logging.
