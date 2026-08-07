*** Settings ***
Documentation     Microsecond timebase conformance under emulation. The firmware
...               asserts the two contracts IN-BAND (100k-sample monotonicity
...               sweep across tick boundaries, then a 50 ms sleep measured in
...               µs) and prints a verdict per property — so this leg gates on
...               the firmware's own measurement, not on Renode reproducing
...               sub-millisecond wall time. A torn CVR/tick read prints
...               "monotonic: FAIL"; an interpolation slope error prints
...               "rate: FAIL dt=NNN" with the measured delta. RESC and UART
...               come from `alloy emulate` via --variable.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
Microsecond Clock Is Monotonic And Tracks The Tick
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy time_probe    timeout=30
    Wait For Line On Uart     us monotonic: ok    timeout=60
    Wait For Line On Uart     us rate: ok    timeout=30
