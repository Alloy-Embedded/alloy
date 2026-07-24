// Compile-time-gated, level-filtered logger over any ByteSink (the opened
// debug UART, or a host capture buffer). Messages below the logger's minimum
// level compile to NOTHING — the disabled-log-costs-zero-flash property that
// matches alloy's zero-overhead philosophy. No macros and no preprocessor level
// gating (NORTH_STAR guards #2/#3): the minimum level is a template non-type
// parameter and each call site is discarded by `if constexpr`. Constrained on
// the ByteSink concept, never on a concrete UART type.

#pragma once

#include <cstdint>

#include "alloy/concepts.hpp"

namespace alloy::log {

enum class level : std::uint8_t { trace, debug, info, warn, error, off };

template <class Sink, level MinLevel = level::info>
    requires alloy::ByteSink<Sink>
class logger {
    Sink& sink_;

    static constexpr bool enabled(level l) {
        return static_cast<std::uint8_t>(l) >= static_cast<std::uint8_t>(MinLevel);
    }
    static constexpr const char* prefix_of(level l) {
        switch (l) {
            case level::trace: return "[T] ";
            case level::debug: return "[D] ";
            case level::info: return "[I] ";
            case level::warn: return "[W] ";
            case level::error: return "[E] ";
            default: return "";
        }
    }
    void put_hex32(std::uint32_t v) const {
        constexpr char digits[] = "0123456789abcdef";
        sink_.write("0x");
        for (int shift = 28; shift >= 0; shift -= 4) {
            sink_.write(static_cast<std::uint8_t>(digits[(v >> shift) & 0xFu]));
        }
    }

    template <level L>
    void emit(const char* msg) const {
        if constexpr (enabled(L)) {
            sink_.write(prefix_of(L));
            sink_.write(msg);
            sink_.write("\r\n");
        }
    }
    template <level L>
    void emit(const char* msg, std::uint32_t value) const {
        if constexpr (enabled(L)) {
            sink_.write(prefix_of(L));
            sink_.write(msg);
            sink_.write(" ");
            put_hex32(value);
            sink_.write("\r\n");
        }
    }

public:
    explicit logger(Sink& sink) : sink_(sink) {}

    void trace(const char* m) const { emit<level::trace>(m); }
    void debug(const char* m) const { emit<level::debug>(m); }
    void info(const char* m) const { emit<level::info>(m); }
    void warn(const char* m) const { emit<level::warn>(m); }
    void error(const char* m) const { emit<level::error>(m); }

    // Same, with a trailing hex-formatted value (register dumps, counters).
    void trace(const char* m, std::uint32_t v) const { emit<level::trace>(m, v); }
    void debug(const char* m, std::uint32_t v) const { emit<level::debug>(m, v); }
    void info(const char* m, std::uint32_t v) const { emit<level::info>(m, v); }
    void warn(const char* m, std::uint32_t v) const { emit<level::warn>(m, v); }
    void error(const char* m, std::uint32_t v) const { emit<level::error>(m, v); }
};

// Deduce the sink type: `auto log = alloy::log::make<level::debug>(uart);`.
template <level MinLevel = level::info, class Sink>
[[nodiscard]] logger<Sink, MinLevel> make(Sink& sink) {
    return logger<Sink, MinLevel>{sink};
}

}  // namespace alloy::log
