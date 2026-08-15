*** Settings ***
Documentation     Bus bridge conformance in emulation: the robot plays the PEER
...               BOARD on the emulated UART with an INDEPENDENT frame
...               implementation in monitor Python — its own CRC-32 (bitwise,
...               no table) and its own encoder — so a green run proves the
...               firmware parsed a real datagram, republished it on its LOCAL
...               bus, and a subscriber-service answered by publishing back
...               through the bridge. The pong carries an INCREMENTING count
...               that only the service's state can produce: a firmware that
...               echoed frames could not pass.
...
...               Framing here is length-driven (bus/wire.hpp): no silence
...               windows, no sub-millisecond timing — nothing leans on the
...               emulator's clock fidelity. The negative cases are the
...               contract's hard half: a CRC-corrupt frame and an unknown
...               message id each earn TOTAL SILENCE and cost exactly one
...               frame — the next ping is answered as if nothing happened.
...
...               The firmware also PUBLISHES telemetry on a timer, which the
...               same bridge forwards, so the link is never idle. Assertions
...               therefore filter by message id: "silence" means the server
...               did not ANSWER, not that no byte moved — telemetry nobody
...               asked for is exactly what a sniffing route is for.
...
...               Injection uses uart.WriteChar and capture hooks
...               uart.CharReceived; each phase runs under `emulation RunFor`,
...               so timing is virtual and deterministic. RESC/UART/BANNER
...               come from `alloy emulate` via --variable.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Keywords ***
Ping And Run
    [Arguments]    ${seq}    ${token}
    Execute Command           bus_ping "${UART}" "${seq}" "${token}"
    Execute Command           emulation RunFor "0.3"

Expect Pong
    [Arguments]    ${token}    ${count}
    ${out}=    Execute Command    bus_expect_pong "${token}" "${count}"
    Should Contain    ${out}    BUS-OK

Expect Silence
    ${out}=    Execute Command    bus_expect_silence
    Should Contain    ${out}    BUS-OK

