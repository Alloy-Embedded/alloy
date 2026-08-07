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
    # Make that slave an ECHO: whatever the master writes is queued as the answer
    # to the next read. DummyI2CSlave has no monitor-level "prime a byte" verb (the
    # SPI mock's EnqueueValue has no I2C counterpart), so this hooks DataReceived
    # from the monitor's Python, the same shape Renode's own Renesas_RA8M1.robot
    # uses. It is what turns the second leg into a VALUE assertion: the bytes read
    # back can only be the bytes the write leg actually put on the bus.
    ${echo}=  Catenate     SEPARATOR=\n
    ...  python
    ...  """
    ...  class EchoI2C:
    ...  ${SPACE*4}def __init__(self, dummy):
    ...  ${SPACE*8}self.dummy = dummy
    ...
    ...  ${SPACE*4}def write(self, data):
    ...  ${SPACE*8}self.dummy.EnqueueResponseBytes(data)
    ...
    ...  def mc_setup_echo_i2c(path):
    ...  ${SPACE*4}dummy = monitor.Machine[path]
    ...  ${SPACE*4}dummy.DataReceived += EchoI2C(dummy).write
    ...  """
    Execute Command           ${echo}
    Execute Command           setup_echo_i2c "sysbus.i2c1.probe_target"
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy i2c_read    timeout=30
    Wait For Line On Uart     device acked    timeout=30
    # The same traffic, INTERRUPT-DRIVEN. de-ad-be were written by the driver's
    # ISR one byte per TXIS and read back by the same ISR one byte per RXNE — the
    # CPU touched neither TXDR nor RXDR — so this pattern appearing at all means
    # the handler ran on both directions AND kept the bytes in order.
    Wait For Line On Uart     i2c async: 0xdeadbe    timeout=30
    # And the completion callback, which only runs from the STOPF branch of that
    # ISR. It needs the platform's `EventInterrupt -> nvic@23` line: with the
    # controller left unwired (what the emitter produced before this leg existed)
    # the same firmware prints "i2c async: timeout" and "i2c irq: NOT fired", and
    # this assertion fails. Verified by running it that way.
    Wait For Line On Uart     i2c irq: fired    timeout=30
