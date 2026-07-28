// Portability concepts — the horizontal contract every driver and every
// piece of generic user code constrains on (mirrors embedded-hal 1.0).
// First-party drivers may ONLY name these concepts, never a chip or family.

#pragma once

#include <concepts>
#include <cstdint>
#include <span>

namespace alloy {

template <class T>
concept OutputPin = requires(const T p) {
    p.set_high();
    p.set_low();
    p.toggle();
};

template <class T>
concept InputPin = requires(const T p) {
    { p.is_high() } -> std::same_as<bool>;
};

template <class T>
concept ByteSink = requires(T& s, std::uint8_t b, const char* z) {
    s.write(b);
    s.write(z);
};

template <class T>
concept ByteSource = requires(T& s, std::uint8_t& b) {
    { s.read(b) } -> std::same_as<bool>;
};

template <class T>
concept ByteStream = ByteSink<T> && ByteSource<T>;

template <class T>
concept DelayNs = requires(T d, std::uint32_t ns) {
    d.delay_ns(ns);
};

// I2C controller. bool results are honest v1 error reporting: false = NACK
// or bus error (NACK detection is THE error that matters on I2C).
template <class T>
concept I2cBus = requires(const T& b, std::uint8_t addr, std::span<std::uint8_t> rd,
                          std::span<const std::uint8_t> wr) {
    { b.write(addr, wr) } -> std::same_as<bool>;
    { b.read(addr, rd) } -> std::same_as<bool>;
    { b.write_read(addr, wr, rd) } -> std::same_as<bool>;
};

// SPI controller (bus only; chip-selects are plain OutputPins — the
// SpiDevice sharing layer comes later, embedded-hal style).
// Checked on const T& because handles are stateless and drivers hold them
// by const reference — a conforming bus must expose const methods.
template <class T>
concept SpiBus = requires(const T& b, std::uint8_t byte, std::span<std::uint8_t> buf,
                          std::span<const std::uint8_t> cbuf) {
    { b.xfer(byte) } -> std::same_as<std::uint8_t>;
    b.write(cbuf);
    b.transfer(buf);  // in-place: TX buf, RX overwrites it
};

// PWM output with normalized 16-bit duty (0 = off, 0xFFFF = full on).
template <class T>
concept PwmChannel = requires(T& p, std::uint16_t duty) {
    p.set_duty(duty);
    p.off();
};

// Watchdog timer: feed() restarts the countdown; failing to feed in time
// resets the MCU. The timeout is set at configuration (board data / start()),
// so the runtime contract every driver shares is just the periodic kick.
template <class T>
concept Watchdog = requires(const T w) {
    w.feed();
};

// A flash controller: erase a page, program 64-bit double-words. Reads are
// memory-mapped so they are not part of the contract. The key/value store
// (alloy/util/nvm_kv.hpp) is written against exactly this.
template <class T>
concept FlashController =
    requires(const T f, std::uintptr_t addr, const std::uint64_t* data, std::size_t n) {
        { f.erase_page(addr) } -> std::same_as<bool>;
        { f.program(addr, data, n) } -> std::same_as<bool>;
    };

// A block device for a log-structured filesystem (alloy/fs, littlefs): fixed
// erase blocks, programmed in prog_size units, read in read_size units, both
// addressed by (block, offset). A false return is an I/O error; sync flushes to
// media. Geometry is reported at RUN TIME because the reserved region size
// varies per board. alloy/fs/littlefs.hpp adapts both internal MCU flash and
// external SPI-NOR onto this contract.
template <class T>
concept BlockDevice =
    requires(const T d, std::uint32_t block, std::uint32_t off,
             void* rbuf, const void* wbuf, std::size_t n) {
        { d.bd_read(block, off, rbuf, n) } -> std::same_as<bool>;
        { d.bd_prog(block, off, wbuf, n) } -> std::same_as<bool>;
        { d.bd_erase(block) } -> std::same_as<bool>;
        { d.bd_sync() } -> std::same_as<bool>;
        { d.block_size() } -> std::convertible_to<std::uint32_t>;
        { d.block_count() } -> std::convertible_to<std::uint32_t>;
        { d.read_size() } -> std::convertible_to<std::uint32_t>;
        { d.prog_size() } -> std::convertible_to<std::uint32_t>;
    };

// Wall-clock date + time an RTC keeps. Plain binary fields (NOT BCD) — the
// driver does the BCD conversion at the register edge, so user code never sees
// nibble-packed values.
struct datetime {
    std::uint8_t hour{};
    std::uint8_t minute{};
    std::uint8_t second{};
    std::uint8_t day{1};    // 1-31
    std::uint8_t month{1};  // 1-12
    std::uint8_t year{};    // 0-99 (i.e. 20YY)
};

// A real-time clock/calendar: set the wall clock, read it back. The clock keeps
// running (in the backup domain) across resets once configured.
template <class T>
concept Rtc = requires(T& r, datetime dt) {
    r.set(dt);
    { r.now() } -> std::same_as<datetime>;
};

// A digital-to-analog output channel: enable it, then write a raw code that the
// hardware converts to a voltage on its output pin.
template <class T>
concept DacChannel = requires(T& d, std::uint16_t code) {
    d.enable();
    d.write(code);
};

// A classic-CAN frame: 11-bit standard identifier + up to 8 data bytes.
struct can_frame {
    std::uint32_t id{};        // standard 11-bit identifier
    std::uint8_t len{};        // data length, 0..8
    std::uint8_t data[8]{};
};

// A CAN controller: bring the bus up, transmit a frame, poll for a received
// one. receive() returns false when nothing is waiting (non-blocking).
template <class T>
concept CanBus = requires(T& c, const can_frame& tx, can_frame& rx) {
    c.enable();
    { c.send(tx) } -> std::same_as<bool>;
    { c.receive(rx) } -> std::same_as<bool>;
};

// ── Connectivity (M0 seam; drivers land in M1) ──────────────────────────
//
// These are the horizontal contracts the optional alloy-net layer binds to.
// An L2 MAC we own (GMAC/ETH) satisfies NetDevice; a vendor blob (ESP32
// WiFi) satisfies the SAME NetDevice upward, so the net stack above is
// link-agnostic. Kept poll-driven and zero-alloc at this seam — buffers are
// the caller's; no heap crosses this boundary.

// Clause-22 SMI/MDIO station management — how a MAC reaches its PHY.
template <class T>
concept Mdio = requires(T& m, std::uint8_t phy, std::uint8_t reg, std::uint16_t val) {
    { m.read(phy, reg) } -> std::same_as<std::uint16_t>;
    m.write(phy, reg, val);
};

// L2 network device: a link-layer MAC presented to the stack. RX/TX move
// whole Ethernet frames through caller-owned spans; poll_link reports
// up/down + negotiated speed/duplex. Modeled on embassy-net-driver.
template <class T>
concept NetDevice = requires(T& d, std::span<std::uint8_t> rx,
                             std::span<const std::uint8_t> tx) {
    { d.receive(rx) } -> std::same_as<std::uint32_t>;  // bytes received, 0 = none
    { d.transmit(tx) } -> std::same_as<bool>;          // false = no TX buffer free
    { d.link_up() } -> std::same_as<bool>;
    { d.mtu() } -> std::same_as<std::uint32_t>;
};

// Byte-stream socket the alloy-net front end exposes to user code (M1+).
template <class T>
concept Socket = requires(T& s, std::span<std::uint8_t> rx,
                          std::span<const std::uint8_t> tx) {
    { s.send(tx) } -> std::same_as<std::uint32_t>;
    { s.recv(rx) } -> std::same_as<std::uint32_t>;
    { s.connected() } -> std::same_as<bool>;
    s.close();
};

// An IPv4 endpoint: address + port. `addr` is a packed v4 in HOST byte order
// (a.b.c.d -> (a<<24)|(b<<16)|(c<<8)|d); `port` is host order. A plain POD with
// defaults on every field (like datetime/can_frame), paired with DatagramSocket
// the way can_frame is paired with CanBus. Byte order is this exact shift form —
// do NOT ntohl/htonl the packed word.
struct ip_endpoint {
    std::uint32_t addr{};  // packed IPv4, host byte order
    std::uint16_t port{};  // host byte order
};

// Connectionless, message-oriented socket (UDP) the alloy-net front end exposes
// to user code (M2+). Unlike the byte-stream Socket, every datagram carries its
// own peer: recv_from yields the SENDER's endpoint, send_to targets one. There
// is no connection to be "connected" — bound() reports whether the local port is
// claimed. Byte counts are whole-datagram, 0 = none (non-blocking, like Socket).
template <class T>
concept DatagramSocket = requires(T& s, std::span<std::uint8_t> rx,
                                  std::span<const std::uint8_t> tx,
                                  const ip_endpoint& to, ip_endpoint& from) {
    { s.send_to(tx, to) }     -> std::same_as<std::uint32_t>;
    { s.recv_from(rx, from) } -> std::same_as<std::uint32_t>;
    { s.bound() }             -> std::same_as<bool>;
    s.close();
};

}  // namespace alloy
