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
    # generated controller, at 0x08: the FIRST address the scanner probes. A
    # DummyI2CSlave ACKs its own address, so if the driver/controller/slave
    # handshake works, "0x08" appears on the very first probe — before the ~60
    # empty addresses that would each burn the driver's poll budget. This is a
    # TEST fixture, deliberately NOT emitted into the platform (only real silicon).
    Execute Command           machine LoadPlatformDescriptionFromString "probe_target: Mocks.DummyI2CSlave @ i2c1 0x08"
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy i2c_scan    timeout=30
    # The scan prints "scan: 0x08 " as soon as the first probe ACKs. Match the
    # address in the still-unfinished line (the newline only comes after the full
    # 0x08..0x77 sweep) so the assertion fires on the ACK, not the sweep's end.
    Wait For Line On Uart     0x08    treatAsRegex=True    includeUnfinishedLine=True    timeout=15
