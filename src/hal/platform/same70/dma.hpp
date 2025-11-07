/**
 * @file dma.hpp
 * @brief Template-based XDMAC implementation for SAME70 (ARM Cortex-M7)
 *
 * This file implements the XDMAC (eXtensible DMA Controller) for SAME70
 * using templates with ZERO virtual functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Channel number resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - Generic: Works with any peripheral (ADC, UART, SPI, I2C, etc.)
 *
 * SAME70 XDMAC Features:
 * - 24 DMA channels
 * - Supports peripheral-to-memory, memory-to-peripheral, memory-to-memory
 * - Hardware handshaking with peripherals
 * - Linked list descriptor support
 * - Multiple transfer sizes (byte, halfword, word)
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// Include SAME70 register definitions
#include "hal/vendors/atmel/same70/registers/xdmac_registers.hpp"
#include "hal/vendors/atmel/same70/bitfields/xdmac_bitfields.hpp"
#include "hal/platform/same70/clock.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;
namespace xdmac = atmel::same70::xdmac;  // Alias for easier bitfield access

/**
 * @brief DMA Transfer Direction
 */
enum class DmaDirection : uint8_t {
    PeripheralToMemory = 0,  // Peripheral -> Memory (e.g., ADC -> RAM)
    MemoryToPeripheral = 1,  // Memory -> Peripheral (e.g., RAM -> UART)
    MemoryToMemory = 2,      // Memory -> Memory (memcpy)
};

/**
 * @brief DMA Transfer Width
 */
enum class DmaWidth : uint8_t {
    Byte = 0,      // 8-bit transfers
    HalfWord = 1,  // 16-bit transfers
    Word = 2,      // 32-bit transfers
};

/**
 * @brief DMA Peripheral IDs for hardware handshaking
 *
 * These IDs are used to connect DMA channels to specific peripherals.
 * The hardware handles the request/acknowledge automatically.
 */
enum class DmaPeripheralId : uint8_t {
    // TWI (I2C)
    TWIHS0_TX = 0,
    TWIHS0_RX = 1,
    TWIHS1_TX = 2,
    TWIHS1_RX = 3,
    TWIHS2_TX = 4,
    TWIHS2_RX = 5,

    // SPI
    SPI0_TX = 6,
    SPI0_RX = 7,
    SPI1_TX = 8,
    SPI1_RX = 9,

    // USART/UART
    USART0_TX = 10,
    USART0_RX = 11,
    USART1_TX = 12,
    USART1_RX = 13,
    USART2_TX = 14,
    USART2_RX = 15,

    // ADC (AFEC)
    AFEC0 = 16,
    AFEC1 = 17,

    // DAC
    DACC = 18,

    // SSC (Audio)
    SSC_TX = 19,
    SSC_RX = 20,

    // PWM
    PWM0 = 21,
    PWM1 = 22,

    // TC (Timer)
    TC0 = 23,
    TC1 = 24,
    TC2 = 25,
    TC3 = 26,

    // AES
    AES_TX = 27,
    AES_RX = 28,

    // QSPI
    QSPI_TX = 29,
    QSPI_RX = 30,

    // Memory-to-memory (no peripheral)
    MEM = 0xFF,
};

/**
 * @brief DMA Channel Configuration
 */
struct DmaConfig {
    DmaDirection direction = DmaDirection::PeripheralToMemory;
    DmaWidth src_width = DmaWidth::Word;
    DmaWidth dst_width = DmaWidth::Word;
    DmaPeripheralId peripheral = DmaPeripheralId::MEM;
    bool src_increment = false;      ///< Increment source address
    bool dst_increment = true;       ///< Increment destination address
    uint8_t burst_size = 1;          ///< Burst size (1, 4, 8, 16)
};

/**
 * @brief XDMAC Channel Registers (per-channel)
 *
 * Each of the 24 DMA channels has its own register block at offset 0x50 + (channel * 0x40)
 */
struct XdmacChannelRegisters {
    volatile uint32_t CIE;       ///< Channel Interrupt Enable
    volatile uint32_t CID;       ///< Channel Interrupt Disable
    volatile uint32_t CIM;       ///< Channel Interrupt Mask
    volatile uint32_t CIS;       ///< Channel Interrupt Status
    volatile uint32_t CSA;       ///< Channel Source Address
    volatile uint32_t CDA;       ///< Channel Destination Address
    volatile uint32_t CNDA;      ///< Channel Next Descriptor Address
    volatile uint32_t CNDC;      ///< Channel Next Descriptor Control
    volatile uint32_t CUBC;      ///< Channel Microblock Control
    volatile uint32_t CBC;       ///< Channel Block Control
    volatile uint32_t CC;        ///< Channel Configuration
    volatile uint32_t CDS_MSP;   ///< Channel Data Stride/Memory Set Pattern
    volatile uint32_t CSUS;      ///< Channel Source Microblock Stride
    volatile uint32_t CDUS;      ///< Channel Destination Microblock Stride
    uint32_t RESERVED[2];
};

/**
 * @brief Template-based DMA channel for SAME70
 *
 * Each DMA channel is a separate template instance, allowing compile-time
 * optimization and type safety.
 *
 * @tparam CHANNEL_NUM DMA channel number (0-23)
 */
