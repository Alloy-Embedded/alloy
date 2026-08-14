*** Settings ***
Documentation     Modbus RTU server conformance in emulation: the robot plays the
...               MASTER on the emulated UART, byte-level, with an INDEPENDENT
...               CRC-16 implementation in monitor Python — so a green run proves
...               the firmware parsed a real request and emitted a byte-exact,
...               CRC-correct response; it cannot pass by the firmware echoing its
...               own table. The example runs at 9600 BAUD DELIBERATELY: t3.5 is
...               4011 µs there, several SysTick periods, so the leg exercises the
...               framer's LOGIC (length prediction + silence resync) without
...               leaning on the emulator reproducing sub-millisecond gaps.
...
...               The negative cases are the point: a CRC-corrupt request and
...               another unit's request get TOTAL SILENCE (the hard half of a
...               server's contract), and the oversized-length sequence
...               11 10 00 00 00 7F FF — which permanently wedged the reference C
...               library this design was measured against (rx buffer pinned at
...               capacity, every later valid frame refused) — must cost exactly
...               one t3.5, after which a valid request IS answered. That case is
...               the regression gate on the whole integration.
...
...               Injection uses uart.WriteChar and capture hooks uart.CharReceived
...               (the same event the terminal tester consumes); each phase runs
...               under `emulation RunFor`, so timing is virtual and deterministic.
...               RESC/UART/BANNER come from `alloy emulate` via --variable.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Keywords ***
Inject And Run
    [Arguments]    ${bytes}    ${virtual_seconds}
    Execute Command           modbus_inject "${UART}" "${bytes}"
    Execute Command           emulation RunFor "${virtual_seconds}"

Expect Response
    [Arguments]    ${bytes}
    ${out}=    Execute Command    modbus_expect "${bytes}"
    Should Contain    ${out}    MODBUS-OK

Expect Silence
    ${out}=    Execute Command    modbus_expect_silence
    Should Contain    ${out}    MODBUS-OK

*** Test Cases ***
Modbus Server Answers A Byte Level Master And Honors The Silences
    Execute Command           include @${RESC}
    # Monitor-Python master: an RX-byte collector on CharReceived plus inject/
    # verify macros. _crc16 here is a SEPARATE implementation of CRC-16/MODBUS
    # (bitwise, no table) — the verdict recomputes every response CRC with it,
    # so firmware and oracle cannot share a table bug.
    ${master}=  Catenate     SEPARATOR=\n
    ...  python
    ...  """
    ...  collected = []
    ...
    ...  def _crc16(data):
    ...  ${SPACE*4}c = 0xFFFF
    ...  ${SPACE*4}for b in data:
    ...  ${SPACE*8}c = c ^ b
    ...  ${SPACE*8}for _ in range(8):
    ...  ${SPACE*12}if c & 1:
    ...  ${SPACE*16}c = (c >> 1) ^ 0xA001
    ...  ${SPACE*12}else:
    ...  ${SPACE*16}c = c >> 1
    ...  ${SPACE*4}return c
    ...
    ...  def mc_modbus_hook(path):
    ...  ${SPACE*4}uart = monitor.Machine[path]
    ...  ${SPACE*4}uart.CharReceived += lambda b: collected.append(int(b))
    ...
    ...  def mc_modbus_clear():
    ...  ${SPACE*4}del collected[:]
    ...
    ...  def mc_modbus_inject(path, hexbytes):
    ...  ${SPACE*4}uart = monitor.Machine[path]
    ...  ${SPACE*4}for tok in hexbytes.split():
    ...  ${SPACE*8}uart.WriteChar(int(tok, 16))
    ...
    ...  def mc_modbus_expect(hexbytes):
    ...  ${SPACE*4}want = [int(t, 16) for t in hexbytes.split()]
    ...  ${SPACE*4}got = list(collected)
    ...  ${SPACE*4}del collected[:]
    ...  ${SPACE*4}if got != want:
    ...  ${SPACE*8}print "MODBUS-FAIL got=%s want=%s" % (got, want)
    ...  ${SPACE*8}return
    ...  ${SPACE*4}if _crc16(got[:-2]) != (got[-2] | (got[-1] << 8)):
    ...  ${SPACE*8}print "MODBUS-FAIL independent crc disagrees"
    ...  ${SPACE*8}return
    ...  ${SPACE*4}print "MODBUS-OK"
    ...
    ...  def mc_modbus_expect_silence():
    ...  ${SPACE*4}got = list(collected)
    ...  ${SPACE*4}del collected[:]
    ...  ${SPACE*4}if len(got) == 0:
    ...  ${SPACE*8}print "MODBUS-OK"
    ...  ${SPACE*4}else:
    ...  ${SPACE*8}print "MODBUS-FAIL unexpected bytes %s" % got
    ...  """
    Execute Command           ${master}
    Execute Command           modbus_hook "${UART}"
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     ${BANNER}    timeout=30
    # ADDITION (dma-streams phase 2, anchor §2.2): the ring-path marker. The
    # firmware prints this line ONLY when RX rides the DMA ring — channel
    # claimed on the board-assigned route, frames drained via readable()/
    # consume(), no per-byte RX interrupt — and the polled fallback cannot
    # print it, so a silent fold back to the old path cannot pass this leg.
    # Every assertion below is UNCHANGED from the polled-path version of this
    # suite: same requests, same byte-exact responses, same silences.
    # (The IDLE frame-gap wake is NOT witnessed here: the pinned USART model
    # does not implement IDLE; under emulation SysTick provides the wakes.
    # What this leg witnesses is the DMA transport and the framing logic.)
    Wait For Line On Uart     modbus: dma ring rx    timeout=10
    # From here on, time is driven explicitly: pause, inject, RunFor — every
    # phase sees the same virtual timeline on every run.
    Execute Command           pause
    Execute Command           modbus_clear

    # 1. A well-formed FC03 read of holding registers 0..1 is answered byte-
    #    exactly: 0x1234, 0xABCD as the example seeds them, CRC verified by
    #    the INDEPENDENT implementation above.
    Inject And Run            11 03 00 00 00 02 C6 9B    0.5
    Expect Response           11 03 04 12 34 AB CD 11 E1

    # 2. The same request with its CRC corrupted: not one byte in reply (the
    #    spec's answer to a bad frame is exactly nothing), and the following
    #    RunFor's virtual silence is the framer's t3.5 resync.
    Inject And Run            11 03 00 00 00 02 C6 9C    0.5
    Expect Silence

    # 3. THE WEDGE REGRESSION. An FC16 claiming 0x7F registers/0xFF bytes
    #    (ADU would be 264 > any buffer) plus trailing garbage — 7 bytes of
    #    this permanently bricked the reference implementation. Here it must
    #    cost one t3.5 of resync and NOTHING more: the next valid request is
    #    answered as if nothing happened.
    Inject And Run            11 10 00 00 00 7F FF DE AD BE EF    0.2
    Expect Silence
    Inject And Run            11 03 00 00 00 02 C6 9B    0.5
    Expect Response           11 03 04 12 34 AB CD 11 E1

    # 4. An address the model does not hold earns the wire exception 0x02 —
    #    a REPLY, not silence: refusals are part of the protocol.
    Inject And Run            11 03 27 0F 00 02 FC 2C    0.5
    Expect Response           11 83 02 C1 34

    # 5. Another unit's conversation (unit 5; this server is 17): total
    #    silence — answering would collide with unit 5's real reply.
    Inject And Run            05 03 00 00 00 01 85 8E    0.5
    Expect Silence
