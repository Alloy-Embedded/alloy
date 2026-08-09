*** Settings ***
Documentation     Concurrency-model conformance under emulation, and the source
...               of every number in docs/guide/async.md.
...
...               This leg deliberately does NOT assert any figure. Renode is an
...               instruction interpreter: it charges a fixed slice of virtual
...               time per instruction, so the absolute costs the firmware prints
...               move with the emulator's speed model and pinning them here
...               would turn a Renode upgrade into a red build for no engineering
...               reason. What it asserts are the ORDERING properties the guide
...               claims, which the firmware evaluates in-band and prints as
...               verdicts:
...
...                 irq vectored     — the raw leg really reached a handler. A
...                                    strong <NAME>_IRQHandler is chip-specific
...                                    (vector 22 is TIM17 on STM32G071 but
...                                    CAN1_SCE on an STM32F722), and without
...                                    this line a leg built for the wrong board
...                                    reports a latency of zero and everything
...                                    downstream looks better than reality.
...                 verdict dispatch — alloy::irq dispatch costs MORE than a
...                                    strong handler, and a second handler on
...                                    the same line costs more again. Proves the
...                                    published overhead is a real measurement
...                                    of a real chain walk.
...                 verdict mask     — a 100-iteration interrupts-off section
...                                    delays the ISR by much more than an empty
...                                    one. Proves the "the only thing that
...                                    delays an ISR is a critical section" claim
...                                    is measured, not asserted.
...                 verdict cooperative — a 2 ms `co_await delay` lands within one
...                                    extra tick when alone, and past 4 ms when a
...                                    task runs 5 ms without yielding. This is
...                                    the no-preemption doctrine as a test: if
...                                    the executor ever gained preemption this
...                                    line would go FAIL.
...                 verdict parked   — an empty superstep costs the same with
...                                    eight event-parked tasks as with none, and
...                                    MORE once eight tasks are parked on
...                                    timers. run_once() walks the timer list
...                                    every superstep; the guide quotes what
...                                    that costs, so the ordering is gated.
...
...               The firmware prints its whole measurement block to the UART, so
...               the CI log carries the figures even though nothing gates on
...               them and a reader can diff a run against the guide. RESC and
...               UART come from `alloy emulate` via --variable.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
Interrupts Preempt, The Executor Does Not
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy concurrency_probe    timeout=30
    # The ruler first: every ieq figure downstream is divided by this one, so a
    # run where the calibration block did not measure is not a run at all.
    Wait For Line On Uart     cal nop1000=    timeout=60
    # Order matters: the terminal tester consumes the stream in sequence, so
    # these must appear in the order the firmware prints them.
    Wait For Line On Uart     irq chain2 ran: ok    timeout=120
    Wait For Line On Uart     irq vectored: ok    timeout=120
    Wait For Line On Uart     verdict dispatch: ok    timeout=120
    Wait For Line On Uart     verdict mask: ok    timeout=30
    Wait For Line On Uart     verdict cooperative: ok    timeout=30
    Wait For Line On Uart     verdict parked: ok    timeout=30
    Wait For Line On Uart     concurrency_probe done    timeout=30
