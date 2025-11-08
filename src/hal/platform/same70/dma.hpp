/**
 * @file dma.hpp
 * @brief Template-based DMA implementation for SAME70 (Platform Layer)
 *
 * This file implements DMA peripheral using templates with ZERO virtual
 * functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Channel number resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - Error handling: Uses Result<T, ErrorCode> for robust error handling
 * - Generic: Works with any peripheral (ADC, UART, SPI, I2C, etc.)
 * - Testable: Includes test hooks for unit testing
 *
 * Auto-generated from: same70
 * Generator: generate_platform_dma.py
 * Generated: 2025-11-07 18:02:44
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/atmel/same70/registers/xdmac_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/atmel/same70/bitfields/xdmac_bitfields.hpp"


namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfield access
namespace xdmac = alloy::hal::atmel::same70::xdmac;

// ============================================================================
// Platform-Specific Enums
// ============================================================================

/**
 * @brief DMA Transfer Direction
 */
enum class DmaDirection : uint8_t {
    PeripheralToMemory = 0,  ///< Peripheral -> Memory (e.g., ADC -> RAM)
    MemoryToPeripheral = 1,  ///< Memory -> Peripheral (e.g., RAM -> UART)
    MemoryToMemory = 2,  ///< Memory -> Memory (memcpy)
};

/**
 * @brief DMA Transfer Width
 */
enum class DmaWidth : uint8_t {
    Byte = 0,  ///< 8-bit transfers
    HalfWord = 1,  ///< 16-bit transfers
    Word = 2,  ///< 32-bit transfers
};

/**
 * @brief DMA Peripheral IDs for hardware handshaking
 */
enum class DmaPeripheralId : uint8_t {
    TWIHS0_TX = 0,  ///< TWI0 Transmit
    TWIHS0_RX = 1,  ///< TWI0 Receive
    TWIHS1_TX = 2,  ///< TWI1 Transmit
    TWIHS1_RX = 3,  ///< TWI1 Receive
    TWIHS2_TX = 4,  ///< TWI2 Transmit
    TWIHS2_RX = 5,  ///< TWI2 Receive
    SPI0_TX = 6,  ///< SPI0 Transmit
    SPI0_RX = 7,  ///< SPI0 Receive
    SPI1_TX = 8,  ///< SPI1 Transmit
    SPI1_RX = 9,  ///< SPI1 Receive
    USART0_TX = 10,  ///< USART0 Transmit
    USART0_RX = 11,  ///< USART0 Receive
    USART1_TX = 12,  ///< USART1 Transmit
    USART1_RX = 13,  ///< USART1 Receive
    USART2_TX = 14,  ///< USART2 Transmit
    USART2_RX = 15,  ///< USART2 Receive
    AFEC0 = 16,  ///< ADC AFEC0
    AFEC1 = 17,  ///< ADC AFEC1
    DACC = 18,  ///< DAC
    SSC_TX = 19,  ///< SSC Transmit
    SSC_RX = 20,  ///< SSC Receive
    PWM0 = 21,  ///< PWM0
    PWM1 = 22,  ///< PWM1
    MEM = 255,  ///< Memory-to-memory (no peripheral)
};


/**
 * @brief DMA Channel Configuration
 */
struct DmaConfig {
    DmaDirection direction = DmaDirection::PeripheralToMemory;  ///< Transfer direction
    DmaWidth src_width = DmaWidth::Word;  ///< Source data width
    DmaWidth dst_width = DmaWidth::Word;  ///< Destination data width
    DmaPeripheralId peripheral = DmaPeripheralId::MEM;  ///< Peripheral ID for handshaking
    bool src_increment = false;  ///< Increment source address
    bool dst_increment = true;  ///< Increment destination address
    uint8_t burst_size = 1;  ///< Burst size (1, 4, 8, 16)
};

// ============================================================================
// Additional Structures
// ============================================================================

/**
 * @brief XDMAC Channel Registers (per-channel). Each of the 24 DMA channels has its own register block at offset 0x50 + (channel * 0x40)
 */
