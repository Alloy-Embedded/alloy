/**
 * @file dma_fluent.hpp
 * @brief Level 2 Fluent API for DMA
 *
 * Provides chainable builder pattern for DMA configuration.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/api/dma_simple.hpp"

namespace alloy::hal {

using namespace alloy::core;

struct DmaBuilderState {
    bool has_channel = false;
    bool has_source = false;
    bool has_destination = false;
    bool has_size = false;

    constexpr bool is_valid() const {
        return has_channel && has_source && has_destination && has_size;
    }
};

template <typename DmaPolicy>
struct FluentDmaConfig {
    SimpleDmaTransfer<DmaPolicy> transfer;

    Result<void, ErrorCode> start() {
        DmaPolicy::enable_channel(transfer.get_channel());
        return Ok();
    }

    Result<void, ErrorCode> stop() { return transfer.stop(); }
    bool is_complete() const { return transfer.is_complete(); }
    bool is_busy() const { return transfer.is_busy(); }
};

/**
 * @brief Fluent DMA configuration builder
 *
 * Example:
 * @code
 * u8 src[100], dst[100];
 * auto dma = DmaBuilder<DmaHardware>()
 *     .channel(0)
 *     .from_memory(src)
 *     .to_memory(dst)
 *     .transfer_size(100)
 *     .initialize();
 * dma.value().start();
 * @endcode
 */
template <typename DmaPolicy>
class DmaBuilder {
public:
    constexpr DmaBuilder() : channel_(0), src_(0), dst_(0), size_(0), state_() {}

    constexpr DmaBuilder& channel(u8 ch) {
        channel_ = ch;
        state_.has_channel = true;
        return *this;
    }

    constexpr DmaBuilder& from_memory(const void* src) {
        src_ = reinterpret_cast<u32>(src);
        state_.has_source = true;
        return *this;
    }

    constexpr DmaBuilder& from_peripheral(volatile const void* periph) {
        src_ = reinterpret_cast<u32>(periph);
        state_.has_source = true;
        return *this;
    }

    constexpr DmaBuilder& to_memory(void* dst) {
        dst_ = reinterpret_cast<u32>(dst);
        state_.has_destination = true;
        return *this;
    }

    constexpr DmaBuilder& to_peripheral(volatile void* periph) {
        dst_ = reinterpret_cast<u32>(periph);
        state_.has_destination = true;
        return *this;
    }

    constexpr DmaBuilder& transfer_size(u32 size) {
        size_ = size;
        state_.has_size = true;
        return *this;
    }

    Result<FluentDmaConfig<DmaPolicy>, ErrorCode> initialize() const {
        if (!state_.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        SimpleDmaTransfer<DmaPolicy> transfer(channel_);

        DmaPolicy::set_source_address(channel_, src_);
        DmaPolicy::set_destination_address(channel_, dst_);
        DmaPolicy::set_transfer_size(channel_, size_);

        return Ok(FluentDmaConfig<DmaPolicy>{std::move(transfer)});
    }

private:
    u8 channel_;
    u32 src_;
    u32 dst_;
    u32 size_;
    DmaBuilderState state_;
};

}  // namespace alloy::hal
