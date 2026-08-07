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