struct XdmacChannelRegisters {
    volatile uint32_t CIE;  ///< Channel Interrupt Enable
    volatile uint32_t CID;  ///< Channel Interrupt Disable
    volatile uint32_t CIM;  ///< Channel Interrupt Mask
    volatile uint32_t CIS;  ///< Channel Interrupt Status
    volatile uint32_t CSA;  ///< Channel Source Address
    volatile uint32_t CDA;  ///< Channel Destination Address
    volatile uint32_t CNDA;  ///< Channel Next Descriptor Address
    volatile uint32_t CNDC;  ///< Channel Next Descriptor Control
    volatile uint32_t CUBC;  ///< Channel Microblock Control
    volatile uint32_t CBC;  ///< Channel Block Control
    volatile uint32_t CC;  ///< Channel Configuration
    volatile uint32_t CDS_MSP;  ///< Channel Data Stride/Memory Set Pattern
    volatile uint32_t CSUS;  ///< Channel Source Microblock Stride
    volatile uint32_t CDUS;  ///< Channel Destination Microblock Stride
    uint32_t RESERVED[2];  ///< Reserved
};


/**
 * @brief Template-based DMA channel for SAME70
 *
 * This class provides a template-based DMA implementation with ZERO runtime
 * overhead. Each DMA channel is a separate template instance.
 *
 * Template Parameters:
 * - CHANNEL_NUM: DMA channel number (0-23)
 *
 * Example usage:
 * @code
 * // Basic DMA memory copy
 * auto dma = DmaChannel0{};
 * DmaConfig config;
 * config.direction = DmaDirection::MemoryToMemory;
 * config.src_increment = true;
 * config.dst_increment = true;
 * uint32_t src[100], dst[100];
 * dma.open();
 * dma.configure(config);
 * dma.transfer(src, dst, 100);
 * dma.waitComplete();
 * @endcode
 *
 * @tparam CHANNEL_NUM DMA channel number (0-23)
 */
template <uint8_t CHANNEL_NUM>
class DmaChannel {
    static_assert(CHANNEL_NUM < 24, "SAME70 XDMAC has 24 channels (0-23)");
public:
    // Compile-time constants
    static constexpr uint8_t channel_num = CHANNEL_NUM;

    // Configuration constants
    static constexpr uint32_t base_address = 0x40078000;  ///< XDMAC base address
    static constexpr uint32_t channel_offset = 0x50 + (CHANNEL_NUM * 0x40);  ///< Channel register offset

    /**
     * @brief Get XDMAC peripheral registers
     */
static inline volatile alloy::hal::atmel::same70::xdmac::XDMAC_Registers* get_xdmac() {
        #ifdef ALLOY_DMA_MOCK_HW
        return ALLOY_DMA_MOCK_HW();
#else
        return reinterpret_cast<volatile alloy::hal::atmel::same70::xdmac::XDMAC_Registers*>(base_address);
#endif
    }

    /**
     * @brief Get DMA channel registers
     */
static inline volatile XdmacChannelRegisters* get_channel() {
        #ifdef ALLOY_DMA_MOCK_CHANNEL
        return ALLOY_DMA_MOCK_CHANNEL();
#else
        return reinterpret_cast<volatile XdmacChannelRegisters*>(base_address + channel_offset);
#endif
    }


    constexpr DmaChannel() = default;

