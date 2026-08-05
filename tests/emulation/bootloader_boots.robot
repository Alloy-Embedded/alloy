*** Settings ***
Documentation     The UART update bootloader, proven in emulation — no hardware.
...               RESC_FULL loads the bootloader ELF plus a PACKED update image
...               (`alloy image`) placed at slot A's base; RESC_EMPTY loads only the
...               bootloader. A green run proves the production boot path end-to-end:
...               header+payload CRC verification of the slot (alloy/ota/verify), the
...               newest-valid-slot pick, the Cortex-M handoff (VTOR/MSP/jump in
...               arch/boot_jump) — the APP'S banner printing is the app genuinely
...               running from its slot link address — and, with blank slots, that the
...               bootloader stays recoverable in update mode instead of jumping into
...               erased flash.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
Bootloader Verifies And Boots The Slot Image
    Execute Command           include @${RESC_FULL}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy bootloader    timeout=30
    Wait For Line On Uart     boot slot A    timeout=30
    Wait For Line On Uart     alloy uart_echo ready    timeout=30

Blank Slots Stay In Update Mode
    Execute Command           include @${RESC_EMPTY}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy bootloader    timeout=30
    Wait For Line On Uart     no valid image, waiting for update    timeout=30
