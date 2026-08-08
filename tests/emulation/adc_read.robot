*** Settings ***
Documentation     ADC driver conformance in emulation — the sibling of i2c_read /
...               spi_read, now against Renode's OWN Analog.STM32G0_ADC (the inline
...               Python model this project used to write is gone from the emitter).
...               The suite feeds DISTINCT per-channel voltages as a test fixture —
...               SetDefaultValue 1650 mV on ch3, 3300 mV on ch4 — and the firmware
...               converts both channels. At the platform's 3.3 V reference the model
...               converts round(mV/3300*4095), so "adc ch3: 2048" + "adc ch4: 4095"
...               prove, against a model this project did not write: enable (ADEN ->
...               ADRDY), start (ADSTART -> EOC), the DR read clearing EOC, the
...               CHANNEL ROUTING (the wrong channel converts a different voltage —
...               an unfed channel prints 0, witnessed), and the 12-bit conversion
...               math itself, which the old 1000+channel model never had. Vacuous
...               under emulation, stated plainly: CCRDY (the model lacks the flag;
...               the generated .resc hooks it into every ISR read) and ADCAL (a
...               tagged no-op that never reads back 1). No hardware. RESC and UART
...               come from `alloy emulate` via --variable.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
ADC Driver Converts The Right Channels
    Execute Command           include @${RESC}
    # Per-channel input voltages — a TEST FIXTURE the robot sets at runtime, like
    # i2c_read's DummyI2CSlave; the platform itself carries only the model. Note
    # SetDefaultValue, not FeedVoltageSampleToChannel: the feed queue's samples
    # land one conversion late (read-then-dequeue), defaults are deterministic.
    Execute Command           sysbus.adc SetDefaultValue 1650 3
    Execute Command           sysbus.adc SetDefaultValue 3300 4
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy adc_read    timeout=30
    Wait For Line On Uart     adc ch3: 2048    timeout=30
    Wait For Line On Uart     adc ch4: 4095    timeout=30