    /**
     * @brief Enable DMA channel and clock
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> open() {
        if (m_opened) {
            return Err(ErrorCode::AlreadyInitialized);
        }

        // Enable XDMAC clock (peripheral ID 22)
        // TODO: Enable peripheral clock via PMC

        // Disable channel initially
        auto* xdmac = get_xdmac();
        xdmac->GD = (1u << CHANNEL_NUM);

        m_opened = true;

        return Ok();
    }

    /**
     * @brief Close DMA channel
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> close() {
        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Disable channel
        auto* xdmac = get_xdmac();
        xdmac->GD = (1u << CHANNEL_NUM);

        m_opened = false;

        return Ok();
    }

    /**
     * @brief Configure DMA channel
     *
     * @param config DMA configuration
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> configure(const DmaConfig& config) {
        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Build CC register value
        auto* channel = get_channel();
        
        uint32_t cc = 0;
        
        // Transfer type
        if (config.direction == DmaDirection::MemoryToMemory) {
            cc = xdmac::cc::TYPE::write(cc, xdmac::cc::type::MEM_TRAN);
        } else {
            cc = xdmac::cc::TYPE::write(cc, xdmac::cc::type::PER_TRAN);
        }
        
        // Memory burst size (single beat)
        cc = xdmac::cc::MBSIZE::write(cc, xdmac::cc::mbsize::SINGLE);
        
        // Synchronization and peripheral ID
        if (config.peripheral != DmaPeripheralId::MEM) {
            // Peripheral synchronized
            cc = xdmac::cc::DSYNC::write(cc, xdmac::cc::dsync::PER2MEM);
            cc = xdmac::cc::PERID::write(cc, static_cast<uint32_t>(config.peripheral));
        } else {
            // Memory synchronized
            cc = xdmac::cc::DSYNC::write(cc, xdmac::cc::dsync::MEM2PER);
        }
        
        // Source addressing mode
        if (config.src_increment) {
            cc = xdmac::cc::SAM::write(cc, xdmac::cc::sam::INCREMENTED_AM);
        } else {
            cc = xdmac::cc::SAM::write(cc, xdmac::cc::sam::FIXED_AM);
        }
        
        // Destination addressing mode
        if (config.dst_increment) {
            cc = xdmac::cc::DAM::write(cc, xdmac::cc::dam::INCREMENTED_AM);
        } else {
            cc = xdmac::cc::DAM::write(cc, xdmac::cc::dam::FIXED_AM);
        }
        
        // Data width
        uint32_t width_value = xdmac::cc::dwidth::BYTE;
        if (config.src_width == DmaWidth::HalfWord) {
            width_value = xdmac::cc::dwidth::HALFWORD;
        } else if (config.src_width == DmaWidth::Word) {
            width_value = xdmac::cc::dwidth::WORD;
        }
        cc = xdmac::cc::DWIDTH::write(cc, width_value);
        
        // Write configuration
        channel->CC = cc;
        
        m_config = config;

        return Ok();
    }

    /**
     * @brief Start DMA transfer
     *
     * @param src_addr Source address
     * @param dst_addr Destination address
     * @param size Transfer size in units
     * @return Result<void, ErrorCode>     *
     * @note Size is in units (bytes, halfwords, or words depending on width)
     */
    Result<void, ErrorCode> transfer(const volatile void* src_addr, volatile void* dst_addr, size_t size) {
        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (src_addr == nullptr || dst_addr == nullptr || size == 0) {
            return Err(ErrorCode::InvalidParameter);
        }

        // 
        auto* xdmac = get_xdmac();
        auto* channel = get_channel();
        
        // Set source and destination addresses
        channel->CSA = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src_addr));
        channel->CDA = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dst_addr));
        
        // Set transfer size (microblock control)
        channel->CUBC = size & 0xFFFFFF;  // 24-bit transfer count
        
        // Enable channel
        xdmac->GE = (1u << CHANNEL_NUM);
        
        // For memory-to-memory, trigger software request
        if (m_config.peripheral == DmaPeripheralId::MEM) {
            xdmac->GSWR = (1u << CHANNEL_NUM);
        }

        return Ok();
    }

    /**
     * @brief Check if transfer is complete
     *
     * @return bool Check if transfer is complete     */
    bool isComplete() const {
        // 
        auto* xdmac = get_xdmac();
        // Check if channel is still enabled
        return (xdmac->GS & (1u << CHANNEL_NUM)) == 0;

        return (xdmac->GS & (1u << CHANNEL_NUM)) == 0;
    }

    /**
     * @brief Wait for transfer to complete
     *
     * @param timeout_ms Timeout in milliseconds
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> waitComplete(uint32_t timeout_ms = 1000) {
        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // 
        uint32_t count = 0;
        while (!isComplete() && count < timeout_ms * 1000) {
            ++count;
        }
        
        if (!isComplete()) {
            return Err(ErrorCode::Timeout);
        }

        return Ok();
    }

    /**
     * @brief Enable channel interrupts
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> enableInterrupts() {
        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Enable block interrupt for this channel
        auto* xdmac = get_xdmac();
        xdmac->GIE = (1u << CHANNEL_NUM);

        return Ok();
    }

    /**
     * @brief Check if DMA channel is open
     *
     * @return bool Check if DMA channel is open     */
    bool isOpen() const {
        return m_opened;
    }


private:
    bool m_opened = false;  ///< Tracks if channel is initialized
    DmaConfig m_config = {};  ///< Current configuration
};

// ==============================================================================
// Predefined DMA Channel Instances
// ==============================================================================

using DmaChannel0 = DmaChannel<0>;  ///< DMA Channel 0 - General purpose
using DmaChannel1 = DmaChannel<1>;  ///< DMA Channel 1 - General purpose
using DmaChannel2 = DmaChannel<2>;  ///< DMA Channel 2 - General purpose
using DmaChannel3 = DmaChannel<3>;  ///< DMA Channel 3 - General purpose
using DmaChannel4 = DmaChannel<4>;  ///< DMA Channel 4 - General purpose
using DmaChannel5 = DmaChannel<5>;  ///< DMA Channel 5 - General purpose
using DmaChannel6 = DmaChannel<6>;  ///< DMA Channel 6 - General purpose
using DmaChannel7 = DmaChannel<7>;  ///< DMA Channel 7 - General purpose

} // namespace alloy::hal::same70
