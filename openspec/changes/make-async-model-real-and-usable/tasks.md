## 1. OpenSpec Baseline

- [x] 1.1 Add runtime-async-model, public-hal-api, and runtime-validation deltas

## 2. Real Driver Path

- [x] 2.1 Choose one canonical async/event path for the first closure (UART+DMA completion)
- [x] 2.2 Implement timeout + completion flow on that path (`runtime::event::completion::wait_for` returns `Err(Timeout)`)
- [x] 2.3 Add an official example showing the recommended usage (`examples/async_uart_timeout/`)

## 3. Validation

- [x] 3.1 Add host-MMIO or equivalent coverage for the chosen path (`tests/unit/test_async_completion_timeout.cpp`)
- [ ] 3.2 Keep zero-overhead expectations explicit for the blocking-only path
- [ ] 3.3 Add any required hardware or emulation spot-check for the chosen path

## 4. Low Power Coordination

- [ ] 4.1 Prove one supported wake/time coordination path
- [ ] 4.2 Document what is supported versus still representative

## 5. Docs

- [x] 5.1 Update `docs/RUNTIME_ASYNC_MODEL.md`
- [ ] 5.2 Update cookbook/example references

## 6. Validation

- [ ] 6.1 Validate the change with `openspec validate make-async-model-real-and-usable --strict`
