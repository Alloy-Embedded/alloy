# Tasks: Middleware Integrations

Host-testable: phases 1–3 (compile tests). Phase 4 requires hardware.

## 1. CMake infrastructure

- [ ] 1.1 Create `src/middleware/` directory tree:
      `lwip/`, `mbedtls/`, `freertos/`, `segger_rtt/`, `tinyusb/`, `canopen/`.
- [ ] 1.2 Add `cmake/middleware.cmake`: defines `alloy::middleware::<stack>` interface
      targets. Each target sets include paths + compile definitions; does NOT
      link the middleware library itself (user supplies that).
- [ ] 1.3 Add CMake guard: each integration target checks `TARGET <middleware>`
      and emits a clear `message(FATAL_ERROR ...)` if the middleware is absent,
      instead of cryptic include errors.
- [ ] 1.4 Add compile test infra: `tests/compile_tests/middleware/CMakeLists.txt`
      that skips gracefully if middleware not available in the test environment.

## 2. lwIP integration

- [ ] 2.1 Create `src/middleware/lwip/alloy_netif.hpp`:
      `init_netif<EthHandle>(...)` + `eth_input<EthHandle>(...)`.
- [ ] 2.2 Implement `alloy_netif_output()` (lwIP → alloy tx path):
      copies PBUF chain to DMA-aligned buffer, calls `eth_handle::send()`.
- [ ] 2.3 Implement `alloy_netif_init()`: set `netif.output`, `netif.linkoutput`,
      MTU=1500, mac address from eth_handle.
- [ ] 2.4 Add compile test: mock `EthHandle`; instantiate `init_netif`.
- [ ] 2.5 Add `examples/lwip_echo_server/` for same70_xplained.

## 3. mbedTLS integration

- [ ] 3.1 Create `src/middleware/mbedtls/alloy_entropy.hpp`:
      `register_entropy<RngHandle>(ctx, rng)`.
- [ ] 3.2 Create `src/middleware/mbedtls/alloy_timing.hpp`:
      `mbedtls_timing_get_timer` implementation backed by SysTick.
- [ ] 3.3 Add compile test: mock `RngHandle`; instantiate `register_entropy`.
- [ ] 3.4 Add `examples/tls_client/` for boards with Ethernet + mbedTLS + lwIP.

## 4. FreeRTOS integration

- [ ] 4.1 Create `src/middleware/freertos/alloy_heap.hpp`:
      `StaticHeap<HeapBytes>` with `install()`.
- [ ] 4.2 Create `src/middleware/freertos/alloy_hooks.hpp`:
      `vApplicationMallocFailedHook`, `vApplicationStackOverflowHook`,
      `vApplicationAssertCalled` — all mapped to alloy panic handler.
- [ ] 4.3 Create `src/middleware/freertos/alloy_tick.hpp`:
      `vApplicationTickHook` → alloy SysTick counter increment.
- [ ] 4.4 Add compile test.
- [ ] 4.5 Add `examples/freertos_blink/` for nucleo_g071rb.

## 5. SEGGER RTT integration

- [ ] 5.1 Create `src/middleware/segger_rtt/alloy_rtt_log.hpp`:
      `RttSink` struct implementing the alloy log sink concept.
- [ ] 5.2 Add compile test: instantiate `RttSink` against mock SEGGER_RTT.h.

## 6. TinyUSB integration

- [ ] 6.1 Create `src/middleware/tinyusb/alloy_usb_hal.hpp`:
      `init<UsbHandle>(usb)` binding alloy USB HAL → tinyusb `dcd_` port.
- [ ] 6.2 Implement `dcd_init`, `dcd_int_handler`, `dcd_edpt_open`,
      `dcd_edpt_xfer` backed by alloy USB semantic traits.
- [ ] 6.3 Add compile test.
- [ ] 6.4 Add `examples/cdc_acm/` for nucleo_f401re (USB FS).

## 7. Documentation

- [ ] 7.1 `docs/MIDDLEWARE.md`: integration guide for each stack.
      Minimum versions, CMake snippet, known limitations.
- [ ] 7.2 `docs/PORTING_NEW_PLATFORM.md`: note which middleware integrations
      are automatically available once HAL modules are ported.
