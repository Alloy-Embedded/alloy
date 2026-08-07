*** Settings ***
Documentation     DMA driver conformance in emulation — completes the peripheral
...               conformance set after i2c_read / spi_read / adc_read, against a
...               purpose-written STM32G0 (channel-style) DMA model (Renode ships only
...               the F4 stream-style DMA.STM32DMA). The firmware sends a distinct line
...               over the debug UART BY DMA (memory->TDR, the m2p path). The model
...               copies the bytes CMAR->CPAR through the system bus and sets TCIF, so
...               a green run — "dma via DMA" appearing on the UART — proves the
...               driver's channel config (CPAR/CMAR/CNDTR/CCR) + transfer + completion
...               handshake actually moved the bytes to the UART, no hardware. RESC and
...               UART come from `alloy emulate` via --variable.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
DMA Moves A Buffer To The UART
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy dma_uart    timeout=30
    Wait For Line On Uart     dma via DMA    timeout=30
    # Completion delivered as an INTERRUPT, not polled: this line is only
    # printed when the DMA NVIC line fired and the channel's handler ran.
    # It needs Renode's own DMA model — the inline Python peripheral this
    # platform used to emit had no NVIC connection, so it could not have
    # failed this assertion or passed it.
    Wait For Line On Uart     dma irq: fired    timeout=30
