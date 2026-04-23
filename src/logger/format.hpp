#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <string_view>

#include "types.hpp"

namespace alloy::logger {

namespace detail {

class BufferWriter {
   public:
    constexpr BufferWriter(char* buffer, size_t capacity) : buffer_(buffer), capacity_(capacity) {
        if (buffer_ != nullptr && capacity_ > 0u) {
            buffer_[0] = '\0';
        }
    }

    [[nodiscard]] constexpr auto size() const -> size_t { return size_; }

    [[nodiscard]] constexpr auto remaining() const -> size_t {
        return (capacity_ > size_) ? (capacity_ - size_) : 0u;
    }

    auto append_char(char value) -> void {
        if (buffer_ == nullptr || remaining() <= 1u) {
            return;
        }
        buffer_[size_++] = value;
        buffer_[size_] = '\0';
    }

    auto append(std::string_view text) -> void {
        for (const char value : text) {
            append_char(value);
        }
    }

    auto append_unsigned(std::uint64_t value) -> void {
        char scratch[20]{};
        size_t count = 0u;
        do {
            scratch[count++] = static_cast<char>('0' + (value % 10u));
            value /= 10u;
        } while (value != 0u && count < sizeof(scratch));

        while (count > 0u) {
            append_char(scratch[--count]);
        }
    }

    auto append_zero_padded(std::uint32_t value, size_t width) -> void {
        char scratch[10]{};
        size_t count = 0u;
        do {
            scratch[count++] = static_cast<char>('0' + (value % 10u));
            value /= 10u;
        } while (value != 0u && count < sizeof(scratch));

        while (count < width && count < sizeof(scratch)) {
            scratch[count++] = '0';
        }

        while (count > 0u) {
            append_char(scratch[--count]);
        }
    }

   private:
    char* buffer_ = nullptr;
    size_t capacity_ = 0u;
    size_t size_ = 0u;
};

inline auto level_color(Level level) -> const char* {
    switch (level) {
        case Level::Trace:
            return "\033[90m";
        case Level::Debug:
            return "\033[36m";
        case Level::Info:
            return "\033[32m";
        case Level::Warn:
            return "\033[33m";
        case Level::Error:
            return "\033[31m";
        default:
            return "\033[0m";
    }
}

inline auto filename_from_path(const char* path) -> const char* {
    if (path == nullptr) {
        return "";
    }

    const char* filename = path;
    for (const char* cursor = path; *cursor != '\0'; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            filename = cursor + 1;
        }
    }
    return filename;
}

}  // namespace detail

/**
 * Format utilities for logger messages.
 *
 * Prefix formatting is done with a bounded writer so truncation never causes
 * the cursor to walk past the actual buffer capacity.
 */
class Formatter {
   public:
    static auto format_timestamp(char* buffer, size_t size, std::uint64_t timestamp_us,
                                 TimestampPrecision precision) -> size_t {
        detail::BufferWriter writer(buffer, size);

        if (timestamp_us == 0u) {
            writer.append("[BOOT] ");
            return writer.size();
        }

        const auto seconds = timestamp_us / 1'000'000ull;
        const auto micros = static_cast<std::uint32_t>(timestamp_us % 1'000'000ull);

        writer.append_char('[');
        writer.append_unsigned(seconds);

        switch (precision) {
            case TimestampPrecision::Seconds:
                break;
            case TimestampPrecision::Milliseconds:
                writer.append_char('.');
                writer.append_zero_padded(micros / 1'000u, 3u);
                break;
            case TimestampPrecision::Microseconds:
                writer.append_char('.');
                writer.append_zero_padded(micros, 6u);
                break;
        }

        writer.append("] ");
        return writer.size();
    }

    static auto format_level(char* buffer, size_t size, Level level, bool use_colors) -> size_t {
        detail::BufferWriter writer(buffer, size);

        if (use_colors) {
            writer.append(detail::level_color(level));
            writer.append(level_to_short_string(level));
            writer.append("\033[0m ");
            return writer.size();
        }

        writer.append(level_to_short_string(level));
        writer.append_char(' ');
        return writer.size();
    }

    static auto format_source_location(char* buffer, size_t size, const char* file, int line)
        -> size_t {
        detail::BufferWriter writer(buffer, size);
        writer.append_char('[');
        writer.append(detail::filename_from_path(file));
        writer.append_char(':');
        writer.append_unsigned(line < 0 ? 0u : static_cast<std::uint32_t>(line));
        writer.append("] ");
        return writer.size();
    }

    static auto format_prefix(char* buffer, size_t size, std::uint64_t timestamp_us, Level level,
                              SourceLocation source_location, const Config& config) -> size_t {
        detail::BufferWriter writer(buffer, size);

        if (config.enable_timestamps) {
            char local[32]{};
            const auto timestamp_len =
                format_timestamp(local, sizeof(local), timestamp_us, config.timestamp_precision);
            writer.append(std::string_view{local, timestamp_len});
        }

        {
            char local[32]{};
            const auto level_len =
                format_level(local, sizeof(local), level, config.enable_colors);
            writer.append(std::string_view{local, level_len});
        }

        if (config.enable_source_location && source_location.file != nullptr) {
            char local[96]{};
            const auto source_len = format_source_location(local, sizeof(local), source_location.file,
                                                           source_location.line);
            writer.append(std::string_view{local, source_len});
        }

        return writer.size();
    }

    static auto format_message(char* buffer, size_t size, const char* fmt, va_list args) -> size_t {
        if (buffer == nullptr || size == 0u) {
            return 0u;
        }

        const int written = vsnprintf(buffer, size, fmt, args);
        if (written <= 0) {
            buffer[0] = '\0';
            return 0u;
        }

        const auto normalized = static_cast<size_t>(written);
        if (normalized >= size) {
            return size - 1u;
        }
        return normalized;
    }

    static auto append_line_ending(char* buffer, size_t size, LineEnding line_ending) -> size_t {
        detail::BufferWriter writer(buffer, size);
        switch (line_ending) {
            case LineEnding::None:
                break;
            case LineEnding::LF:
                writer.append_char('\n');
                break;
            case LineEnding::CRLF:
                writer.append("\r\n");
                break;
        }
        return writer.size();
    }
};

}  // namespace alloy::logger
