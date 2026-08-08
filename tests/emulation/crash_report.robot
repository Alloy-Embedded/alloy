*** Settings ***
Documentation     The crash-report loop end to end, across FIVE boots of one machine:
...               a clean first boot, three firmware-triggered crash/report cycles into
...               safe mode, then a HardFault pended through the NVIC to prove the other
...               trampoline. Every reboot is real: the fault handler stores the stacked
...               frame into .noinit RAM and resets via SYSRESETREQ; the emitted .resc
...               registers `macro reset`, so Renode re-runs LoadELF (flash only — RAM,
...               and with it the crash record, survives) and the next boot reports.
...               The firmware's own trigger is a software-pended NMI (fault::trigger()),
...               NOT an organic fault: Renode never vectors those (a wild jump kills the
...               core with the vector table unread), which stays a silicon-only claim.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
Device Crashes Reports Each Crash And Parks After Three
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    # Boot 1: RAM is fresh, no magic, nothing to report.
    Wait For Line On Uart     no crash on record
    Wait For Line On Uart     about to fault deliberately
    # Boots 2..4: each crash must come back naming a real pc from the stacked
    # frame, and the consecutive counter must count — a stale record would
    # repeat "(1 in a row)" forever.
    Wait For Line On Uart     RECOVERED FROM A FAULT\\s+pc=0x[0-9a-f]{8} lr=0x[0-9a-f]{8} status=0x[0-9a-f]{8} \\(1 in a row\\)    treatAsRegex=true
    Wait For Line On Uart     RECOVERED FROM A FAULT.*\\(2 in a row\\)    treatAsRegex=true
    Wait For Line On Uart     RECOVERED FROM A FAULT.*\\(3 in a row\\)    treatAsRegex=true
    Wait For Line On Uart     three faults in a row
    # The firmware is now parked in safe mode — the NMI self-trigger can no
    # longer run. Pend HardFault (exception 3) through the NVIC: the OTHER
    # naked trampoline must capture the park loop's pc and reset, and the
    # counter must keep counting across the reboot.
    Execute Command           sysbus.nvic SetPendingIRQ 3
    Wait For Line On Uart     RECOVERED FROM A FAULT.*\\(4 in a row\\)    treatAsRegex=true
    Wait For Line On Uart     three faults in a row
