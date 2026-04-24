*** Settings ***
Suite Setup       Setup
Suite Teardown    Teardown
Test Setup        Setup Emulation
Resource          ${COMMON_BOOT_RESOURCE}

*** Variables ***
${EXPECTED_BOOT_BANNER}    alloy renode same70 boot ok
${EXPECTED_BOOT_STAGE}     0x4
${EXPECTED_BOOT_MARKER}    0xA1107001
${EXPECTED_UART_BYTES}     28

*** Keywords ***
Setup Emulation
    Create Boot Smoke Machine    sysbus.usart1    125

*** Test Cases ***
SAME70 boot smoke reaches main and emits UART banner
    Start Emulation
    Wait For Boot Banner    ${EXPECTED_BOOT_BANNER}
    Boot Stage Should Be    ${BOOT_STAGE_ADDR}    ${EXPECTED_BOOT_STAGE}
    Boot Marker Should Be    ${BOOT_MARKER_ADDR}    ${EXPECTED_BOOT_MARKER}
    Double Word Should Be    ${BOOT_UART_BYTES_ADDR}    ${EXPECTED_UART_BYTES}
