*** Settings ***
Documentation     Driver conformance in emulation: proves the I2C driver actually
...               TALKS to a device on the bus, with no physical hardware. Boots
...               i2c_scan on the data-generated platform (which now carries the real
...               STM32 I2C controller), attaches a DummyI2CSlave at 0x44 (the SHT31
...               address) as a test fixture, and asserts the scanner discovers it —
...               the "scan:" line contains 0x44. A green run proves the START / addr /
...               ACK path works on the emulated controller, not just that the driver
...               compiled. RESC and UART come from `alloy emulate` via --variable.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
I2C Scanner Finds A Device On The Bus
    Execute Command           include @${RESC}
    # Attach the test fixture — a slave the firmware should discover — to the
    # generated controller. DummyI2CSlave ACKs its own address, so probe(0x44)
    # returns true. This is a TEST fixture, deliberately NOT emitted into the
    # platform (which carries only real silicon).
    Execute Command           machine LoadPlatformDescriptionFromString "sht31: Mocks.DummyI2CSlave @ i2c1 0x44"
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy i2c_scan    timeout=30
    # The scan prints "scan: 0x44 " when the probe ACKs. Match the address as a
    # regex so the surrounding "scan: "/trailing space don't matter.
    Wait For Line On Uart     0x44    treatAsRegex=True    timeout=30
