#include "uart.hpp"

namespace alloy::hal::stm32f1 {

template<UsartId ID>
UartDevice<ID>::UartDevice() {
    enable_usart_clock();

    // Default configuration: 115200 baud, 8N1
    UartConfig default_config{core::baud_rates::Baud115200};
    configure(default_config);
}

template<UsartId ID>
void UartDevice<ID>::enable_usart_clock() {
    auto* rcc_regs = rcc::RCC;
    const uint32_t enable_bit = get_usart_enable_bit(ID);

    if (is_usart_on_apb2(ID)) {
        // Enable USART1 clock (APB2ENR)
        rcc_regs->APB2ENR |= (1U << enable_bit);
    } else {
        // Enable USART2/3 clock (APB1ENR)
        rcc_regs->APB1ENR |= (1U << enable_bit);
    }
}

template<UsartId ID>
UsartRegs* UartDevice<ID>::get_usart_registers() {
    return reinterpret_cast<UsartRegs*>(get_usart_address(ID));
}

template<UsartId ID>
constexpr core::u32 UartDevice<ID>::get_peripheral_clock() {
    // USART1 is on APB2 (72 MHz max)
    // USART2/3 are on APB1 (36 MHz max)
    // For now, assume maximum clock frequencies
    if (is_usart_on_apb2(ID)) {
        return 72000000;  // 72 MHz
    } else {
        return 36000000;  // 36 MHz
    }
}

template<UsartId ID>
uint32_t UartDevice<ID>::calculate_brr(core::u32 baud_rate) {
    // BRR calculation for STM32F1:
    // BRR = (peripheral_clock / (16 * baud_rate))
    //
    // BRR register format:
    // Bits [15:4] = Mantissa
    // Bits [3:0]  = Fraction (multiplied by 16)
    //
    // Example: 72MHz / (16 * 115200) = 39.0625
    //   Mantissa = 39 = 0x27
    //   Fraction = 0.0625 * 16 = 1 = 0x1
    //   BRR = 0x271

    const core::u32 pclk = get_peripheral_clock();
    const core::u32 usartdiv = (pclk + (baud_rate / 2)) / baud_rate;  // Round to nearest

    return usartdiv;
}

template<UsartId ID>
core::Result<void> UartDevice<ID>::configure(const UartConfig& config) {
    auto* usart = get_usart_registers();

    // Disable USART during configuration
    usart->CR1 &= ~cr1_bits::UE;

    // Configure baud rate
    usart->BRR = calculate_brr(config.baud_rate.value());

    // Configure CR1
    uint32_t cr1 = 0;
    cr1 |= cr1_bits::TE;   // Enable transmitter
    cr1 |= cr1_bits::RE;   // Enable receiver

    // Configure word length
    if (config.data_bits == DataBits::Nine) {
        cr1 |= cr1_bits::M;  // 9-bit mode
    }
    // 8-bit mode is default (M=0)

    // Configure parity
    if (config.parity != Parity::None) {
        cr1 |= cr1_bits::PCE;  // Enable parity control
        if (config.parity == Parity::Odd) {
            cr1 |= cr1_bits::PS;  // Odd parity
        }
        // Even parity is default (PS=0)
    }

    usart->CR1 = cr1;

    // Configure CR2 (stop bits)
    uint32_t cr2 = 0;
    if (config.stop_bits == StopBits::Two) {
        cr2 |= cr2_bits::STOP_2BIT;
    }
    // 1 stop bit is default

    usart->CR2 = cr2;

    // CR3 can be left at default (no hardware flow control)
    usart->CR3 = 0;

    // Enable USART
    usart->CR1 |= cr1_bits::UE;

    return core::Result<void>::ok();
}

template<UsartId ID>
core::Result<core::u8> UartDevice<ID>::read_byte() {
    auto* usart = get_usart_registers();

    // Check if data is available (RXNE flag)
    if ((usart->SR & sr_bits::RXNE) == 0) {
        return core::Result<core::u8>::error(core::ErrorCode::Timeout);
    }

    // Read data from DR register
    // Reading DR automatically clears RXNE flag
    core::u8 data = static_cast<core::u8>(usart->DR & 0xFF);

    return core::Result<core::u8>::ok(data);
}

template<UsartId ID>
core::Result<void> UartDevice<ID>::write_byte(core::u8 byte) {
    auto* usart = get_usart_registers();

    // Check if transmit buffer is empty (TXE flag)
    if ((usart->SR & sr_bits::TXE) == 0) {
        return core::Result<void>::error(core::ErrorCode::Busy);
    }

    // Write data to DR register
    // Writing DR automatically clears TXE flag
    usart->DR = byte;

    return core::Result<void>::ok();
}

template<UsartId ID>
core::usize UartDevice<ID>::available() const {
    auto* usart = get_usart_registers();

    // Check RXNE flag
    return (usart->SR & sr_bits::RXNE) != 0 ? 1 : 0;
}

// Explicit template instantiations for all USART peripherals
template class UartDevice<UsartId::USART1>;
template class UartDevice<UsartId::USART2>;
template class UartDevice<UsartId::USART3>;

} // namespace alloy::hal::stm32f1
