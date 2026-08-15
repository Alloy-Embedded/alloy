*** Settings ***
Documentation     SPI driver conformance in emulation — the full-duplex sibling of
...               i2c_read. The platform carries the real STM32 SPI controller; the
...               robot attaches a DummySPISlave, primes it to shift out 0x5A, and the
...               firmware exchanges one byte (0xA5 out on MOSI) and prints what it
...               reads back on MISO. A green run — "spi: 0x5a" — proves the driver's
...               clock / MOSI / MISO path actually talks to a device: it clocked a
...               byte out AND read back the exact value the slave was primed with (a
...               different primed value would print a different byte, so this can't
...               pass by coincidence). No hardware. RESC/UART from `alloy emulate`.
...
...               A THIRD leg is anchor 2.4 of docs/design/dma-streams.md — the same
...               exchange with the CPU out of the data phase, over the PAIR of DMA
...               channels the board assigned to spi.rx/spi.tx. WHAT THIS LEG PROVES,
...               and it is more than phase 3 could claim for a route:
...                 * the CHANNEL/STREAM INDEX — the platform's request wire is
...                   `DMARecieve -> <controller>@<index>`, and moving it by one
...                   leaves the receive buffer all zeros (measured);
...                 * the RX-BEFORE-TX ARMING ORDER that design §1 states as doctrine.
...                   Renode's DmaEngine writes a peripheral destination one unit at a
...                   time, so an m2p block into SPI->DR is N separate slave Transmits
...                   and N request edges — but the WHOLE m2p block runs inside the
...                   write that sets EN. Arm transmit first and all N edges land on a
...                   channel that is not yet enabled: buffer all zeros, leg red;
...                 * CR2.RXDMAEN — the model gates the request edge on it, so a
...                   driver that forgets it moves nothing.
...               WHAT IT DOES NOT PROVE, same boundary as every other leg here: the
...               REQUEST ID half of the route. Renode 1.16.1 models no DMAMUX at all
...               (the G0's DMAMUX window logs as a non-existent peripheral) and CHSEL
...               routes nothing in the generated stream model, and in any case the
...               platform wire and the firmware's route descend from the SAME
...               board.json statement, so they cannot disagree. Only silicon witnesses
...               that half. CR2.TXDMAEN is unwitnessed for the same reason the
...               transmit request wire is absent: m2p self-runs at EN in both models.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
SPI Driver Exchanges A Byte With A Device
    Execute Command           include @${RESC}
    # Attach the device (test fixture, not emitted into the platform) and prime the
    # byte it will shift out on MISO during the next exchange.
    Execute Command           machine LoadPlatformDescriptionFromString "spidev: Mocks.DummySPISlave @ spi1"
    # Queued in transfer order: 0x5A answers the blocking leg's single byte, then
    # de-ad-be answer the three bytes of the interrupt-driven leg.
    Execute Command           sysbus.spi1.spidev EnqueueValue 0x5A
    Execute Command           sysbus.spi1.spidev EnqueueValue 0xDE
    Execute Command           sysbus.spi1.spidev EnqueueValue 0xAD
    Execute Command           sysbus.spi1.spidev EnqueueValue 0xBE
    # Four more for the DMA pair, and then ONE marker byte. The mock answers
    # from a queue that advances once per byte it is clocked, and it keeps no
    # record of what it received — so the marker is the duplex witness: it can
    # only come back if the transmit channel really clocked FOUR cycles into
    # the device, leaving the cursor exactly here. A stub that filled the
    # receive buffer without driving MOSI would answer 0xf3.
    Execute Command           sysbus.spi1.spidev EnqueueValue 0xF0
    Execute Command           sysbus.spi1.spidev EnqueueValue 0xF1
    Execute Command           sysbus.spi1.spidev EnqueueValue 0xF2
    Execute Command           sysbus.spi1.spidev EnqueueValue 0xF3
    Execute Command           sysbus.spi1.spidev EnqueueValue 0x9E
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy spi_read    timeout=30
    Wait For Line On Uart     spi: 0x5a    timeout=30
    # The same exchange, interrupt-driven. Only the first byte is written by the
    # CPU; bytes 2 and 3 are clocked from the SPI ISR, so this pattern appearing
    # at all means the handler ran twice AND put the bytes in the right order.
    Wait For Line On Uart     spi async: 0xdeadbe    timeout=30
    # And the completion callback: printed only when the NVIC line fired and the
    # driver's handler reached the end of the transfer. Renode's SPI.STM32SPI
    # honours the CR2 enable bits — with RXNEIE clear it leaves the line unset —
    # so a driver that forgot to arm the interrupt prints "NOT fired", which does
    # not match, and neither does silence.
    Wait For Line On Uart     spi irq: fired    timeout=30
    # ANCHOR 2.4. Four bytes exchanged with the CPU touching neither DR nor any
    # channel register after the call: `spi.transfer_dma(tx, rx)` claimed both
    # board-assigned channels as one unit. The exact pattern is the four values
    # primed above, in order — a different index on the platform's request wire,
    # a transmit channel armed before the receive one, or a missing RXDMAEN each
    # leave this buffer all zeros, so "0x00000000" (which does not match) is what
    # a broken pair prints. It is also a marker no other leg in this firmware can
    # produce: the polled and interrupt paths print "spi:" and "spi async:".
    Wait For Line On Uart     spi dma: 0xf0f1f2f3    timeout=30
    # The duplex half, read back OUT of the peer rather than asserted about it:
    # one ordinary polled exchange after the DMA one, whose answer is wherever
    # the device's cursor now stands. 0x9e means it stands five past the async
    # leg — four cycles clocked by the transmit channel, then this one.
    Wait For Line On Uart     spi dma peer: 0x9e    timeout=30
