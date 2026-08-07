*** Settings ***
Documentation     Pin-interrupt driver conformance in emulation. The platform carries
...               a real GPIO port model and a real EXTI (both emitted from chip data),
...               so a monitor-injected pin edge travels the same path silicon uses:
...               pad -> EXTICR port select -> trigger -> pending -> shared NVIC vector
...               -> alloy::irq chain -> the driver's handler. The firmware never calls
...               is_active(): the only thing it reports is a counter its INTERRUPT
...               handler bumped, so a green run cannot be produced by reading the pin.
...
...               Two edges are injected and exactly ONE must be counted. B1 USER is
...               declared active-low, so on_active() arms the FALLING edge only; the
...               rising edge driven first must be ignored. count=00000001 is therefore
...               three assertions in one — the interrupt fired, the driver honoured the
...               board's polarity, and the handler cleared its write-1-to-clear pending
...               bit (a handler that does not re-enters forever and the count runs away
...               — or Renode simply stops making progress, which is a SPIN, not a pass).
...
...               "pin irq: done" is printed after the report, so a firmware wedged in
...               its own handler fails here even if it managed to print the count.
...               RESC/UART from `alloy emulate`.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
A Pin Edge Raises An Interrupt And Only The Armed Edge Counts
    Execute Command           include @${RESC}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy pin_irq    timeout=30
    # Waiting for "armed" before injecting is load-bearing: an edge delivered
    # before the driver unmasks the line would prove nothing.
    Wait For Line On Uart     pin irq: armed    timeout=30
    # Wrong direction for an active-low button — the driver armed FTSR1 only,
    # so this must not be counted.
    Execute Command           sysbus.gpioc OnGPIO 13 true
    # The press.
    Execute Command           sysbus.gpioc OnGPIO 13 false
    Wait For Line On Uart     pin irq: fired count=00000001    timeout=30
    Wait For Line On Uart     pin irq: done    timeout=30
