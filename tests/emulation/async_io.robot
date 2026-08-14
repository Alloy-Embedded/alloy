*** Settings ***
Documentation     DRIVER AWAITABLES in emulation — proves a coroutine parked on a
...               peripheral is RESUMED BY THE INTERRUPT, on the real ISA.
...               async_runs.robot already proves the executor + timer wheel run;
...               nothing proved that `co_await` over a DRIVER ever suspends and
...               comes back. The async_io firmware runs one leg per interrupt-driven
...               bus (SPI, I2C, DMA) and prints, for each, the number of times the
...               executor RESUMED that leg's task:
...
...               ${SPACE*2}spi await: resumes=2${SPACE*4}1 start+park, 1 wake from the SPI ISR
...               ${SPACE*2}i2c await: resumes=3${SPACE*4}1 + one wake per transfer (write, read)
...               ${SPACE*2}dma await: resumes=2${SPACE*4}1 + one wake from the DMA channel ISR
...
...               That count is what makes this test non-vacuous. The same firmware
...               written WITHOUT coroutines — start the transfer, spin on busy() —
...               retires its task in ONE resume and prints resumes=1, which does not
...               match. And an interrupt that never fires leaves the task parked, so
...               the leg's bounded pump prints "STUCK" and fails here rather than
...               hanging the run. Both were verified by deliberately breaking the
...               firmware (see the commit message).
...
...               The data lines are asserted too, so the bytes really moved: the SPI
...               slave is primed de-ad-be, the I2C slave echoes what it was written,
...               and "dma await: sent by DMA" is a UART line the CPU never writes to
...               TDR — the DMA engine carries it. RESC/UART from `alloy emulate`.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
${BANNER}         alloy async_io

*** Test Cases ***
Coroutines Resume From Peripheral Interrupts
    Execute Command           include @${RESC}
    # SPI device (a TEST fixture, not emitted into the platform), primed with the
    # three bytes the awaited exchange will shift back on MISO.
    Execute Command           machine LoadPlatformDescriptionFromString "spidev: Mocks.DummySPISlave @ spi1"
    Execute Command           sysbus.spi1.spidev EnqueueValue 0xDE
    Execute Command           sysbus.spi1.spidev EnqueueValue 0xAD
    Execute Command           sysbus.spi1.spidev EnqueueValue 0xBE
    # I2C device at 0x08, turned into an ECHO from the monitor's Python (the SPI
    # mock's EnqueueValue has no I2C counterpart), so the bytes the read await
    # returns can only be the bytes the write await put on the bus.
    Execute Command           machine LoadPlatformDescriptionFromString "probe_target: Mocks.DummyI2CSlave @ i2c1 0x08"
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
    Wait For Line On Uart     ${BANNER}    timeout=30
    # SPI: the whole three-byte exchange behind ONE co_await. The pattern proves
    # the ISR clocked bytes 2 and 3; resumes=2 proves the task was suspended
    # while it did and woken by the completion interrupt.
    Wait For Line On Uart     spi await: 0xdeadbe    timeout=30
    Wait For Line On Uart     spi await: resumes=2    timeout=30
    # I2C: a write await and a read await, one STOP interrupt each -> 3 resumes.
    Wait For Line On Uart     i2c await: 0xdeadbe    timeout=30
    Wait For Line On Uart     i2c await: resumes=3    timeout=30
    # ADDITION (dma-streams phase 2, anchor §2.3): the route-claim marker.
    # The firmware claims its TX channel FROM THE BOARD'S debug_uart.tx route
    # (alloy::dma::claim(board::dma::debug_uart_tx) — the generated fact, not
    # a channel literal) and prints this line only on that path; the explicit
    # channel<Dma, N> escape hatch cannot print it. Every assertion below is
    # unchanged.
    Wait For Line On Uart     dma route: claimed board debug_uart.tx    timeout=30
    # DMA: this line is carried by the DMA engine (memory -> USART TDR), so it
    # cannot appear unless the transfer ran; resumes=2 says the task slept
    # through it and the channel's completion interrupt woke it.
    Wait For Line On Uart     dma await: sent by DMA    timeout=30
    Wait For Line On Uart     dma await: resumes=2    timeout=30
    Wait For Line On Uart     async io: done    timeout=30
