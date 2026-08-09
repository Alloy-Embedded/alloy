*** Settings ***
Documentation     The peripheral surface, executing rather than compiling. Boots
...               examples/uart_frame — one portable main.cpp, zero preprocessor —
...               and asserts the lines it prints, which are chosen at COMPILE time
...               by two different mechanisms:
...
...               ${FIFO} comes from DEGREE: a generated number on the instance
...               descriptor (Inst::feat::rx_fifo_depth, 0 = absent) driving an
...               `if constexpr`. The STM32G0's usart_v4 has an 8-byte FIFO and the
...               STM32F7's usart_v3 has none, so the SAME SOURCE must print
...               different lines on the two boards. If the number never reached the
...               image, or reached it as a default zero, both legs print the same
...               thing and one of them fails.
...
...               ${DATABITS} comes from LAYER 2: whether `opts` has a `data_bits`
...               member at all, probed by a `requires` concept. It is the "absence
...               is structural" claim, asserted on real emulated silicon.
...
...               Driven by renode-test; RESC/UART come from `alloy emulate`.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
${BANNER}         alloy uart_frame: this port, described by the surface
${FIFO}           rx-fifo: shallow (fewer than 32 bytes)
${DATABITS}       data-bits: Layer 2 knob (opts.data_bits), programmed by this driver

*** Test Cases ***
The Surface Describes This Port
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     ${BANNER}
    Wait For Line On Uart     ${FIFO}
    Wait For Line On Uart     ${DATABITS}
    Wait For Line On Uart     echoing; type to see bytes come back
