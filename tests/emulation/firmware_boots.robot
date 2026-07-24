*** Settings ***
Documentation     Upgrades NORTH_STAR guard #9 from "compiles" to "executes": boots a
...               built alloy ELF on the data-generated Renode platform and asserts the
...               firmware actually runs by waiting for uart_echo's startup banner on the
...               emulated debug UART. Driven by renode-test; RESC and UART are supplied
...               with --variable by the CI job (which gets them from `alloy emulate`).
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
${BANNER}         alloy uart_echo ready

*** Test Cases ***
Firmware Boots And Prints Its Banner
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     ${BANNER}
