*** Settings ***
Documentation     ADC DMA ring in emulation — the phase-1 anchor (docs/design/
...               dma-streams.md §2.1) against Renode's OWN Analog.STM32G0_ADC +
...               DMA.STM32G0DMA. The robot feeds 1650 mV to channel 3
...               (SetDefaultValue, the adc_read fixture) and the firmware streams
...               continuous conversions into a 256-sample ring by DMA, taking
...               hardware-stable halves. The three asserted lines are take()-order
...               facts: "first: half A" — the HALF interrupt delivered before the
...               FULL one; "then: half B" — the FULL interrupt delivered and the
...               cursor moved to the other half; "wrap: half A" — the ring wrapped
...               and the pattern was written twice. v=2048..2048 on every line pins
...               all 128 samples of each half to round(1650/3300*4095) — an
...               unrouted or unfed channel converts 0, so neither the data path nor
...               the channel routing passes by coincidence.
...
...               WHAT THIS LEG DOES NOT WITNESS, stated plainly:
...               * DMAMUX request ROUTING. Renode models no DMAMUX, and at 1.16.1
...               the ADC's native IDMA hookup cannot reach STM32G0DMA (it is an
...               IGPIOReceiver, not an IDMA), so the platform completes the path
...               with a generated C# bridge that forwards each conversion's
...               request to the channel the BOARD assigned (board.json "dma":
...               adc.conv). The firmware's DMAMUX CCR programming is written but
...               never read back; platform and firmware derive the channel from
...               the same board statement, so a misprogrammed mux would not fail
...               here. Only silicon checks that. (A second generated shim
...               subclasses the ADC to allow word-size DR reads — the model is
...               DoubleWord-only while the DMA engine reads 16-bit like silicon;
...               translation only, no behavior change.)
...               * Once-per-boundary HALF events. The model recomputes HTIF on
...               EVERY second-half sample (dataCount <= half, 1.16.1 source), so
...               the half IRQ re-fires per sample and missed() measures the model,
...               not the consumer — the firmware prints it as a diagnostic and this
...               leg does not assert it. The asserted ORDER (half before full,
...               wrap re-delivery) survives the re-raises.
...               * CONT continuous conversion. The model tags CONT unimplemented;
...               the .resc shims EXTEN into DMAEN-carrying CFGR1 writes so the
...               model's mocked external trigger (externalEventFrequency, 1 kHz)
...               paces the stream — the driver's CONT bit is written but inert.
...               * The CCRDY handshake (shimmed, vacuous) — same as adc_read.
...               Everything else — the channel's circular reload, HTIF/TCIF
...               delivery through the shared-vector OR gates, the per-sample DR
...               copies landing in the right halves, the 12-bit conversion math —
...               comes from Renode's models, which this project did not write.
...               RESC and UART come from `alloy emulate` via --variable.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
# The converter's instance name comes from chip data: the G0B1RE die numbers
# its instances (adc1) where the G071RB's does not (adc) — same model, other
# name. CI passes --variable ADC:sysbus.adc1 for the G0B1RE leg.
${ADC}            sysbus.adc

*** Test Cases ***
ADC Ring Delivers Halves In Half Then Full Order
    Execute Command           include @${RESC}
    # The voltage fixture, set at runtime like adc_read's: channel 3 converts
    # 1650 mV -> 2048 at the platform's 3.3 V reference.
    Execute Command           ${ADC} SetDefaultValue 1650 3
    # The paced sequence logs benign per-tick warnings (e.g. a request landing
    # while the channel tears down); mute both peripherals to errors.
    Execute Command           logLevel 3 ${ADC}
    Execute Command           logLevel 3 sysbus.dma1
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy adc_stream               timeout=30
    Wait For Line On Uart     first: half A v=2048..2048     timeout=30
    Wait For Line On Uart     then: half B v=2048..2048      timeout=30
    Wait For Line On Uart     wrap: half A v=2048..2048      timeout=30
    Wait For Line On Uart     adc stream: done               timeout=30
