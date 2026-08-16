# Firmware update over UART

Shipping a product means being able to fix it after it leaves the building. alloy carries a
complete field-update path: a resident **bootloader**, two firmware **slots**, a **trial boot**
that rolls back on its own when the new firmware misbehaves, and optional **Ed25519 signing**.

Like everything else here, it is portable — the same bootloader source builds for every family
whose flash controller alloy models, and the slot layout is *computed from chip data*, never
hand-written.

```console
$ alloy keygen -o keys/update.key          # once, per product
$ alloy build --slot bl && alloy flash     # the bootloader, programmed with a probe
$ alloy image build/app.elf --set-version 2 --sign keys/update.key -o app_b.img
$ alloy update --image-b app_b.img --port /dev/tty.usbmodem1103
  42496/42496 B (100%)
update accepted -> slot B (device was running version 1); device now reboots
into a TRIAL boot — it must be confirmed to stick
```

## The layout comes from the chip data

Flash is partitioned into four regions, plus a factory identity page carved out of one of them.
The CLI derives all of it from the flash size and the erase granularity of the flash-controller
IP — facts already in the device database — and emits it as `alloy/slots.hpp`:

```
[ bootloader | identity ][ slot A ][ slot B ][ boot-state store ]
```

| Board | Erase stride | Bootloader | Slot A | Slot B | Store | Identity |
| --- | --- | --- | --- | --- | --- | --- |
| `nucleo_g071rb` (128 KB) | 2 KB page | 32 KB @ `0x08000000` | 46 KB @ `0x08008000` | 46 KB @ `0x08013800` | 4 KB | 2 KB @ `0x08007800` |
| `nucleo_f722ze` (512 KB) | 128 KB sector | 32 KB @ `0x08000000` | 128 KB @ `0x08020000` | 128 KB @ `0x08040000` | 32 KB | 64 KB @ `0x08010000` |
| `same70_xplained` (2 MB) | 8 KB block | 32 KB @ `0x00400000` | 1000 KB @ `0x00408000` | 1000 KB @ `0x00502000` | 16 KB | 8 KB @ `0x00406000` |

Two decisions in that table are worth knowing about, because they are permanent once a device is
in the field:

!!! info "The bootloader region is 32 KB even when you don't sign"
    The UART bootloader itself is around 4 KB. Ed25519 verification costs ~14 KB more on
    Cortex-M0+. The region is sized for signing **whether or not a given build signs**, because
    the layout is baked into fielded devices: if turning signing on later moved the slots,
    already-deployed products could never accept the update that introduces it. Address space is
    cheap; a bricked fleet is not.

!!! info "No image swap — apps run from the slot they were written to"
    The active slot *is* the known-good fallback. An update lands in the other slot and boots
    from there directly. That removes the swap algorithm (and its power-loss states) entirely,
    at the cost of linking the app twice — see below.

!!! info "The identity page is carved out, never added"
    Per-device identity needs a home outside both slots. It could not become a *fifth* partition:
    that would have moved `slot_b_base`, and a slot base is the one number a fielded device can
    never be told about — the update it would need in order to learn the new layout is written
    using the old one. So the page comes out of a region that already exists.

    On uniform-page flash (G0, SAME70) it is the **last erase page of the bootloader region**.
    Every published address is byte-identical to what shipped before the page existed; what
    changes is the bootloader's *link window*, which shrinks by one page (`bootloader_code_size`
    in `slots.hpp`). A bootloader that grows into the identity page therefore fails the **link**,
    instead of quietly erasing a customer's serial number at the next reflash.

    On the F7 the erase unit is a whole sector and the bootloader needs all 32 KB for Ed25519, so
    there is no "last page" to give — identity goes in the medium sector the F7 map already left
    reserved, and nothing moves at all.

The app's vector table sits at a fixed `0x200` offset inside its slot, so the updater can stream
`[header | payload]` straight into flash with the vectors landing where the bootloader's jump
expects them.

## Build the three binaries

```console
$ alloy build --slot bl      # the bootloader → its own region
$ alloy build --slot a       # your app, linked to run from slot A
$ alloy build --slot b       # the same app, linked to run from slot B
```

Because there is no swap, an app is position-dependent: it must be linked for the slot it will
execute in. You ship **both** builds, and `alloy update` picks the right one — the device says
which slot it is about to write in its `INFO` reply.

