*** Settings ***
Documentation     The update root of trust, proven in emulation — no hardware.
...               A bootloader built with an [ota] public_key boots ONLY images
...               signed by the matching private key (Ed25519 over [header|payload],
...               third_party/monocypher). Three machines, and the two REJECTIONS are
...               the security claim:
...
...               1. correctly signed  -> boots the app.
...               2. TAMPERED payload  -> refused. The tamper repairs both CRCs, so
...                  integrity checks pass and ONLY the signature can catch it —
...                  this is the test that would fail if authenticity were fake.
...               3. signed by ANOTHER key (an attacker with their own keypair, a
...                  perfectly valid signature) -> refused.
...
...               A refusal must land in update mode, never in a jump: an unauthentic
...               image leaves the device recoverable over the same wire.
Suite Setup       Setup
Suite Teardown    Teardown
Resource          ${RENODEKEYWORDS}

*** Test Cases ***
Correctly Signed Image Boots
    Execute Command           include @${RESC_SIGNED}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy bootloader    timeout=30
    Wait For Line On Uart     boot slot A    timeout=30
    Wait For Line On Uart     alloy uart_echo ready    timeout=30

Tampered Image Is Rejected
    Execute Command           include @${RESC_TAMPERED}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy bootloader    timeout=30
    Wait For Line On Uart     no valid image, waiting for update    timeout=30

Image Signed By The Wrong Key Is Rejected
    Execute Command           include @${RESC_WRONGKEY}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy bootloader    timeout=30
    Wait For Line On Uart     no valid image, waiting for update    timeout=30
