#pragma once

#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

#include "types.hpp"

namespace alloy::logger {

/**
 * Base class for logger output sinks
 *
 * A sink receives formatted log messages and writes them to an output
 * destination (UART, file, network, memory buffer, etc.)
 *
 * Example implementation:
 *
 * class MySink : public Sink {
 * public:
 *     void write(const char* data, size_t length) override {
 *         // Write to your output device
 *         my_device.send(data, length);
 *     }
 *
 *     void flush() override {
 *         // Optional: flush buffered data
 *         my_device.flush();
 *     }
 * };
 */
class Sink {
   public:
    virtual ~Sink() = default;

    /**
     * Write formatted log message to the sink
     *
     * @param data Pointer to formatted message (null-terminated)
     * @param length Length of message in bytes (excluding null terminator)
     *
     * Note: This method will be called from the Logger with mutex held (on RTOS)
     *       Keep implementation fast and non-blocking if possible
     */
    virtual void write(const char* data, size_t length) = 0;

    /**
     * Flush any buffered data (optional)
     *
     * Default implementation does nothing.
     * Override if your sink buffers data.
     */
    virtual void flush() {}

    /**
     * Check if sink is ready to receive data (optional)
     *
     * Default implementation returns true.
     * Override if your sink needs initialization or can be in not-ready state.
     *
     * @return true if sink is ready, false otherwise
     */
    virtual bool is_ready() const { return true; }
};

/**
 * Lightweight non-owning sink reference.
 *
 * This keeps the logger core flexible without forcing every sink to inherit
 * from a virtual base. Existing `Sink` implementations still work, but plain
 * structs with `write()/flush()/is_ready()` are also accepted.
 */
struct SinkRef {
    using WriteFn = void (*)(void* object, const char* data, size_t length);
    using WriteRecordFn = void (*)(void* object, const RecordView& record);
    using FlushFn = void (*)(void* object);
    using ReadyFn = bool (*)(const void* object);

    void* object = nullptr;
    WriteFn write_fn = nullptr;
    WriteRecordFn write_record_fn = nullptr;
    FlushFn flush_fn = nullptr;
    ReadyFn ready_fn = nullptr;

    [[nodiscard]] constexpr auto valid() const -> bool {
        return object != nullptr && write_fn != nullptr;
    }

    [[nodiscard]] constexpr auto identity() const -> const void* { return object; }

    auto write(const char* data, size_t length) const -> void {
        if (valid()) {
            write_fn(object, data, length);
        }
    }

    auto write(std::string_view text) const -> void { write(text.data(), text.size()); }

    auto write_record(const RecordView& record) const -> void {
        if (!valid()) {
            return;
        }
        if (write_record_fn != nullptr) {
            write_record_fn(object, record);
            return;
        }
        write(record.rendered);
    }

    auto flush() const -> void {
        if (valid() && flush_fn != nullptr) {
            flush_fn(object);
        }
    }

    [[nodiscard]] auto is_ready() const -> bool {
        if (!valid()) {
            return false;
        }
        if (ready_fn == nullptr) {
            return true;
        }
        return ready_fn(object);
    }

    template <typename T>
    static auto from(T& sink) -> SinkRef {
        using SinkType = std::remove_reference_t<T>;
        return SinkRef{
            .object = &sink,
            .write_fn =
                [](void* object, const char* data, size_t length) {
                    auto& typed = *static_cast<SinkType*>(object);
                    if constexpr (requires(SinkType& s, std::string_view text) { s.write(text); }) {
                        typed.write(std::string_view{data, length});
                    } else if constexpr (requires(SinkType& s, const RecordView& record_view) {
                                             s.write_record(record_view);
                                         }) {
                        typed.write_record(RecordView{
                            .rendered = std::string_view{data, length},
                        });
                    } else {
                        typed.write(data, length);
                    }
                },
            .write_record_fn =
                [](void* object, const RecordView& record) {
                    auto& typed = *static_cast<SinkType*>(object);
                    if constexpr (requires(SinkType& s, const RecordView& record_view) {
                                      s.write_record(record_view);
                                  }) {
                        typed.write_record(record);
                    } else if constexpr (requires(SinkType& s, std::string_view text) {
                                             s.write(text);
                                         }) {
                        typed.write(record.rendered);
                    } else {
                        typed.write(record.rendered.data(), record.rendered.size());
                    }
                },
            .flush_fn =
                [](void* object) {
                    auto& typed = *static_cast<SinkType*>(object);
                    if constexpr (requires(SinkType& s) { s.flush(); }) {
                        typed.flush();
                    } else {
                        static_cast<void>(typed);
                    }
                },
            .ready_fn =
                [](const void* object) -> bool {
                    const auto& typed = *static_cast<const SinkType*>(object);
                    if constexpr (requires(const SinkType& s) {
                                      { s.is_ready() } -> std::convertible_to<bool>;
                                  }) {
                        return static_cast<bool>(typed.is_ready());
                    }
                    return true;
                },
        };
    }
};

template <typename T>
auto make_sink_ref(T& sink) -> SinkRef {
    return SinkRef::from(sink);
}

}  // namespace alloy::logger
