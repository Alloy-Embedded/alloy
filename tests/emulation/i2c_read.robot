*** Settings ***
Documentation     I2C driver conformance in emulation: proves the driver actually
...               TALKS to a device on the bus, with no hardware. The platform carries
...               the real STM32 I2C controller; the robot attaches a DummyI2CSlave at
...               0x08 and the firmware does a 1-byte write then a 1-byte read to it,
...               printing "device acked" only if BOTH transfers were acknowledged. A
...               green run proves the driver's START / address / data / ACK / STOP
...               sequence works end-to-end on the emulated controller.
...
...               NOTE a bus SCAN can't be emulated this way: probe() is a zero-length
...               write (NBYTES=0), a valid side-effect-free ACK test on real STM32
...               silicon that Renode's STM32 I2C model does not implement. A real
...               >=1-byte transfer (this test) is what the model supports. RESC and
...               UART come from `alloy emulate` via --variable.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
I2C Driver Talks To A Device
    Execute Command           include @${RESC}
    # Attach the device the firmware expects at 0x08 — a TEST fixture, not emitted
    # into the platform (which carries only real silicon). DummyI2CSlave ACKs its
    # address and returns bytes on read, so both transfers complete.
    Execute Command           machine LoadPlatformDescriptionFromString "probe_target: Mocks.DummyI2CSlave @ i2c1 0x08"
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy i2c_read    timeout=30
    Wait For Line On Uart     device acked    timeout=30
