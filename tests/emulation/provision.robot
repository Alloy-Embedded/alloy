*** Settings ***
Documentation     PER-DEVICE FACTORY IDENTITY, proven in emulation — no hardware.
...
...               `alloy provision write` puts a 64-byte identity record in the
...               page the slot layout reserves for it (alloy::slots::provision_base
...               — outside both A/B slots, so no update ever touches it). The
...               claim these three machines test is that a DEVICE can find that
...               record, parse it, and act on it: the address the linker baked
...               in, the format the HOST encoder wrote and the parser that ships
...               in the product are the same three things.
...
...               Every machine boots the REAL bootloader and runs examples/factory
...               out of slot A as a REAL packed image, so the identity read
...               happens where it will happen on the line — inside a product
...               firmware, after an A/B boot — not in a special test harness.
...
...               1. identity page loaded  -> the firmware prints back the exact
...                  serial, MAC, hw revision and batch that `alloy provision write
...                  -o` encoded, and LINE TEST: PASS.
...               2. ERASED identity page (all-0xFF — the state every board is in
...                  before the line touches it) -> LINE TEST: FAIL unprovisioned.
...                  This is the negative control for the whole mechanism: a
...                  firmware that "read" a serial out of blank flash would pass
...                  test 1 for the wrong reason and fail here.
...               3. identity page with ONE FLIPPED BIT in the serial -> LINE TEST:
...                  FAIL ... corrupt. The record's CRC is the only thing that can
...                  catch this; a device that shipped with a mis-programmed serial
...                  and never noticed is the failure this exists to prevent.
...               4. identity page holding SOMETHING ELSE -> a DIFFERENT verdict
...                  from case 2. "You skipped a line step" and "something
...                  overwrote the page" send an operator to different places, so
...                  the two must not collapse into one message.
...
...               RESC_BLANK must LOAD an all-0xFF page rather than leave the
...               region untouched: Renode zero-fills memory nobody loaded, and
...               0x00 is not what an erased NOR flash reads. Left implicit, case
...               2 would silently exercise case 4's path — which is exactly what
...               happened the first time this suite was run.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
Provisioned Board Reports Its Identity And Passes
    [Documentation]    The address is asserted too (identity page: 0x...): if the
    ...                linker's provision_base and the host verb's write address
    ...                ever disagreed, the firmware would print one and read the
    ...                other, and every other line here would still pass.
    Execute Command           include @${RESC_PROVISIONED}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy bootloader    timeout=30
    Wait For Line On Uart     boot slot A    timeout=30
    Wait For Line On Uart     alloy factory line-test    timeout=30
    Wait For Line On Uart     identity page: ${PROVISION_BASE}    timeout=30
    Wait For Line On Uart     serial: ALY-0001-A7    timeout=30
    Wait For Line On Uart     mac: 02:1a:2b:3c:4d:5e    timeout=30
    Wait For Line On Uart     hw_rev: 3    timeout=30
    Wait For Line On Uart     batch: 42    timeout=30
    Wait For Line On Uart     LINE TEST: PASS    timeout=30

Unprovisioned Board Fails The Line Test
    [Documentation]    The state every board is in when it reaches the line.
    ...                Erased flash reads all-ones, which the record format
    ...                fast-rejects as `blank` rather than as corruption — the
    ...                operator needs "you skipped a step", not "your programmer
    ...                is broken".
    Execute Command           include @${RESC_BLANK}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy factory line-test    timeout=30
    Wait For Line On Uart     LINE TEST: FAIL unprovisioned    timeout=30

Foreign Bytes On The Identity Page Get Their Own Verdict
    [Documentation]    Not erased, not a record — something else is at
    ...                provision_base. A line operator seeing "unprovisioned"
    ...                would re-run the provisioning step and watch it fail
    ...                again; seeing this, they check what wrote over the page.
    ...                Collapsing the two would cost real hours on a real line.
    Execute Command           include @${RESC_FOREIGN}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy factory line-test    timeout=30
    Wait For Line On Uart     LINE TEST: FAIL no identity record    timeout=30

Corrupt Identity Record Is Refused By Its CRC
    [Documentation]    One bit flipped inside the serial number, everything else
    ...                byte-identical to the record that passes above. Only the
    ...                CRC separates the two, so this is the test that fails if
    ...                the CRC is ever computed over the wrong span — or not
    ...                checked at all.
    Execute Command           include @${RESC_CORRUPT}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy factory line-test    timeout=30
    Wait For Line On Uart     LINE TEST: FAIL identity record corrupt    timeout=30
