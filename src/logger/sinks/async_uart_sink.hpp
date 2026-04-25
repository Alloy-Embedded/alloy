#pragma once

#include <cstddef>
#include <span>

#include "logger/sinks/ring_buffer_sink.hpp"

namespace alloy::logger {

template <typename UartImpl, size_t Capacity = 512, size_t MaxRecords = 16>
class AsyncUartSink {
   public:
    struct PumpStats {
        size_t records_drained = 0u;
        size_t bytes_written = 0u;
        bool blocked = false;
        bool transport_error = false;
    };

    explicit AsyncUartSink(UartImpl& uart) : uart_(uart) {}

    auto write(const char* data, size_t size) -> void { ring_.write(std::string_view{data, size}); }

    auto write(std::string_view text) -> void { ring_.write(text); }

    auto write_record(const RecordView& record) -> void { ring_.write_record(record); }

    auto flush() -> void {
        static_cast<void>(pump_all());
        const auto result = uart_.flush();
        if (result.is_err()) {
            ready_ = false;
        }
    }

    [[nodiscard]] auto is_ready() const -> bool { return ready_; }

    auto set_enabled(bool enabled) -> void { ready_ = enabled; }

    [[nodiscard]] auto pending_records() const -> size_t { return ring_.record_count(); }

    [[nodiscard]] auto dropped_records() const -> size_t { return ring_.dropped_records(); }

    [[nodiscard]] auto pump_one() -> PumpStats { return pump(1u); }

    [[nodiscard]] auto pump_all() -> PumpStats { return pump(ring_.record_count()); }

    [[nodiscard]] auto pump(size_t max_records) -> PumpStats {
        PumpStats stats{};
        if (!ready_ || max_records == 0u) {
            return stats;
        }

        while (stats.records_drained < max_records && ring_.record_count() > 0u) {
            const auto snapshot = ring_.oldest();
            const auto write_result = write_rendered(snapshot.rendered);
            if (write_result == 0u) {
                stats.blocked = true;
                stats.transport_error = !ready_;
                break;
            }
            stats.bytes_written += write_result;
            ring_.drop_oldest();
            ++stats.records_drained;
        }
        return stats;
    }

   private:
    auto write_rendered(std::string_view rendered) -> size_t {
        if (rendered.empty()) {
            return 0u;
        }

        if constexpr (requires(UartImpl& uart, std::span<const std::byte> bytes) {
                          { uart.write(bytes) };
                      }) {
            const auto bytes = std::span{
                reinterpret_cast<const std::byte*>(rendered.data()),
                rendered.size(),
            };
            const auto result = uart_.write(bytes);
            if (result.is_err()) {
                ready_ = false;
                return 0u;
            }
            return result.unwrap();
        } else {
            for (const auto ch : rendered) {
                const auto result =
                    uart_.write_byte(static_cast<std::byte>(static_cast<unsigned char>(ch)));
                if (result.is_err()) {
                    ready_ = false;
                    return 0u;
                }
            }
            return rendered.size();
        }
    }
    UartImpl& uart_;
    RingBufferSink<Capacity, MaxRecords> ring_{};
    bool ready_ = true;
};

template <typename UartImpl, size_t Capacity = 512, size_t MaxRecords = 16>
auto make_async_uart_sink(UartImpl& uart) -> AsyncUartSink<UartImpl, Capacity, MaxRecords> {
    return AsyncUartSink<UartImpl, Capacity, MaxRecords>(uart);
}

}  // namespace alloy::logger
