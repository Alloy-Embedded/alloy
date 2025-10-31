#ifndef ALLOY_HAL_STM32F1_UART_HPP
#define ALLOY_HAL_STM32F1_UART_HPP

#include "../interface/uart.hpp"
#include "core/error.hpp"
#include "core/types.hpp"
#include <cstdint>

namespace alloy::hal::stm32f1 {

// Include generated peripheral definitions
#ifdef ALLOY_USE_GENERATED_PERIPHERALS
#include "peripherals.hpp"
    // Use generated definitions
    using UsartRegs = alloy::generated::stm32f103c8::usart::Registers;
    namespace usart_addresses {
        using namespace alloy::generated::stm32f103c8::usart;
    }
#else
    // Fallback to hardcoded definitions

    /// STM32F1 USART peripheral base addresses
    namespace usart_addresses {
        constexpr uint32_t USART1_BASE = 0x40013800;
        constexpr uint32_t USART2_BASE = 0x40004400;
        constexpr uint32_t USART3_BASE = 0x40004800;

        constexpr uint32_t RCC_BASE = 0x40021000;
    }

    /// STM32F1 USART register structure
    struct UsartRegs {
        volatile uint32_t SR;    ///< Status register
        volatile uint32_t DR;    ///< Data register
        volatile uint32_t BRR;   ///< Baud rate register
        volatile uint32_t CR1;   ///< Control register 1
        volatile uint32_t CR2;   ///< Control register 2
        volatile uint32_t CR3;   ///< Control register 3
        volatile uint32_t GTPR;  ///< Guard time and prescaler register
    };
#endif

/// USART Status Register (SR) bits
namespace sr_bits {
    constexpr uint32_t TXE  = (1U << 7);  ///< Transmit data register empty
    constexpr uint32_t TC   = (1U << 6);  ///< Transmission complete
    constexpr uint32_t RXNE = (1U << 5);  ///< Read data register not empty
    constexpr uint32_t IDLE = (1U << 4);  ///< IDLE line detected
    constexpr uint32_t ORE  = (1U << 3);  ///< Overrun error
    constexpr uint32_t NE   = (1U << 2);  ///< Noise error
    constexpr uint32_t FE   = (1U << 1);  ///< Framing error
    constexpr uint32_t PE   = (1U << 0);  ///< Parity error
}

/// USART Control Register 1 (CR1) bits
namespace cr1_bits {
    constexpr uint32_t UE    = (1U << 13); ///< USART enable
    constexpr uint32_t M     = (1U << 12); ///< Word length (0=8bit, 1=9bit)
    constexpr uint32_t PCE   = (1U << 10); ///< Parity control enable
    constexpr uint32_t PS    = (1U << 9);  ///< Parity selection (0=even, 1=odd)
    constexpr uint32_t PEIE  = (1U << 8);  ///< PE interrupt enable
    constexpr uint32_t TXEIE = (1U << 7);  ///< TXE interrupt enable
    constexpr uint32_t TCIE  = (1U << 6);  ///< TC interrupt enable
    constexpr uint32_t RXNEIE= (1U << 5);  ///< RXNE interrupt enable
    constexpr uint32_t TE    = (1U << 3);  ///< Transmitter enable
    constexpr uint32_t RE    = (1U << 2);  ///< Receiver enable
}

/// USART Control Register 2 (CR2) bits
namespace cr2_bits {
    constexpr uint32_t STOP_1BIT = (0b00 << 12); ///< 1 stop bit
    constexpr uint32_t STOP_2BIT = (0b10 << 12); ///< 2 stop bits
}

/// USART peripheral identifiers
enum class UsartId : uint8_t {
    USART1 = 0,
    USART2 = 1,
    USART3 = 2
};

/// Get USART peripheral base address
constexpr inline uint32_t get_usart_address(UsartId id) {
    switch (id) {
        case UsartId::USART1: return usart_addresses::USART1_BASE;
        case UsartId::USART2: return usart_addresses::USART2_BASE;
        case UsartId::USART3: return usart_addresses::USART3_BASE;
    }
    return 0;
}

/// Get USART clock enable bit in RCC
/// USART1 is on APB2 (bit 14 in RCC_APB2ENR)
/// USART2/3 are on APB1 (bits 17/18 in RCC_APB1ENR)
constexpr inline bool is_usart_on_apb2(UsartId id) {
    return id == UsartId::USART1;
}

constexpr inline uint32_t get_usart_enable_bit(UsartId id) {
    switch (id) {
        case UsartId::USART1: return 14;  // APB2ENR bit 14
        case UsartId::USART2: return 17;  // APB1ENR bit 17
        case UsartId::USART3: return 18;  // APB1ENR bit 18
    }
    return 0;
}

/// STM32F1 UART device implementation
///
/// Implements UART operations for STM32F1 USART peripherals.
///
/// Default clock frequencies:
/// - APB2 (USART1): 72 MHz (max)
/// - APB1 (USART2/3): 36 MHz (max)
template<UsartId ID>
class UartDevice {
public:
    static constexpr UsartId usart_id = ID;

    /// Constructor - initializes USART peripheral
    UartDevice();

    /// Read a single byte from UART
    /// @return Byte value or ErrorCode::Timeout if no data available
    [[nodiscard]] core::Result<core::u8> read_byte();

    /// Write a single byte to UART
    /// @param byte Byte to transmit
    /// @return Success or ErrorCode::Busy if transmit buffer is full
    [[nodiscard]] core::Result<void> write_byte(core::u8 byte);

    /// Check how many bytes are available to read
    /// @return Number of bytes available (0 or 1 for simple implementation)
    [[nodiscard]] core::usize available() const;

    /// Configure the UART with new parameters
    /// @param config UART configuration
    /// @return Success or ErrorCode::InvalidParameter
    [[nodiscard]] core::Result<void> configure(const UartConfig& config);

private:
    /// Enable USART peripheral clock
    static void enable_usart_clock();

    /// Get pointer to USART registers
    static UsartRegs* get_usart_registers();

    /// Calculate BRR value from baud rate
    /// @param baud_rate Desired baud rate
    /// @return BRR register value
    static uint32_t calculate_brr(core::u32 baud_rate);

    /// Get peripheral clock frequency (APB1 or APB2)
    /// @return Clock frequency in Hz
    static constexpr core::u32 get_peripheral_clock();
};

// Static assertion to verify concept compliance
static_assert(alloy::hal::UartDevice<UartDevice<UsartId::USART1>>,
              "stm32f1::UartDevice must satisfy UartDevice concept");

} // namespace alloy::hal::stm32f1

#endif // ALLOY_HAL_STM32F1_UART_HPP
