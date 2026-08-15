*** Settings ***
Documentation     DMA driver conformance in emulation — completes the peripheral
...               conformance set after i2c_read / spi_read / adc_read, and the one
...               leg every DMA ENGINE alloy supports has to pass. The firmware sends
...               a distinct line over the debug UART BY DMA (memory->peripheral, the
...               m2p path) with the completion delivered as an INTERRUPT, so a green
...               run proves the driver's channel programming, the transfer, and the
...               NVIC handshake actually moved the bytes — no hardware. RESC and UART
...               come from `alloy emulate` via --variable; the CI matrix runs this
...               same case per board.
...
...               THE THREE ENGINES IT RUNS AGAINST, and what each is measured
...               against, because "green" means something different per platform:
...
...               * st/dma_v1 (G0, free router) — Renode's OWN DMA.STM32G0DMA. This
...                 file once claimed Renode had no channel-style model and the
...                 platform shipped a hand-written one; it does have one, its own
...                 stm32g0.repl wires two, and the native model passes this leg. A
...                 model this project writes cannot falsify a misreading of the
...                 reference manual that it shares with the driver, so the stock one
...                 wins wherever it exists.
...               * st/dma_v2 (F4/F7, stream engine) — a GENERATED model, because the
...                 stock DMA.STM32DMA measurably lacks CIRC reload and HTIF (both are
...                 tagged no-ops) and is sealed. See emit/renode.py RENODE_DMA_V2.
...               * microchip/xdmac_v1 (SAME70) — a GENERATED model, because Renode
...                 1.16.1 ships NO XDMAC and no Microchip DMA model AT ALL (measured
...                 three ways; see emit/renode.py RENODE_XDMAC). Not modelling it is
...                 not a neutral option here: Renode returns 0 for reads of unmapped
...                 addresses, so the driver's poll-mode complete() would read GS as
...                 "already finished" and write_dma would return true having moved
...                 nothing — a leg that fails for a reason that looks like a driver
...                 bug.
...
...               WHAT THIS LEG DOES NOT WITNESS, on any of the three: REQUEST
...               ROUTING. On the two ST engines the DMAMUX/CHSEL half is unwitnessed
...               because the platform wire and the firmware route descend from the
...               same board.json statement — deleting the DMAMUX write on G0 and the
...               CHSEL write on F7 both leave their legs green. On SAME70 it is worse
...               and permanently so: PERID is unwitnessed BY CONSTRUCTION, because
...               there is no request wire to consume. UART.SAM_USART exposes no DMA
...               request output at all, so the model triggers the whole microblock on
...               the GE write and has no reason to look at PERID. The claim a green
...               SAME70 run is entitled to make is exactly: these bytes crossed from
...               RAM to the USART by the XDMAC channel the firmware programmed, the
...               XDMAC NVIC line fired, and that channel's handler ran. Never that
...               the routing is correct — only silicon witnesses that half, on this
...               family as on the other two.
...
...               AND THE RING PATH IS NOT HERE AT ALL. alloy::dma::ring on the XDMAC
...               is two linked view-0 descriptors, and the descriptor's memory layout
...               is curated in no file in either repo, so the generated model REFUSES
...               linked-list mode out loud (a warning, and nothing transferred)
...               rather than confirm the driver against a reading it would share with
...               it. The SAME70 ring is host-witnessed only — tests/test_xdmac_v1_ring.cpp
...               — and design §5 says so.
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
    # It is the assertion the whole leg exists for, and the one that needs a
    # model with a real interrupt connection — which is why the SAME70
    # platform could not settle for the Python-peripheral tier its flash
    # controller uses (that class has no IRQ property at all).
    Wait For Line On Uart     dma irq: fired    timeout=30