*** Test Cases ***
Bus Bridge Round Trips Datagrams Through The Local Bus
    Execute Command           include @${RESC}
    # Monitor-Python peer: an RX collector on CharReceived, an independent
    # bitwise CRC-32 (the firmware's is bytewise-branchless — separately
    # written, same spec), and encode/verify macros. The verdict recomputes
    # every received frame's CRC, so firmware and oracle cannot share a bug.
    ${peer}=  Catenate     SEPARATOR=\n
    ...  python
    ...  """
    ...  collected = []
    ...
    ...  def _crc32(data):
    ...  ${SPACE*4}c = 0xFFFFFFFF
    ...  ${SPACE*4}for b in data:
    ...  ${SPACE*8}c = c ^ b
    ...  ${SPACE*8}for _ in range(8):
    ...  ${SPACE*12}if c & 1:
    ...  ${SPACE*16}c = (c >> 1) ^ 0xEDB88320
    ...  ${SPACE*12}else:
    ...  ${SPACE*16}c = c >> 1
    ...  ${SPACE*4}return c ^ 0xFFFFFFFF
    ...
    ...  def _le32(v):
    ...  ${SPACE*4}return [v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, (v >> 24) & 0xFF]
    ...
    ...  def _frame(seq, msg_id, ver, body):
    ...  ${SPACE*4}payload = [msg_id & 0xFF, (msg_id >> 8) & 0xFF, ver] + body
    ...  ${SPACE*4}f = [0x01, seq & 0xFF, len(payload) & 0xFF, (len(payload) >> 8) & 0xFF] + payload
    ...  ${SPACE*4}c = _crc32(f)
    ...  ${SPACE*4}return [0x7E] + f + [c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF, (c >> 24) & 0xFF]
    ...
    ...  def mc_bus_hook(path):
    ...  ${SPACE*4}uart = monitor.Machine[path]
    ...  ${SPACE*4}uart.CharReceived += lambda b: collected.append(int(b))
    ...
    ...  def mc_bus_clear():
    ...  ${SPACE*4}del collected[:]
    ...
    ...  def mc_bus_ping(path, seq, token):
    ...  ${SPACE*4}uart = monitor.Machine[path]
    ...  ${SPACE*4}for b in _frame(int(seq), 0x0301, 1, _le32(int(token, 16))):
    ...  ${SPACE*8}uart.WriteChar(b)
    ...
    ...  def mc_bus_ping_corrupt(path, seq, token):
    ...  ${SPACE*4}uart = monitor.Machine[path]
    ...  ${SPACE*4}f = _frame(int(seq), 0x0301, 1, _le32(int(token, 16)))
    ...  ${SPACE*4}f[-1] = f[-1] ^ 0xFF
    ...  ${SPACE*4}for b in f:
    ...  ${SPACE*8}uart.WriteChar(b)
    ...
    ...  def mc_bus_send(path, seq, msgid, ver, bodyhex):
    ...  ${SPACE*4}uart = monitor.Machine[path]
    ...  ${SPACE*4}body = [int(t, 16) for t in bodyhex.split()]
    ...  ${SPACE*4}for b in _frame(int(seq), int(msgid, 16), int(ver), body):
    ...  ${SPACE*8}uart.WriteChar(b)
    ...
    ...  def mc_bus_inject(path, hexbytes):
    ...  ${SPACE*4}uart = monitor.Machine[path]
    ...  ${SPACE*4}for tok in hexbytes.split():
    ...  ${SPACE*8}uart.WriteChar(int(tok, 16))
    ...
    ...  def _frames():
    ...  ${SPACE*4}got = list(collected)
    ...  ${SPACE*4}del collected[:]
    ...  ${SPACE*4}out = []
    ...  ${SPACE*4}i = 0
    ...  ${SPACE*4}while i + 9 <= len(got):
    ...  ${SPACE*8}if got[i] != 0x7E:
    ...  ${SPACE*12}i += 1
    ...  ${SPACE*12}continue
    ...  ${SPACE*8}ln = got[i+3] | (got[i+4] << 8)
    ...  ${SPACE*8}if ln > 131 or i + 9 + ln > len(got):
    ...  ${SPACE*12}i += 1
    ...  ${SPACE*12}continue
    ...  ${SPACE*8}f = got[i:i+9+ln]
    ...  ${SPACE*8}crc = f[5+ln] | (f[6+ln] << 8) | (f[7+ln] << 16) | (f[8+ln] << 24)
    ...  ${SPACE*8}if _crc32(f[1:5+ln]) != crc:
    ...  ${SPACE*12}i += 1
    ...  ${SPACE*12}continue
    ...  ${SPACE*8}out.append((f[5] | (f[6] << 8), f[5:5+ln]))
    ...  ${SPACE*8}i += 9 + ln
    ...  ${SPACE*4}return out
    ...
    ...  def mc_bus_expect_pong(token, count):
    ...  ${SPACE*4}pongs = [p for (mid, p) in _frames() if mid == 0x0302]
    ...  ${SPACE*4}if len(pongs) != 1:
    ...  ${SPACE*8}print "BUS-FAIL expected 1 pong, got %d" % len(pongs)
    ...  ${SPACE*8}return
    ...  ${SPACE*4}want = [0x02, 0x03, 0x01] + _le32(int(token, 16)) + _le32(int(count))
    ...  ${SPACE*4}if list(pongs[0]) != want:
    ...  ${SPACE*8}print "BUS-FAIL payload got=%s want=%s" % (list(pongs[0]), want)
    ...  ${SPACE*8}return
    ...  ${SPACE*4}print "BUS-OK"
    ...
    ...  def mc_bus_expect_silence():
    ...  ${SPACE*4}pongs = [p for (mid, p) in _frames() if mid == 0x0302]
    ...  ${SPACE*4}if len(pongs) == 0:
    ...  ${SPACE*8}print "BUS-OK"
    ...  ${SPACE*4}else:
    ...  ${SPACE*8}print "BUS-FAIL answered when it should not: %s" % pongs
    ...  """
    Execute Command           ${peer}
    Execute Command           bus_hook "${UART}"
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     ${BANNER}    timeout=30
    # From here on, time is driven explicitly: pause, inject, RunFor — every
    # phase sees the same virtual timeline on every run. clear drops the
    # banner bytes the hook collected during boot.
    Execute Command           pause
    Execute Command           bus_clear

    # 1. A ping round-trips THROUGH the local bus: decoded, republished,
    #    consumed by the service, answered as a pong with count=1 — frame
    #    verified byte-exactly with the independent CRC.
    Ping And Run              0    CAFE0001
    Expect Pong               CAFE0001    1

    # 2. A second ping: count=2. The count lives in the service, not the
    #    codec — an echoing firmware cannot produce it.
    Ping And Run              1    CAFE0002
    Expect Pong               CAFE0002    2

    # 3. A CRC-corrupt ping earns total silence and costs exactly one
    #    frame: the next ping is answered as if nothing happened.
    Execute Command           bus_ping_corrupt "${UART}" "2" "0BAD0001"
    Execute Command           emulation RunFor "0.3"
    Expect Silence
    Ping And Run              3    CAFE0003
    Expect Pong               CAFE0003    3

    # 4. A valid frame whose id this side does not route: silence (counted
    #    on-device as rx_unknown), and the link keeps working.
    Execute Command           bus_send "${UART}" "4" "0999" "1" "AA BB"
    Execute Command           emulation RunFor "0.3"
    Expect Silence
    Ping And Run              5    CAFE0004
    Expect Pong               CAFE0004    4

    # 5. Garbage — including a stray SOF opening a bogus oversize frame —
    #    then a valid ping: the machine resyncs and answers.
    Execute Command           bus_inject "${UART}" "00 FF 7E 01 63 FF FF 12 34 55"
    Execute Command           emulation RunFor "0.2"
    Ping And Run              6    CAFE0005
    Expect Pong               CAFE0005    5
