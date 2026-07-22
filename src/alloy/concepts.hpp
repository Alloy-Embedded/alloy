// Portability concepts — the horizontal contract every driver and every
// piece of generic user code constrains on (mirrors embedded-hal 1.0).
// First-party drivers may ONLY name these concepts, never a chip or family.

#pragma once

#include <concepts>
#include <cstdint>

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

}  // namespace alloy
