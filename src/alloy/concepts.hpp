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

}  // namespace alloy
