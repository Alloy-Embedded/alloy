/**
 * @file dma_simple.hpp
 * @brief Level 1 Simple API for DMA
 *
 * Provides one-liner setup for common DMA transfer patterns.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Simple DMA transfer wrapper
 *
 * @tparam DmaPolicy DMA hardware policy
 */
template <typename DmaPolicy>
class SimpleDmaTransfer {
public:
    constexpr SimpleDmaTransfer(u8 channel) : channel_(channel) {}

    /**
     * @brief Start memory-to-memory transfer
     */
    Result<void, ErrorCode> transfer_mem_to_mem(const void* src, void* dst, u32 size) {
        DmaPolicy::set_source_address(channel_, reinterpret_cast<u32>(src));
        DmaPolicy::set_destination_address(channel_, reinterpret_cast<u32>(dst));
        DmaPolicy::set_transfer_size(channel_, size);
        DmaPolicy::enable_channel(channel_);
        return Ok();
    }

    /**
     * @brief Start memory-to-peripheral transfer
     */
    Result<void, ErrorCode> transfer_mem_to_periph(const void* src, volatile void* periph, u32 size) {
        DmaPolicy::set_source_address(channel_, reinterpret_cast<u32>(src));
        DmaPolicy::set_destination_address(channel_, reinterpret_cast<u32>(periph));
        DmaPolicy::set_transfer_size(channel_, size);
        DmaPolicy::enable_channel(channel_);
        return Ok();
    }

    /**
     * @brief Start peripheral-to-memory transfer
     */
    Result<void, ErrorCode> transfer_periph_to_mem(volatile const void* periph, void* dst, u32 size) {
        DmaPolicy::set_source_address(channel_, reinterpret_cast<u32>(periph));
        DmaPolicy::set_destination_address(channel_, reinterpret_cast<u32>(dst));
        DmaPolicy::set_transfer_size(channel_, size);
        DmaPolicy::enable_channel(channel_);
        return Ok();
    }

    /**
     * @brief Stop transfer
     */
    Result<void, ErrorCode> stop() {
        DmaPolicy::disable_channel(channel_);
        return Ok();
    }

    /**
     * @brief Check if transfer complete
     */
    bool is_complete() const {
        return DmaPolicy::is_transfer_complete(channel_);
    }

    /**
     * @brief Check if busy
     */
    bool is_busy() const {
        return DmaPolicy::is_busy(channel_);
    }

    u8 get_channel() const { return channel_; }

private:
    u8 channel_;
};

/**
 * @brief Simple DMA API
 *
 * Example:
 * @code
 * auto dma = Dma<DmaHardware>::mem_to_mem(0);
 * u8 src[100], dst[100];
 * dma.transfer_mem_to_mem(src, dst, 100);
 * while (!dma.is_complete()) {}
 * @endcode
 */
template <typename DmaPolicy>
class Dma {
public:
    static SimpleDmaTransfer<DmaPolicy> mem_to_mem(u8 channel) {
        return SimpleDmaTransfer<DmaPolicy>(channel);
    }

    static SimpleDmaTransfer<DmaPolicy> mem_to_periph(u8 channel) {
        return SimpleDmaTransfer<DmaPolicy>(channel);
    }

    static SimpleDmaTransfer<DmaPolicy> periph_to_mem(u8 channel) {
        return SimpleDmaTransfer<DmaPolicy>(channel);
    }

    static SimpleDmaTransfer<DmaPolicy> channel(u8 ch) {
        return SimpleDmaTransfer<DmaPolicy>(ch);
    }
};

}  // namespace alloy::hal
