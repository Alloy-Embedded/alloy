# Connectivity & full peripheral coverage — strategy (DRAFT)

Design analysis for two related questions: **ESP32 WiFi/BT**, and **driving
every peripheral of the six validated families**. Written from a
three-front investigation (2026-07-23). This is the contract that keeps
both landing coherently with the [NORTH_STAR](NORTH_STAR.md) doctrine.

## 1. The uncomfortable truth about ESP32 WiFi/BT

WiFi/BT is **not a peripheral you drive from registers.** The 802.11 MAC and
the BT controller are proprietary Espressif silicon with *no public register
documentation* — the TRM deliberately omits them. They are drivable only
through **closed binary blobs** (`libpp`, `libnet80211`, `libcore`, `libphy`,
`libcoexist`; `libbtdm_app` for BT). Nobody has reimplemented the MAC from
registers, and Espressif has repeatedly declined to open it. So the choice is
not *how to reimplement* (impossible) — it is **port the blobs + write the
runtime shim they demand.**

The blobs are **ABI-locked to a FreeRTOS-shaped OS-adapter** (`wifi_osi_funcs_t`,
~100-120 C function pointers) that requires exactly what alloy deliberately
does *not* have: a **heap**, a **preemptive scheduler** (semaphores/queues/
recursive mutexes/event-groups), real **ms+µs software timers**, dynamic
**interrupt allocation**, and **NVS** for PHY calibration.

**The precedent that proves feasibility:** `esp-wifi` (esp-rs) drives the SAME
blobs bare-metal, without ESP-IDF, via a ~200-line preemptive round-robin
scheduler + a compat shim + a heap, paired with a pure-Rust TCP stack. For C++
the natural port is to reimplement esp-idf's own `esp_adapter.c` (~900 lines of
C, the blobs want a C-ABI table) against alloy primitives, and bring **lwIP**
(BSD-C) for TCP/IP. The blobs are **Apache-2.0 redistributable** (esp-wifi-sys
proves a non-IDF project may ship them).

**Verdict:** feasible, but a **months-scale, high-risk subsystem** whose crux is
that it forces alloy to grow a real scheduler + heap — the exact things its
design avoids. Not a bolt-on. Smallest first WiFi win (STA + one HTTP GET) is
~2-4 months; BLE is *not* meaningfully easier (same shim + a large host stack).

## 2. Therefore: blob-free Ethernet FIRST

The strategic move is **not to start with the ESP32 blobs.** Start with
**Ethernet on the SAME70 Xplained** — the GMAC and the KSZ8081 PHY are already
in our device data, it is **100% open (zero blobs)**, and it exercises the
*entire* new machinery (a MAC driver → a TCP stack → a Socket API) on a target
we fully control and can inspect line-by-line. WiFi then becomes *additive*:
swap the MAC driver for `[blob + shim]` behind an already-validated concept —
containing the NEW blob risk separately from the NEW net-stack risk, exactly
the risk isolation NORTH_STAR already applies to the ESP32.

## 3. Architecture — capability-gating without breaking the thesis

The thesis was never "every app runs on every board." It is: **portable source
recompiles everywhere, and non-portability is a one-screen compile error.**

- **Small roles** (led/uart/spi/i2c/adc/pwm/dma/irq): unchanged — `caps::x`
  bool + no-op stub.
- **Heavy roles** (net/wifi/ethernet/usb/fs): add `caps::wifi`,
  `caps::ethernet`, `caps::net`. **No no-op stub.** The board provides a real
  `board::eth` / `board::wifi` type only where the silicon has it; where absent,
  emit a **poisoned type** — `static_assert(dependent_false_v<T>, "board X has
  no Ethernet — see board::caps::ethernet")`. Portable code guarded by
  `if constexpr (caps::net)` in a generic lambda (the `dma_probe` idiom) never
  instantiates it and compiles everywhere; code that unconditionally names
  `board::eth` on a board without it gets a readable compile error. Capability
  is **data-derived** (board.json + chip data), never a user flag — so you
  cannot "enable wifi" on a G0 and get a link error instead of a static_assert.

### Layering — three tiers, one seam

- **alloy (core):** peripherals + the driver side of MACs we own (GMAC/ETH is
  just another IP-version driver — facts generated, behavior hand-written,
  static DMA descriptor rings, **no heap**), exposing a narrow
  `alloy::net::NetDevice` concept (embassy-net-driver shape:
  `poll_link` / `receive()->RxToken` / `transmit(len)->TxToken` /
  `capabilities{mtu,mac}`). New concepts in `concepts.hpp`: `NetDevice`,
  `Mdio`/`SmiBus`, `Socket`, `WifiStation`.
