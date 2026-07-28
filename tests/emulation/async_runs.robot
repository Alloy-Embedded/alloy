*** Settings ***
Documentation     Upgrades the async runtime from "compiles" to "actually runs on
...               the core". Boots the async_heartbeat firmware (two coroutines) on
...               the data-generated Renode platform and asserts two runtime
...               properties: (1) the executor SCHEDULES REPEATEDLY — after the banner
...               it waits for three consecutive "beat N" lines, each emitted only
...               after a `co_await delay(250ms)` resumes; and (2) two schedules run
...               CONCURRENTLY on one core — a "tick" (from the second task, 400 ms
...               cadence) still arrives after the three beats. A green run means the
...               executor + timer wheel advance over virtual time on the real ISA and
...               interleave two tasks, not merely that the firmware booted once. RESC
...               and UART are supplied with --variable by the caller (from
...               `alloy emulate`).
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
${BANNER}         alloy async_heartbeat ready

*** Test Cases ***
Coroutine Executor Schedules Two Tasks Concurrently
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    # timeout=30 (virtual seconds) gives generous headroom over the ~0.8 s of
    # firmware time to the first post-beat tick, so a slow emulated systick can't
    # flake the run while a genuinely stuck coroutine still fails fast.
    # Prerequisite for ever making these legs blocking.
    Wait For Line On Uart     ${BANNER}    timeout=30
    # heartbeat resumes repeatedly (skips any interleaved tick lines).
    Wait For Line On Uart     beat 1    timeout=30
    Wait For Line On Uart     beat 2    timeout=30
    Wait For Line On Uart     beat 3    timeout=30
    # ...and the second task is still advancing concurrently: an (unnumbered) tick
    # keeps arriving after three beats. ticker never numbers its lines, so this
    # matches the next tick regardless of exact interleave order.
    Wait For Line On Uart     tick    timeout=30
