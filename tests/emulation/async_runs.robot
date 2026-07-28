*** Settings ***
Documentation     Upgrades the async runtime from "compiles" to "actually runs on
...               the core". Boots the async_heartbeat firmware on the data-generated
...               Renode platform and asserts the coroutine executor SCHEDULES
...               REPEATEDLY: after the startup banner it waits for three consecutive
...               "beat N" lines, each emitted only after a `co_await delay(250ms)`
...               resumes. A green run means the executor + timer wheel advance over
...               virtual time on the real ISA, not merely that the firmware booted
...               once. RESC and UART are supplied with --variable by the caller
...               (which gets them from `alloy emulate`).
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
${BANNER}         alloy async_heartbeat ready

*** Test Cases ***
Coroutine Executor Schedules Repeatedly
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     ${BANNER}
    Wait For Line On Uart     beat 1
    Wait For Line On Uart     beat 2
    Wait For Line On Uart     beat 3
