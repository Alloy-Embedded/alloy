## 1. OpenSpec Baseline

- [x] 1.1 Add runtime-async-model, public-hal-api, and runtime-validation deltas

## 2. Real Driver Path

- [x] 2.1 Choose one canonical async/event path for the first closure (UART+DMA completion)
- [x] 2.2 Implement timeout + completion flow on that path (`runtime::event::completion::wait_for` returns `Err(Timeout)`)
- [x] 2.3 Add an official example showing the recommended usage (`examples/async_uart_timeout/`)

## 3. Validation

- [x] 3.1 Add host-MMIO or equivalent coverage for the chosen path (`tests/unit/test_async_completion_timeout.cpp`)
- [x] 3.2 Keep zero-overhead expectations explicit for the blocking-only path (`scripts/check_blocking_only_path.py` + `tests/compile_tests/test_blocking_only_completion_api.cpp`; `blocking-only-path` release gate)
- [x] 3.3 Add any required hardware or emulation spot-check for the chosen path (`tests/hardware/same70/README.md` Stage 5 `async_uart_timeout` acceptance + CHECKLIST follow-up)

## 4. Low Power Coordination

- [x] 4.1 Prove one supported wake/time coordination path (`tests/host_mmio/scenarios/test_same70_bringup.cpp` — SAME70 deep_sleep + first published wake source)
- [x] 4.2 Document what is supported versus still representative (`docs/RUNTIME_ASYNC_MODEL.md` "Supported vs. representative today")

## 5. Docs

- [x] 5.1 Update `docs/RUNTIME_ASYNC_MODEL.md`
- [x] 5.2 Update cookbook/example references (`docs/COOKBOOK.md` canonical row + Runtime Async Model section + no-`async.hpp` rule)

## 6. Validation

- [x] 6.1 Validate the change with `openspec validate make-async-model-real-and-usable --strict`
