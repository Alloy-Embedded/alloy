/**
 * @file dma_expert.hpp
 * @brief Level 3 Expert API for DMA
 *
 * Provides full control over DMA configuration.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::hal {

using namespace alloy::core;

enum class DmaTransferWidth : u8 {
    Byte = 0,      ///< 8-bit transfers
    HalfWord = 1,  ///< 16-bit transfers
    Word = 2       ///< 32-bit transfers
};

enum class DmaTransferType : u8 {
    MemToMem = 0,     ///< Memory to memory
    MemToPeriph = 1,  ///< Memory to peripheral
    PeriphToMem = 2,  ///< Peripheral to memory
    PeriphToPeriph = 3 ///< Peripheral to peripheral
};

enum class DmaPriority : u8 {
    Low = 0,
    Medium = 1,
    High = 2,
    VeryHigh = 3
};

/**
 * @brief Expert DMA configuration
 */
struct DmaExpertConfig {
    u8 channel;                    ///< Channel number (0-23)
    u32 source_addr;               ///< Source address
    u32 dest_addr;                 ///< Destination address
    u32 transfer_count;            ///< Number of transfers
    DmaTransferWidth src_width;    ///< Source data width
    DmaTransferWidth dst_width;    ///< Destination data width
    DmaTransferType transfer_type; ///< Transfer type
    DmaPriority priority;          ///< Channel priority
    bool src_increment;            ///< Increment source address
    bool dst_increment;            ///< Increment destination address
    bool circular_mode;            ///< Circular buffer mode
    bool enable_interrupt;         ///< Enable transfer complete interrupt

    constexpr bool is_valid() const {
        if (channel > 23) return false;
        if (transfer_count == 0) return false;
        if (source_addr == 0 || dest_addr == 0) return false;
        return true;
    }

    constexpr const char* error_message() const {
        if (channel > 23) return "Channel must be 0-23";
        if (transfer_count == 0) return "Transfer count cannot be zero";
        if (source_addr == 0) return "Source address cannot be NULL";
        if (dest_addr == 0) return "Destination address cannot be NULL";
        return "Valid";
    }

    // Factory methods
    static constexpr DmaExpertConfig mem_to_mem(
        u8 ch, const void* src, void* dst, u32 count) {

        return DmaExpertConfig{
            .channel = ch,
            .source_addr = reinterpret_cast<u32>(src),
            .dest_addr = reinterpret_cast<u32>(dst),
            .transfer_count = count,
            .src_width = DmaTransferWidth::Word,
            .dst_width = DmaTransferWidth::Word,
            .transfer_type = DmaTransferType::MemToMem,
            .priority = DmaPriority::Medium,
            .src_increment = true,
            .dst_increment = true,
            .circular_mode = false,
            .enable_interrupt = false
        };
    }

    static constexpr DmaExpertConfig mem_to_periph(
        u8 ch, const void* src, volatile void* periph, u32 count) {

        return DmaExpertConfig{
            .channel = ch,
            .source_addr = reinterpret_cast<u32>(src),
            .dest_addr = reinterpret_cast<u32>(periph),
            .transfer_count = count,
            .src_width = DmaTransferWidth::Byte,
            .dst_width = DmaTransferWidth::Byte,
            .transfer_type = DmaTransferType::MemToPeriph,
            .priority = DmaPriority::High,
            .src_increment = true,
            .dst_increment = false,  // Peripheral address fixed
            .circular_mode = false,
            .enable_interrupt = true
        };
    }

    static constexpr DmaExpertConfig periph_to_mem(
        u8 ch, volatile const void* periph, void* dst, u32 count) {

        return DmaExpertConfig{
            .channel = ch,
            .source_addr = reinterpret_cast<u32>(periph),
            .dest_addr = reinterpret_cast<u32>(dst),
            .transfer_count = count,
            .src_width = DmaTransferWidth::Byte,
            .dst_width = DmaTransferWidth::Byte,
            .transfer_type = DmaTransferType::PeriphToMem,
            .priority = DmaPriority::High,
            .src_increment = false,  // Peripheral address fixed
            .dst_increment = true,
            .circular_mode = false,
            .enable_interrupt = true
        };
    }

    static constexpr DmaExpertConfig circular_buffer(
        u8 ch, const void* src, void* dst, u32 count) {

        return DmaExpertConfig{
            .channel = ch,
            .source_addr = reinterpret_cast<u32>(src),
            .dest_addr = reinterpret_cast<u32>(dst),
            .transfer_count = count,
            .src_width = DmaTransferWidth::Byte,
            .dst_width = DmaTransferWidth::Byte,
            .transfer_type = DmaTransferType::MemToMem,
            .priority = DmaPriority::Medium,
            .src_increment = true,
            .dst_increment = true,
            .circular_mode = true,
            .enable_interrupt = true
        };
    }
};

namespace expert {

template <typename DmaPolicy>
Result<void, ErrorCode> configure(const DmaExpertConfig& config) {
    if (!config.is_valid()) {
        return Err(ErrorCode::InvalidParameter);
    }

    // Set addresses
    DmaPolicy::set_source_address(config.channel, config.source_addr);
    DmaPolicy::set_destination_address(config.channel, config.dest_addr);
    DmaPolicy::set_transfer_size(config.channel, config.transfer_count);

    // Configure channel (platform-specific)
    u32 channel_config = 0;
    // TODO: Build config word based on settings
    DmaPolicy::configure_channel(config.channel, channel_config);

    // Enable interrupt if requested
    if (config.enable_interrupt) {
        DmaPolicy::enable_interrupt(config.channel);
    }

    return Ok();
}

}  // namespace expert

}  // namespace alloy::hal
