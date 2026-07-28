*** Settings ***
Documentation     Upgrades the emulation oracle from "boots and prints a banner" to
...               "actually does I/O": boots uart_echo, then INJECTS a line into the
...               emulated UART and asserts the firmware echoes the same bytes back.
...               A green run proves the RX path, the echo loop and the TX path all work
...               end-to-end on the emulated core, not merely that startup ran. RESC and
...               UART are supplied with --variable by the caller (from `alloy emulate`).
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
${BANNER}         alloy uart_echo ready

*** Test Cases ***
Firmware Echoes What It Receives
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     ${BANNER}    timeout=30
    # waitForEcho=false keeps the write and the echo assertion independent: push
    # the bytes, then assert the echoed characters arrive on their own.
    Write Line To Uart        alloy-ping    waitForEcho=false
    # includeUnfinishedLine=true: uart_echo echoes each received byte verbatim but
    # the injected line terminator is not reflected as a NL the tester treats as a
    # line break, so the echoed "alloy-ping" sits in the buffer without completing
    # a line. Match the unfinished buffer directly — the echo is what we're proving.
    # timeout=10 caps the cost of a genuine miss (virtual time runs ~20x slower than
    # real on this core, so a full wait is minutes); a working echo matches at once.
    Wait For Line On Uart     alloy-ping    timeout=10    includeUnfinishedLine=true
