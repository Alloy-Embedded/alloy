*** Settings ***
Documentation     ADC driver conformance in emulation — the sibling of i2c_read /
...               spi_read, but against a PURPOSE-WRITTEN STM32G0 ADC model (Renode
...               ships only an incompatible F1/F4-style one). The platform carries
...               that model, emitted as an inline Python peripheral by emit/renode.py;
...               the firmware runs one blocking conversion of channel 3 and prints the
...               raw counts. The model returns 1000 + channel, so "adc: 1003" proves
...               the driver's enable / calibrate (ADCAL) / channel-select (CHSELR /
...               CCRDY) / start (ADSTART) / poll-EOC / read-DR sequence produced the
...               converted value of the RIGHT channel — a wrong channel would print a
...               different number, so it can't pass by coincidence. No hardware. RESC
...               and UART come from `alloy emulate` via --variable.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
ADC Driver Converts A Channel
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy adc_read    timeout=30
    Wait For Line On Uart     adc: 1003    timeout=30
