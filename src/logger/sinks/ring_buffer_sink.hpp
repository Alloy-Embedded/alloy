#pragma once

#include <array>
#include <string_view>

#include "logger/types.hpp"

namespace alloy::logger {

/**
 * Fixed-capacity record-aware ring sink.
 *
 * Records are stored contiguously in a byte ring plus lightweight descriptors,
 * so the sink keeps recent logs without heap allocation and without forcing the
 * rest of the system to format twice.
 */
template <size_t Capacity, size_t MaxRecords = 16>
class RingBufferSink {
   public:
    static_assert(Capacity > 0u, "RingBufferSink requires non-zero byte capacity");
    static_assert(MaxRecords > 0u, "RingBufferSink requires non-zero record capacity");

    struct Snapshot {
        Level level = Level::Info;
        std::uint64_t timestamp_us = 0u;
        SourceLocation source_location{};
        std::string_view payload{};
        std::string_view rendered{};
    };

    auto write(std::string_view text) -> void {
        write_record({
            .rendered = text,
        });
    }

    auto write_record(const RecordView& record) -> void {
        if (record.rendered.empty()) {
            return;
        }

        const size_t stored_size =
            (record.rendered.size() < Capacity) ? record.rendered.size() : Capacity;

        while (count_ >= MaxRecords) {
            evict_oldest();
        }

        while (used_bytes_ + stored_size > Capacity) {
            evict_oldest();
        }

        auto start = reserve_write_region(stored_size);
        if (!start.has_value()) {
            clear();
            start = {.value = 0u, .valid = true};
        }

        for (size_t index = 0; index < stored_size; ++index) {
            storage_[*start + index] = record.rendered[index];
        }

        const auto descriptor_index = (head_ + count_) % MaxRecords;
        descriptors_[descriptor_index] = make_descriptor(record, *start, stored_size);

        if (count_ == 0u) {
            read_pos_ = *start;
        }

        ++count_;
        used_bytes_ += stored_size;
        write_pos_ = (*start + stored_size) % Capacity;
    }

    auto flush() -> void {}

    [[nodiscard]] auto is_ready() const -> bool { return true; }

    auto clear() -> void {
        head_ = 0u;
        count_ = 0u;
        read_pos_ = 0u;
        write_pos_ = 0u;
        used_bytes_ = 0u;
        dropped_records_ = 0u;
        for (auto& descriptor : descriptors_) {
            descriptor = {};
        }
    }

    [[nodiscard]] auto empty() const -> bool { return count_ == 0u; }

    [[nodiscard]] auto record_count() const -> size_t { return count_; }

    [[nodiscard]] auto dropped_records() const -> size_t { return dropped_records_; }

    [[nodiscard]] auto latest() const -> Snapshot { return record_from_latest(0u); }

    [[nodiscard]] auto oldest() const -> Snapshot {
        if (count_ == 0u) {
            return {};
        }

        const auto& descriptor = descriptors_[head_];
        return Snapshot{
            .level = descriptor.level,
            .timestamp_us = descriptor.timestamp_us,
            .source_location = descriptor.source_location,
            .payload =
                std::string_view{storage_.data() + descriptor.offset + descriptor.payload_offset,
                                 descriptor.payload_length},
            .rendered = std::string_view{storage_.data() + descriptor.offset, descriptor.rendered_length},
        };
    }

    [[nodiscard]] auto record_from_latest(size_t reverse_index) const -> Snapshot {
        if (reverse_index >= count_) {
            return {};
        }

        const auto descriptor_index = (head_ + count_ - 1u - reverse_index) % MaxRecords;
        const auto& descriptor = descriptors_[descriptor_index];
        return Snapshot{
            .level = descriptor.level,
            .timestamp_us = descriptor.timestamp_us,
            .source_location = descriptor.source_location,
            .payload =
                std::string_view{storage_.data() + descriptor.offset + descriptor.payload_offset,
                                 descriptor.payload_length},
            .rendered = std::string_view{storage_.data() + descriptor.offset, descriptor.rendered_length},
        };
    }

    auto drop_oldest() -> void { evict_oldest(); }

   private:
    struct Descriptor {
        size_t offset = 0u;
        size_t rendered_length = 0u;
        size_t payload_offset = 0u;
        size_t payload_length = 0u;
        Level level = Level::Info;
        std::uint64_t timestamp_us = 0u;
        SourceLocation source_location{};
    };

    struct OptionalOffset {
        bool has_value() const { return valid; }
        auto operator*() const -> size_t { return value; }

        size_t value = 0u;
        bool valid = false;
    };

    auto reserve_write_region(size_t bytes) -> OptionalOffset {
        if (count_ == 0u) {
            write_pos_ = 0u;
            read_pos_ = 0u;
            return {.value = 0u, .valid = true};
        }

        while (true) {
            if (write_pos_ >= read_pos_) {
                const auto tail_free = Capacity - write_pos_;
                if (tail_free >= bytes) {
                    return {.value = write_pos_, .valid = true};
                }
                if (read_pos_ >= bytes) {
                    return {.value = 0u, .valid = true};
                }
            } else {
                const auto gap = read_pos_ - write_pos_;
                if (gap >= bytes) {
                    return {.value = write_pos_, .valid = true};
                }
            }

            if (count_ == 0u) {
                return {.value = 0u, .valid = true};
            }
            evict_oldest();
        }
    }

    auto evict_oldest() -> void {
        if (count_ == 0u) {
            return;
        }

        const auto& descriptor = descriptors_[head_];
        used_bytes_ -= descriptor.rendered_length;
        read_pos_ = (descriptor.offset + descriptor.rendered_length) % Capacity;
        descriptors_[head_] = {};
        head_ = (head_ + 1u) % MaxRecords;
        --count_;
        ++dropped_records_;
        if (count_ == 0u) {
            read_pos_ = 0u;
            write_pos_ = 0u;
            used_bytes_ = 0u;
        }
    }

    static auto make_descriptor(const RecordView& record, size_t offset, size_t stored_size)
        -> Descriptor {
        size_t payload_offset = 0u;
        size_t payload_length = 0u;

        if (!record.payload.empty() && !record.rendered.empty()) {
            const auto rendered_begin = record.rendered.data();
            const auto payload_begin = record.payload.data();
            if (payload_begin >= rendered_begin &&
                payload_begin <= rendered_begin + static_cast<std::ptrdiff_t>(record.rendered.size())) {
                payload_offset = static_cast<size_t>(payload_begin - rendered_begin);
                if (payload_offset < stored_size) {
                    const auto remaining = stored_size - payload_offset;
                    payload_length =
                        (record.payload.size() < remaining) ? record.payload.size() : remaining;
                } else {
                    payload_offset = stored_size;
                }
            }
        }

        return Descriptor{
            .offset = offset,
            .rendered_length = stored_size,
            .payload_offset = payload_offset,
            .payload_length = payload_length,
            .level = record.level,
            .timestamp_us = record.timestamp_us,
            .source_location = record.source_location,
        };
    }

    std::array<char, Capacity> storage_{};
    std::array<Descriptor, MaxRecords> descriptors_{};
    size_t head_ = 0u;
    size_t count_ = 0u;
    size_t read_pos_ = 0u;
    size_t write_pos_ = 0u;
    size_t used_bytes_ = 0u;
    size_t dropped_records_ = 0u;
};

}  // namespace alloy::logger
