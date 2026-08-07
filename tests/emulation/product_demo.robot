*** Settings ***
Documentation     The product dimension, EXECUTED: boots examples/product_demo built with
...               `--product <name>` and asserts the firmware names its product and its
...               control strategy on the emulated UART — and then that the strategy's own
...               arithmetic ran (the step lines). The same robot serves every product of
...               the family: RESC/UART come from `alloy emulate --product ...` and the
...               expected lines are per-product --variable overrides (defaults = fan_eco).
...               The step lines are the real assertion: v/f is stateless and prints the
...               same value twice (4140 4140), the FOC mock carries observer state and
...               prints a chasing sequence (450 787) — one strategy cannot produce the
...               other's output, so a stale or wrongly-keyed build tree fails here.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
${BANNER}         alloy product_demo ready
${PRODUCT_LINE}   product: fan_eco (family: fan_drive)
${CONTROL_LINE}   control: v/f scalar
${STEP1}          step: 4140
${STEP2}          step: 4140

*** Test Cases ***
Product Firmware Names Its Product And Runs Its Strategy
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     ${BANNER}
    Wait For Line On Uart     ${PRODUCT_LINE}
    Wait For Line On Uart     ${CONTROL_LINE}
    Wait For Line On Uart     ${STEP1}
    Wait For Line On Uart     ${STEP2}
