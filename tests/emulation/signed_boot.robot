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
...               Then KEY ROTATION, on a SECOND bootloader whose trusted ring is
...               ["retired", key B] — the state a fleet is in after key A leaked:
...
...               4. signed by the rotated-in key B, --key-id 1 -> boots.
...               5. signed by the RETIRED key A -> refused. Same bytes that boot
...                  on the single-key bootloader above; revocation is the claim.
...               6. signed by A but stamped --key-id 1 -> refused. key_id selects,
...                  it never bypasses.
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

Rotated-In Key Boots On A Ring That Retired The Old One
    [Documentation]    KEY ROTATION. The bootloader's ring is ["retired", key B]:
    ...                key_id 0 is 32 zero bytes, key_id 1 is a key that never
    ...                left the vault. An image signed by B and stamped
    ...                --key-id 1 boots — the fleet survives losing key A.
    Execute Command           include @${RESC_ROTATED}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy bootloader    timeout=30
    Wait For Line On Uart     boot slot A    timeout=30
    Wait For Line On Uart     alloy uart_echo ready    timeout=30

Image Signed By The RETIRED Key Is Rejected
    [Documentation]    REVOCATION, and the case that makes the ring worth having.
    ...                This is the SAME image that boots on the single-key
    ...                bootloader above — a genuine signature by a key the device
    ...                used to trust. Once its ring slot is zeroed it can never
    ...                authenticate again.
    Execute Command           include @${RESC_RETIRED}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy bootloader    timeout=30
    Wait For Line On Uart     no valid image, waiting for update    timeout=30

Image Naming A Key It Was Not Signed With Is Rejected
    [Documentation]    key_id must be a SELECTOR, never a BYPASS. This image is
    ...                signed by the retired key A but stamped --key-id 1, so it
    ...                points the verifier at the live key B. If key_id were
    ...                merely carried, this would boot.
    Execute Command           include @${RESC_KEYIDLIE}
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     alloy bootloader    timeout=30
    Wait For Line On Uart     no valid image, waiting for update    timeout=30