template <uint8_t CHANNEL_NUM>
class DmaChannel {
    static_assert(CHANNEL_NUM < 24, "SAME70 XDMAC has 24 channels (0-23)");

public:
    static constexpr uint8_t channel = CHANNEL_NUM;
    static constexpr uint32_t base_address = 0x40078000;

    // Channel register offset: 0x50 + (channel * 0x40)
    static constexpr uint32_t channel_offset = 0x50 + (CHANNEL_NUM * 0x40);

    static inline volatile atmel::same70::xdmac::XDMAC_Registers* get_xdmac() {
#ifdef ALLOY_DMA_MOCK_HW
        return ALLOY_DMA_MOCK_HW();
#else
        return reinterpret_cast<volatile atmel::same70::xdmac::XDMAC_Registers*>(base_address);
#endif
    }

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
     */
    Result<void> open() {
        if (m_opened) {
            return Result<void>::error(ErrorCode::AlreadyInitialized);
        }

        auto* xdmac = get_xdmac();

        // Enable XDMAC clock (peripheral ID 22) using Clock class
        auto clock_result = Clock::enablePeripheralClock(22);
        if (!clock_result.is_ok()) {
            return Result<void>::error(clock_result.error());
        }

        // Disable channel initially
        xdmac->GD = (1u << CHANNEL_NUM);

        m_opened = true;
        return Result<void>::ok();
    }

    /**
     * @brief Close DMA channel
     */
    Result<void> close() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* xdmac = get_xdmac();

        // Disable channel
        xdmac->GD = (1u << CHANNEL_NUM);

        m_opened = false;
        return Result<void>::ok();
    }

    /**
     * @brief Configure DMA channel
     */
    Result<void> configure(const DmaConfig& config) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* channel = get_channel();

        // Build CC register value using type-safe bitfields
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

        // Data width (source and destination)
        // Note: Using DWIDTH for both source and destination width
        // SAME70 XDMAC uses DWIDTH for data width configuration
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
        return Result<void>::ok();
    }

    /**
     * @brief Start DMA transfer
     *
     * @param src_addr Source address (can be volatile for peripheral registers)
     * @param dst_addr Destination address (can be volatile for peripheral registers)
     * @param size Transfer size in units (bytes, halfwords, or words depending on width)
     */
    Result<void> transfer(const volatile void* src_addr, volatile void* dst_addr, size_t size) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        if (src_addr == nullptr || dst_addr == nullptr || size == 0) {
            return Result<void>::error(ErrorCode::InvalidParameter);
        }

        auto* xdmac = get_xdmac();
        auto* channel = get_channel();

        // Set source and destination addresses
        channel->CSA = reinterpret_cast<uint32_t>(src_addr);
        channel->CDA = reinterpret_cast<uint32_t>(dst_addr);

        // Set transfer size (microblock control)
        channel->CUBC = size & 0xFFFFFF;  // 24-bit transfer count

        // Enable channel
        xdmac->GE = (1u << CHANNEL_NUM);

        // For memory-to-memory, trigger software request
        if (m_config.peripheral == DmaPeripheralId::MEM) {
            xdmac->GSWR = (1u << CHANNEL_NUM);
        }

        return Result<void>::ok();
    }

    /**
     * @brief Check if transfer is complete
     */
    bool isComplete() const {
        auto* xdmac = get_xdmac();
        // Check if channel is still enabled
        return (xdmac->GS & (1u << CHANNEL_NUM)) == 0;
    }

    /**
     * @brief Wait for transfer to complete
     */
    Result<void> waitComplete(uint32_t timeout_ms = 1000) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        uint32_t count = 0;
        while (!isComplete() && count < timeout_ms * 1000) {
            ++count;
        }

        if (!isComplete()) {
            return Result<void>::error(ErrorCode::Timeout);
        }

        return Result<void>::ok();
    }

    /**
     * @brief Enable channel interrupts
     */
    Result<void> enableInterrupts() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* xdmac = get_xdmac();

        // Enable block interrupt for this channel
        xdmac->GIE = (1u << CHANNEL_NUM);

        return Result<void>::ok();
    }

    bool isOpen() const {
        return m_opened;
    }

private:
    bool m_opened = false;
    DmaConfig m_config;
};

// ============================================================================
// Predefined DMA channel instances for SAME70
// ============================================================================

// Common DMA channels for different use cases
using DmaChannel0 = DmaChannel<0>;   // General purpose
using DmaChannel1 = DmaChannel<1>;   // General purpose
using DmaChannel2 = DmaChannel<2>;   // General purpose
using DmaChannel3 = DmaChannel<3>;   // General purpose

// ADC channels
using DmaAdcChannel0 = DmaChannel<4>;
using DmaAdcChannel1 = DmaChannel<5>;

// UART channels
using DmaUartTx = DmaChannel<6>;
using DmaUartRx = DmaChannel<7>;

// SPI channels
using DmaSpiTx = DmaChannel<8>;
using DmaSpiRx = DmaChannel<9>;

// I2C channels
using DmaI2cTx = DmaChannel<10>;
using DmaI2cRx = DmaChannel<11>;

} // namespace alloy::hal::same70