- **alloy-net (NEW opt-in package):** a bounded static-pool allocator (not
  `malloc`), vendored **lwIP** (NO_SYS, static pools, a *generated* `lwipopts.h`),
  an lwIP-netif↔NetDevice adapter, and the `Socket`/`TcpListener`/`UdpSocket`
  front end. **Poll-driven** from the superloop — no RTOS. An RTOS/async
  adapter is an add-on, never inherited.
- **alloy-wifi / vendor-blob packages (opt-in, later):** the ESP32 radio blob +
  netif glue presenting the SAME `NetDevice` concept upward. Blob isolation
  lives here, never in core.
- **Build seam:** generalize `build.py`'s hardcoded optional-file loop into
  "a project's declared packages contribute source sets + include dirs to the
  CMake target" (the one genuinely new build capability).

**TCP/IP stack:** vendor **lwIP** (NO_SYS) for v1 — BSD-licensed, single-thread
pollable, it is *what the ESP32 blob already speaks* (one stack serves both the
owned-MAC and the vendor-netif paths), and its `lwipopts.h` is exactly the kind
of config alloy generates. Do NOT hand-port smoltcp to C++ (huge behavior
surface — violates depth-before-breadth). `alloy::net::Socket` is the only thing
user code sees, so lwIP stays swappable.

**Honesty tiers** for connectivity (a netif that never saw a cable is the
connectivity version of the decorative-C++ trap): `compiled` (CI builds+links) →
`linked-loopback` (on-target internal loopback) → `silicon` (real DHCP lease +
TCP round-trip on the physical board). Generated from a test manifest, never
hand-claimed.

### Staged milestones

- **M0 — data + seam:** curate `registers/microchip/gmac_v1.yaml` (NCR/NCFGR/
  DCFGR/TBQB/RBQB/MAN/ISR/SA1B) + add the `gmac` peripheral to
  `atsame70q21.yaml` + define the `phy` data shape; add NetDevice/Mdio/Socket
  concepts; teach build.py package source sets; CI compiles a guarded stub.
- **M1 — MVP, silicon:** `microchip_gmac_v1` NetDevice driver (static rings +
  MDIO) + `ksz8081` PHY driver + lwIP NO_SYS + `alloy-net` Socket +
  `examples/net_echo` → **DHCP lease + TCP echo on the physical SAME70
  Xplained** (closes the pending network HW-validation note). First
  `connectivity: silicon` board.
- **M2 — portability proof:** bring up `nucleo_f722ze` (STM32F7 ETH MAC,
  different PHY) with the SAME `net_echo` and SAME stack — one board.json line —
  proving the thesis for connectivity across two owned-MAC families.
- **M3 — blob path:** `alloy-wifi` implementing NetDevice over the ESP32 blob;
  `wifi_scan` + reuse `net_echo`. Stack/sockets/examples/poll-loop unchanged —
  WiFi becomes additive, the blob contained to one package satisfying an
  already-validated concept.

## 4. Peripheral-coverage roadmap (the other half of the ask)

Today: 7 driver categories (gpio/uart/spi/i2c/adc/dma/pwm). G0 is the only
complete family; F7/G4/SAME70/RP2040/ESP32 have gaps. Prioritized by
value-per-work × family reach (the tag-dispatch multiplier: one driver per IP
version serves every chip that shares the tag):

1. **FDCAN / M_CAN** [medium] — reach G4 (×3) + SAME70 (×2). One mailbox/FIFO
   driver opens CAN-FD across two families. Best multiplier.
2. **RNG** [easy] — F7 + G4. ~30 lines (poll DRDY, read DR).
3. **CRC** [easy] — F7 + G4. Trivial; OTA/bootloader checksums.
4. **DAC** [easy, 2 versions] — F7 (dac:v2) + G4 (dac:v7). Pairs with ADC.
5. **IWDG** [easy] — F7 + G4 + G0. Table-stakes reliability.
6. **Ethernet + lwIP** [hard, strategic] — the connectivity headline above
   (SAME70 GMAC, then F7 ETH), fully open.
7. **USB device** [hard] — OTG on F4/F7, native on G0/G4/RP2040; needs a device
   stack. After Ethernet proves the package/stack machinery.

CAN + the four easy analog/utility blocks are the next concrete sprint; they
build breadth on the rails we already have while the connectivity milestones
build the new depth.
