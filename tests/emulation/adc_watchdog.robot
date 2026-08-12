*** Settings ***
Documentation     ADC ANALOG WATCHDOG conformance in emulation, against Renode's OWN
...               Analog.STM32G0_ADC — the same model adc_read.robot uses, and one
...               this project did not write. The fixture is adc_read's: distinct
...               voltages on distinct channels (SetDefaultValue 1650 mV on ch3,
...               3300 mV on ch4), which at the platform's 3.3 V reference convert to
...               2048 and 4095 counts.
...
...               The firmware arms ONE window — [1000, 3000] counts — four times over,
...               and the four printed lines are what discriminates. A sample inside
...               the window must be quiet; the SAME window against a sample outside it
...               must trip; the trip must survive until cleared and not re-raise on an
...               in-window sample afterwards; and once disarmed, the very sample that
...               tripped it must say nothing. A model with a flag stuck high fails
...               line 1, stuck low fails line 2, and one that ignores AWD1EN fails
...               line 4 — so no single degenerate behaviour passes all four.
...
...               WHAT THIS DOES NOT PROVE, stated rather than implied. Renode's
...               STM32G0_ADC is constructed with watchdogCount: 1, so ordinals 1 and 2
...               — which this silicon has (ISR/IER bits 8-9, AWD2TR/AWD3TR,
...               AWD2CR/AWD3CR) and this driver programs — are NOT exercised. Neither
...               is the AWD interrupt: the driver polls the sticky flag and never
...               enables AWDnIE. The disable/re-enable cycle that arming performs IS
...               exercised, because the model's ADEN reads back the enable state and
...               the firmware would hang in the ADRDY poll otherwise. No hardware.
...               RESC and UART come from `alloy emulate` via --variable.
...
...               EXECUTED, and shown to be load-bearing. First run: PASS in 1.61 s
...               against pinned Renode 1.16.1 — fast, so no line was reached by a
...               timeout expiring rather than by the firmware printing it. NEGATIVE
...               CONTROL, run once and not kept: delete `IP::awd1en.set(r())` from
...               `awd_arm<0>` in alloy/hal/adc/st_adc_v2.hpp and this suite FAILS in
...               31.73 s, hanging on the "out-of-window ... TRIPPED" line — which is
...               the one assertion that cannot be satisfied by a watchdog that was
...               never enabled. So the four lines are not printed by a program that
...               would print them anyway.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
ADC Analog Watchdog Trips Only Outside Its Window
    Execute Command           include @${RESC}
    # Per-channel input voltages — a TEST FIXTURE the robot sets at runtime, the
    # same one adc_read.robot uses. SetDefaultValue, not FeedVoltageSampleToChannel:
    # the feed queue's samples land one conversion late, defaults are deterministic.
    # `adc1`, not `adc`: the instance name comes from chip data and the G0B1RE's
    # die has numbered instances where the G071RB's does not, so adc_read.robot's
    # `sysbus.adc` is the SAME model under the other board's name. This leg is the
    # G0B1RE's.
    Execute Command           sysbus.adc1 SetDefaultValue 1650 3
    Execute Command           sysbus.adc1 SetDefaultValue 3300 4
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy adc_watchdog    timeout=30
    # 2048 is inside [1000, 3000]: armed, converted, and silent.
    Wait For Line On Uart     in-window ch3=2048: quiet    timeout=30
    # 4095 is above the high threshold: the same watchdog, re-pointed, trips.
    Wait For Line On Uart     out-of-window ch4=4095: TRIPPED    timeout=30
    # Cleared, and an in-window conversion does not bring it back.
    Wait For Line On Uart     after-clear ch3=2048: quiet    timeout=30
    # Disarmed: the sample that tripped it above no longer does.
    Wait For Line On Uart     after-disarm ch4=4095: quiet    timeout=30
