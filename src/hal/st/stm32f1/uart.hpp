#ifndef ALLOY_HAL_STM32F1_UART_HPP
#define ALLOY_HAL_STM32F1_UART_HPP

#include "../../interface/uart.hpp"
#include "core/error.hpp"
#include "core/types.hpp"
#include <cstdint>

// Include generated peripheral definitions
#ifndef ALLOY_GENERATED_NAMESPACE
#error "ALLOY_GENERATED_NAMESPACE must be defined by CMake"
#endif
#include <peripherals.hpp>

namespace alloy::hal::stm32f1 {

// Import generated definitions into local namespace
using UsartRegs = ALLOY_GENERATED_NAMESPACE::usart::Registers;
using RccRegs = ALLOY_GENERATED_NAMESPACE::rcc::Registers;

namespace usart_addresses {
    using namespace ALLOY_GENERATED_NAMESPACE::usart;
}

namespace rcc {
    using namespace ALLOY_GENERATED_NAMESPACE::rcc;
}

// Import bit definitions
namespace sr_bits {
    using namespace ALLOY_GENERATED_NAMESPACE::usart::sr_bits;
}

namespace cr1_bits {
    using namespace ALLOY_GENERATED_NAMESPACE::usart::cr1_bits;
}

namespace cr2_bits {
    using namespace ALLOY_GENERATED_NAMESPACE::usart::cr2_bits;
    // CR2 STOP bit values for convenience
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