```console
$ alloy update --image-a app_a.img --image-b app_b.img --port /dev/ttyUSB0
```

Passing both is the safe form: if the device reports a slot you did not supply an image for, the
update stops before writing anything. A single image can be passed positionally, but then you are
asserting you already know the target slot — nothing cross-checks it, and the wrong one costs a
trial boot (the rollback saves the device; the update cycle is wasted).

## Pack an image

```console
$ alloy image build/app.elf --set-version 3 -o app_b.img
image: app_b.img  (42496 B = 32 B header + 42464 B payload, version 3)
```

`alloy image` accepts an ELF directly (it flattens it exactly the way `objcopy -O binary` does)
or a raw `.bin`, and wraps it in a 32-byte header:

| Offset | Field | Notes |
| --- | --- | --- |
| 0 | magic `"ALOY"` | fast-rejects erased (`0xFF`) flash |
| 4 | header version / size | lets the header grow without breaking old devices |
| 8 | `image_version` | monotonic — `--set-version`; **enforced**, see [anti-rollback](#anti-rollback) |
| 12 | `payload_length` | exact bytes to CRC; never CRC a whole slot |
| 16 | `payload_crc32` | integrity of the firmware itself |
| 20 | `flags` | reserved |
| 22 | `key_id` | which key of the device's ring signed this — `--key-id`, default 0 |
| 28 | `header_crc32` | so a corrupt header is rejected *before* its length is trusted |

Two independent CRCs is not belt-and-braces: the header CRC is what makes `payload_length`
trustworthy enough to use as a bound.

## Trial, confirm, rollback

A new image is armed as a **one-shot trial**. The bootloader consumes one attempt *before*
jumping, so a firmware that hangs cannot retry forever. The app must call `confirm()` after its
own health check; if it never does, the bootloader reverts to the previous firmware.

```mermaid
stateDiagram-v2
    [*] --> Normal: boot the confirmed slot
    Normal --> Trial: update installed (attempts = N)
    Trial --> Confirmed: app calls confirm()
    Trial --> Trial: reset — attempt consumed
    Trial --> Reverted: attempts exhausted
    Reverted --> Normal: previous firmware boots
    Confirmed --> Normal: it is now the fallback
```

The whole state — active slot, pending slot, attempts left — packs into one 32-bit word, so any
transition commits in a **single store write**. There is no half-updated state to tear, and the
store itself is a two-page ping-pong whose write leaves the previous value authoritative if power
drops mid-write.

On the app side that is three lines ([`examples/ota_app`](https://github.com/Alloy-Embedded/alloy/tree/main/examples/ota_app)):

```cpp title="illustrative: `alloy::slots` exists only in a slot-configured project — `alloy build --slot a`, see examples/ota_app"
using flash_hal = alloy::hal::flash_impl<alloy::slots::flash_ctrl_t>;
alloy::ota::boot_store<flash_hal> store{flash_hal{}, alloy::slots::store_base,
                                        alloy::slots::store_page_b};
alloy::ota::boot_manager mgr{store};
if (mgr.confirm()) { uart.write("confirmed\r\n"); }   // idempotent on normal boots
```

!!! warning "Reaching `main()` is not health"
    `confirm()` is a promise that *this firmware works*. Call it after the check that would
    actually catch a bad build — the sensor answered, the radio associated, the calibration
    loaded. Confirming at the top of `main()` gives you the ceremony of rollback with none of the
    protection.

### A hung trial is a reset, not a wedge

Attempts are only consumed across resets, so firmware that hangs without crashing would sit there
forever. The bootloader closes that gap: before a **trial** jump — and only a trial jump — it
arms the hardware watchdog.

```cpp
if constexpr (board::caps::watchdog) { board::watchdog.start(std::chrono::seconds{2}); }
```

A healthy app confirms and keeps feeding. A hung one is reset, consumes an attempt, and the fleet
rolls back on its own with nobody pressing anything. Confirmed boots are *not* armed by the
bootloader — that is the product's policy, not the bootloader's.

## Anti-brick doctrine

Four rules, all enforced in the bootloader rather than documented and hoped for:

1. **Verify what you boot, not just what you wrote.** The slot is verified again at boot, with
   the same code path the updater used.
2. **A trial that fails verification is rejected immediately**, not retried until attempts run
   out.
3. **If nothing verifies, stay in update mode.** A blank or fully corrupt board keeps listening
   on the same wire it was updated over — there is no state that requires a probe to escape.
4. **Verify then arm, in that order.** A crash between the two leaves `pending == none`, so the
   confirmed firmware still boots.

## Signing

Integrity (CRC) proves the image arrived intact. It proves nothing about *who sent it*. If your
update path is a service technician's laptop, a phone app, or anything reachable by someone you
have not met, sign the images.

=== "1. Make a keypair"

    ```console
    $ alloy keygen -o keys/update.key
    private key: keys/update.key  (0600 — KEEP THIS SECRET AND BACKED UP: lose it
    and fielded devices can never be updated again)
    public key:  keys/update.pub
    ```

=== "2. Tell the project its public key"

    ```toml title="alloy.toml"
    [ota]
    public_key = "keys/update.pub"
    ```

    Codegen bakes it into `alloy/ota_key.hpp`. The header is emitted either way, so firmware
    selects the policy with `if constexpr (alloy::ota_key::configured)` — no `#ifdef`, in
    keeping with the rest of the framework.

=== "3. Sign each image"

    ```console
    $ alloy image build/app.elf --set-version 3 --sign keys/update.key -o app_b.img
    ```

The signature is an Ed25519 trailer over exactly the bytes the device already verifies
(header + payload), using [Monocypher](https://monocypher.org/). Nothing about the v1 image
format changed, so the format did not fork when signing arrived.

A signing-enabled device refuses an unsigned or wrongly-signed image **on the wire** — the
operator sees a NAK — rather than accepting it and burning a trial boot on firmware it was never
going to run.

!!! danger "Keep the private key out of the repository"
    `alloy keygen` writes the private key with a `.pub` sibling; commit only the `.pub`. Anyone
    holding the private key can update every device that trusts it.

## Anti-rollback

A signature cannot catch a **downgrade**. A replayed old image is one the vendor really did
sign, so every crypto check passes; the attacker's move is to push last year's firmware to
reopen a bug you fixed. Only a version policy stops that, and alloy enforces one.

**The rule.** At the moment it would *accept* an image, the device refuses anything whose
`image_version` is below the highest version it can prove it already holds — the floor is the
running firmware's version, raised by whatever verifies in the slot about to be overwritten.
The refusal is a NAK carrying `ota_error::rollback`, which `alloy update` prints in words.

```console
$ alloy update --port /dev/ttyUSB0 old_v1.img
error: device rejected update (ota_error 11: rollback — the device already holds a version at
or above this image's, and refuses the downgrade. Re-issue with a higher --set-version. The
target slot was rewritten and is now garbage; the running firmware is untouched)
```

**Why it survives power loss.** There is no new persistent record to tear: the floor is derived
from the *slots themselves*, which is the same flash the boot-state's guarantees already rest
on. After an automatic rollback moves the device back to the older slot, the newer image is
still sitting in the other slot, so the floor does not drop.

!!! warning "Read these three limits before you rely on it"
    - **It is an accept-time rule, never a boot-time one.** A confirmed *older* slot stays
      bootable forever — deliberately. The anti-brick fallback is "if the planned slot does not
      verify, boot the other one", and after any update the other one is by definition older. A
      boot-time floor would refuse exactly the fallback that keeps devices alive.
    - **A device on which neither slot verifies has a floor of 0** and accepts anything. That is
      required: a blank or bricked board must always be recoverable. So the claim is *a working
      device refuses a downgrade*, not *a device refuses a downgrade*.
    - **The refusal happens at FINISH**, after the image has already been streamed and the
      inactive slot erased. The running firmware is untouched and nothing is armed, but the
      operator is left with a garbage inactive slot. The NAK text says so.

## Key rotation and revocation

One baked-in public key is a single point of failure for the whole fleet: the day it leaks,
every device trusts the attacker. A device can instead trust a small **ring** of keys, and the
image header's `key_id` says which one signed it.

```toml title="alloy.toml"
[ota]
public_keys = ["retired", "keys/rotate.pub"]   # key_id 0 retired, key_id 1 live
```

```console
$ alloy image build/app.elf --set-version 4 --sign keys/rotate.key --key-id 1 -o app_b.img
```

The ring is **positional**: entry *i* is what `--key-id i` selects. A key is retired by
replacing its entry with `"retired"`, never by deleting it — deleting entry 0 would silently
re-point every image that names key 0 at what used to be key 1. A retired entry is emitted as 32
zero bytes and is refused before Ed25519 is ever called. Codegen refuses a ring that is empty,
entirely retired, or contains the same key twice (retiring one of a duplicate pair would quietly
do nothing).

Exactly **one** signature check runs regardless of ring size — `key_id` selects a key, it never
triggers a try-every-key sweep, and it is inside both the header CRC and the signed bytes, so it
cannot be re-pointed by an attacker.

Backward compatibility is by construction: `key_id` occupies the `reserved` u16 that every image
ever produced wrote as zero, so old images select ring entry 0 and `header_version` stays 1.
`public_key = "..."` still works and means a ring of one.

!!! danger "Revocation requires shipping a new bootloader — say it plainly"
    The ring is `.rodata`, baked in at build time. Retiring a key takes effect only on devices
    that *receive the bootloader containing the new ring*, which must itself be signed by a key
    they already trust. There is no over-the-air revocation message, because a revocation floor
    would need persistent state `ota::boot_store` cannot hold (it stores exactly one `u32`).
    Until the new bootloader lands, a device keeps trusting the leaked key. Plan the rotation
    *before* you need it: ship a second key in the ring while the first is still healthy.

### What signing, versioning and rotation do and do not buy you

| Threat | Covered? |
| --- | --- |
| Corrupted transfer | Yes — CRC, before any signature work |
| Attacker-authored firmware | Yes — they cannot produce a valid signature |
| Tampering with an image in transit, CRCs repaired | Yes — this is a CI test case |
| Replaying an **older, genuinely signed** image | Yes — refused with `ota_error::rollback`, on a device that has a working slot to derive a floor from ([limits](#anti-rollback)) |
| Key compromise | Partly — a ring lets a leaked key be retired without bricking the fleet, but retiring it requires shipping a new **bootloader**; there is no field revocation |
| Per-device keys (one compromised unit ≠ the fleet) | No — alloy has no RNG/TRNG HAL, so a per-device keypair would have to be generated on the host, which means the host held the private half ([details](#per-device-identity)) |
| Telling one unit from another (serial, MAC) | Yes — but as *identity*, never as a secret: the factory writes it, firmware can only read it, and no update touches it ([details](#per-device-identity)) |
| Reading firmware off the device | No — signing is authenticity, not confidentiality (see `alloy secure` for RDP) |

## Per-device identity

A firmware image is the same on every board. A serial number is not. `alloy provision` writes the
facts a board is *born* with into the identity page — outside both slots, so `alloy update` can
replace the firmware a thousand times and the serial number is still the serial number.

```console
$ alloy provision write --serial ALY-0001-A7 --mac 02:1a:2b:3c:4d:5e --hw-rev 3 --batch 42
nucleo_g071rb (st/STM32G071RB)
  identity page: 0x08007800 +2048 B (from the slot layout)
  writing: serial 'ALY-0001-A7'  mac 02:1a:2b:3c:4d:5e  hw_rev 3  batch 42
verified by readback: serial 'ALY-0001-A7'  mac 02:1a:2b:3c:4d:5e  hw_rev 3  batch 42

$ alloy provision read
nucleo_g071rb @ 0x08007800: serial 'ALY-0001-A7'  mac 02:1a:2b:3c:4d:5e  hw_rev 3  batch 42
```

The record is a flat 64-byte little-endian POD with a self-CRC, marshaled field by field exactly
like the image header:

| Offset | Field | Notes |
| --- | --- | --- |
| 0 | `magic` u32 | `"APRV"` — fast-rejects erased flash |
| 4 | `format_version` u16 | 1 |
| 6 | `record_size` u16 | 64 — lets the record grow later without moving the page |
| 8 | `serial[16]` | ASCII, NUL-padded; **not** NUL-terminated when full |
| 24 | `mac[6]` | EUI-48 in wire order |
| 30 | `hw_revision` u16 | board/PCB revision; 0 = unspecified |
| 32 | `batch` u32 | factory lot id; 0 = unspecified |
| 36 | `reserved[24]` | zero |
| 60 | `crc32` u32 | CRC-32/ISO-HDLC over bytes 0…60 |

Firmware reads it in one line, and the read is the *only* operation firmware has — there is no
writer on the device at all, so no firmware bug and no hostile update can rewrite a device's
identity:

```cpp title="illustrative: `alloy/slots.hpp` exists only in a slot-configured project — `alloy build --slot a`"
#include <alloy/provision.hpp>
#include <alloy/slots.hpp>

if (auto id = alloy::provision::read(alloy::slots::provision_base)) {
    uart.write(id->serial_view());        // bounded: a 16-byte serial has no terminator
} else if (id.error() == alloy::provision::prov_error::blank) {
    // never provisioned — a line fault, not a field fault
}
```

Three failure states, deliberately distinct, because they send an operator to different places:
`blank` (erased page — a line step was skipped), `bad_magic` (something else is at that address),
and `bad_crc` (a torn write or a partial erase — re-provision this board).

!!! warning "The CRC is integrity, not authenticity"
    It catches a bad program, not an attacker with a probe. Identity is **not a secret and not a
    capability** — nothing here should ever be used as a key. What stops an attacker rewriting the
    page is [`alloy secure`](security.md) (RDP, and WRP where the page falls inside the
    write-protected bootloader region), not this CRC.

!!! warning "There is no per-device private key, on purpose"
    It is the obvious next field and it is absent because alloy has **no RNG/TRNG HAL on any
    supported part**. A per-device keypair would therefore have to be generated on the host —
    which means the factory PC held every device's private half, on disk, next to a CSV of serial
    numbers. That is a worse security story than having no per-device key at all. The field
    arrives when a TRNG driver and an on-device keygen do; `record_size` exists so the record can
    grow without moving the page.

### The refusals are the product

A factory types these values once per board, at speed, off a work order. Each of these is a
one-line mistake whose symptom appears months later, so each is a **refusal**, not a warning:

| Input | Why it is refused |
| --- | --- |
| a serial longer than 16 bytes | truncating silently is how two devices end up sharing a serial |
| `" SN-1 "` | stored verbatim, so the device would not match its own work order |
| non-ASCII or control characters | the record is 16 raw bytes printed over a debug UART |
| `01:…` (multicast bit set) | a station may not use a group address as its own; frames it sends are dropped, and it is one nibble from a valid address |
| `ff:ff:ff:ff:ff:ff` | that is the broadcast address |
| a duplicate serial in the work order | see the line script below |

`alloy provision write` also **verifies by reading back** — always. The record is dumped off the
device and compared byte for byte before the command reports success, which is the only check that
catches a write the hardware silently refused (see the ordering trap below).

## Mass programming: the production line

`examples/factory/` is a worked production line: a line-test firmware and the script that drives
it. `line.py` enforces the order rather than documenting it, because every step is reversible
except the last one, and the last one is reversible only by mass-erasing the chip.

```console
$ python examples/factory/line.py --serials work-order.csv --board nucleo_g071rb \
      --product-image /tmp/product.img --count 50
```

| # | Step | Path | Why here |
| --- | --- | --- | --- |
| 1 | flash the bootloader | probe | the only step that needs a probe to load code — a blank board has nothing to take an update with |
| 2 | load the line-test image | `alloy update` | over the field path, so every unit's update path is exercised once before it ships |
| 3 | **provision the identity** | probe | must precede step 6; verified by readback |
| 4 | run the line test | debug UART | requires `LINE TEST: PASS` — the step that proves the *device* agrees with the *work order* |
| 5 | load the product firmware | `alloy update` | |
| 6 | lock the part | probe | **dead last** — see below |

!!! danger "Provision before you secure — this one bricks batches"
    `alloy secure apply --rdp 1 --wrp-bootloader` does two things that make steps 1–3 impossible:
    RDP level 1 blocks the debug probe from flash, and on uniform-page flash `--wrp-bootloader`
    write-protects the bootloader region — **which contains the identity page**. Provision first
    or the serial number never lands. Returning to RDP 0 to fix it mass-erases the board:
    bootloader, both slots, identity.

    If you do get it backwards, `alloy provision write`'s readback is what tells you, and it names
    the cause. openocd exits 0; the page is simply still erased.

    On the F7 the identity page is *outside* the write-protected sectors, so WRP does not freeze
    it there. The order is the same either way — RDP level 1 alone is enough to lock the probe out.

Two smaller things the script refuses, both of which are real production failures:

- **a duplicate serial inside the work order.** The usual source is a spreadsheet copy-paste, and
  the duplicate is discovered when two devices report the same identity to a fleet server, months
  later, from different continents.
- **reusing a serial across runs.** A ledger next to the work order records every serial that has
  been burned into a board, flushed after *every* board — the failure it exists for is the line PC
  losing power mid-batch — so a re-run resumes at the next unused serial instead of starting over.

A board that fails any step never reaches step 6: locking an unprovisioned board makes it scrap.

The line-test firmware (`examples/factory/src/main.cpp`) is a slot-A app, packed by `alloy image`
like any other, so it boots through the real bootloader out of the real slot. It prints the
identity and exactly one verdict line, which is the only thing a line script should grep for:

```
alloy factory line-test
identity page: 0x08007800
serial: ALY-0001-A7
mac: 02:1a:2b:3c:4d:5e
hw_rev: 3
batch: 42
LINE TEST: PASS
```

## Adding a transport

The UART receiver is sans-IO — it owns no UART, no clock and no flash — so the update machinery is
reusable over any byte pipe: CAN, RS-485/Modbus, TCP, BLE. `src/alloy/ota/uart_transport.hpp` is
the **reference implementation**; a new transport re-implements its framing half and reuses
everything below it.

A transport has to supply four things, and nothing else:

1. **Framing.** Deliver a whole, integrity-checked message to the session layer, and emit whole
   messages back. UART does this with `SOF | type | seq | len | payload | crc32`; a bus that
   already frames (CAN, Modbus RTU, TCP) reuses its own framing instead.
2. **The four message types.** `HELLO` → `INFO`, `DATA(seq, bytes)` → `ACK`/`NAK`,
   `FINISH` → `ACK`/`NAK`. `INFO` carries the protocol version, the target slot, the running
   `image_version` and the largest chunk the device will buffer.
3. **The sequence rule.** The device is a pure responder: a duplicate `DATA` seq (a retransmit
   after a lost ACK) is re-ACKed **without rewriting flash**; anything else out of order is NAKed;
   a fresh `HELLO` restarts the session. All retransmission policy lives in the host.
4. **An error channel.** `NAK` carries the `ota_error` value, which is how an operator learns
   *why* — `10 not_authentic`, `11 rollback`, `12 unknown_key` are very different conversations.

Below that line nothing changes: the transport writes into any `UpdateSink` (`alloy::ota::updater`
satisfies it as-is), and verification, the anti-rollback floor, the key ring and the trial/confirm
machine are all downstream of it.

!!! note "Update over Modbus RTU: designed, deliberately not built"
    `libs/modbus`'s `rtu_server` has the matching seam already — a `UserDispatch` hook for vendor
    function codes, invoked with the request PDU and a response buffer — so a device-side
    `ota_dispatch` satisfying it is about 95 lines, and it was written and compiled against
    `src/` + `libs/modbus` during this work. It is not in the tree, because the *demonstrated*
    cost is not 95 lines: it also needs a host-side RTU master (there is no Modbus **client** in
    `tools/alloy` today), an example, and a Renode leg. Shipping the device half alone would be
    exactly the half-built transport this section exists to prevent.

    Two findings are worth recording before anyone picks it up, because both are properties of
    Modbus and not of alloy:

    - **A vendor function code has no length rule**, so `expected_adu_length` returns
      `length_unknown` and OTA frames are closed by **t3.5 silence alone**. That is solid at
      ≤19200 baud (t3.5 ≈ 2005 µs, longer than one 1 ms tick) and tight at 115200 (≈1750 µs).
    - **Modbus has no field for an `ota_error`.** Every refusal collapses into exception `0x04`,
      server failure. The UART transport's NAK carries the exact code; losing it is a genuine
      regression in field diagnosability, and it is the reason update-over-Modbus should be an
      option a product opts into rather than the default path.

    CAN, Ethernet and BLE are further away still: each needs its own framing *and* its own host
    client, and none has an emulation leg to be proven in.

## The wire protocol

Stop-and-wait, every frame CRC-32 guarded, and deliberately dull:

```
SOF 0x7E | type u8 | seq u8 | len u16 | payload[len] | crc32 u32
                     └────── crc32 covers type..payload ──────┘

host → device:  HELLO · DATA · FINISH
device → host:  INFO  · ACK  · NAK
```

The device is a pure responder — **all** retransmission policy lives in the host. A duplicate
`DATA` sequence number (a retransmit after a lost ACK) is re-ACKed without rewriting flash;
anything else out of order is NAKed. A fresh `HELLO` restarts the session, which is what makes a
host retry after a power blip mid-transfer safe.

The receiver is sans-IO: it owns no UART, no clock and no flash. Bytes are pushed in, replies come
out through a callable. That is why it can be host-tested exhaustively, and why the same receiver
works from a bootloader or from a running application.

!!! note "The update window"
    After reset the bootloader listens for ~500 ms, extended to 3 s by any traffic, then boots the
    app. `alloy update` retries `HELLO` several times, so the normal procedure is: start the
    command, then reset the device — the retry lands inside the window and the session begins.
    (An application can also reboot itself into the window on command, which is how a product with
    no reset button gets updated.)

## What this is proven against

The whole lifecycle runs in CI, on emulated hardware, as a **blocking gate** — install → trial
boot → confirm → survive a power cycle → **a replayed older image is refused** → install a bad
app → three watchdog self-resets → autonomous rollback, driven by the real `alloy update` client
over a real serial link, on all three flash families.

The downgrade leg and the bad-update leg are each other's control: in one run the same device
must refuse version 0 and, minutes later, accept version 2. A floor that refused everything
would look identical if you only ran the first half.

The signing gate adds six cases across two bootloaders. On a single-key bootloader: a correctly
signed image boots, a tampered image with both CRCs repaired is rejected, and an image signed by
the wrong key is rejected. On a bootloader whose ring is `["retired", key B]` — the state a fleet
is in the day after its signing key leaked: an image signed by B with `--key-id 1` boots; the
*very same file* that boots on the single-key bootloader is refused, which is the revocation
claim; and an image signed by the retired key but stamped `--key-id 1` is refused, because
`key_id` selects and never bypasses. Every rejection lands the device back in update mode.

The provisioning gate adds four machines, all booting the real bootloader and running the
line-test app out of slot A as a real packed image. A provisioned board prints back the exact
serial, MAC, revision and batch that `alloy provision write` encoded — *and* the identity address
the linker baked in, so a disagreement between the emitter and the host verb cannot hide. The
other three are the refusals: an **erased** page (`FAIL unprovisioned`), a page holding
**something else** (`FAIL no identity record`), and a record with **one flipped bit** in the
serial (`FAIL ... corrupt`), which only the record's CRC can catch.

See [Emulation](emulation.md) for how that harness works.

!!! warning "Honest status"
    None of the three flash drivers has run on silicon, and they are not equally proven in
    emulation either:

    - **STM32F7** runs against Renode's own flash-controller model — real sector erases.
    - **SAME70 EEFC** runs against a model this project wrote, so it cannot independently falsify a
      misreading of the reference manual.
    - **STM32G0** has **no flash-controller model at all** in Renode. Its register writes land on
      unmapped addresses during the emulation legs, so `st_flash_g0`'s program/erase sequences are
      *not* exercised — only the layer above them is. (The G0 driver has, separately, been reported
      working on silicon through the NVM key/value store, whose boot counter survives resets.)

    The full lifecycle — bad image, exhausted trials, automatic rollback, the refused downgrade —
    and the signed-image / key-rotation oracle run on `nucleo_g071rb` only; SAME70 runs the
    install/trial/confirm half.

    The anti-rollback and key-rotation gates are **emulation and host-test claims only**. No
    board has run them. In particular, "a retired key can never authenticate again" is proven
    against Renode and against a native host test using the same vendored Monocypher the
    firmware links — not against silicon, and not against a device whose flash an attacker can
    write directly.

    Provisioning is split the same way. What is proven: the **record format**, by host tests on
    both sides of the language boundary pinning the same 64 golden bytes, and by four Renode
    machines that read it out of the layout's page through a real bootloader. What is **not**
    proven: that `alloy provision write` programs a real device. The openocd command list is
    reasoned from openocd's documented `flash write_image` / `dump_image` and has never driven a
    probe — no board was on hand. Neither has `examples/factory/line.py`; its refusals, its
    ledger, its step order and its exact argv are unit-tested, and nothing else about it is.

    Nor is "the identity page survives an update" a silicon claim. It rests on address arithmetic
    — checked over **every** chip in the database, so the provisioning page provably intersects
    neither slot nor the boot-state store — plus the Renode legs showing firmware reads it from
    where the layout says. No emulation leg runs an update and then re-reads the identity.

    Chips whose flash controller alloy does not model — RP2040 and ESP32 today — get no slot layout
    at all rather than a guessed one, so `--slot` is simply not offered for them.
