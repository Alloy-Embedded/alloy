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
    Execute Command           sysbus.spi1.spidev EnqueueValue 0x5A
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy spi_read    timeout=30
    Wait For Line On Uart     spi: 0x5a    timeout=30
