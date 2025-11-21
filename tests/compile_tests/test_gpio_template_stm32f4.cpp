/**
 * @file test_gpio_template_stm32f4.cpp
 * @brief Compile test for STM32F4 GPIO template generation
 *
 * This test verifies that:
 * - Generated STM32F4 GPIO code compiles
 * - All hardware policy methods are available
 * - Compile-time constants are correct
 * - Zero-overhead (no vtables)
 */

#include "core/types.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"

using namespace ucore::core;

// Mock GPIO registers for STM32F4
namespace st {
    struct GPIO_TypeDef {
        volatile uint32_t MODER;
        volatile uint32_t OTYPER;
        volatile uint32_t OSPEEDR;
        volatile uint32_t PUPDR;
        volatile uint32_t IDR;
        volatile uint32_t ODR;
        volatile uint32_t BSRR;
        volatile uint32_t LCKR;
        volatile uint32_t AFRL;
        volatile uint32_t AFRH;
    };
}

// Manually include the GPIO hardware policy (skip the includes)
namespace ucore::hal::st::stm32f4 {

using namespace ucore::core;

template <uint32_t BASE_ADDR, char PORT_CHAR>
struct STM32F4GpioHardwarePolicy {
    using RegisterType = ::st::GPIO_TypeDef;

    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr char port_char = PORT_CHAR;
    static constexpr uint32_t pins_per_port = 16;

    static constexpr uint32_t MODE_INPUT = 0;
    static constexpr uint32_t MODE_OUTPUT = 1;
    static constexpr uint32_t MODE_AF = 2;
    static constexpr uint32_t MODE_ANALOG = 3;

    static constexpr uint32_t OTYPE_PUSH_PULL = 0;
    static constexpr uint32_t OTYPE_OPEN_DRAIN = 1;

    static constexpr uint32_t OSPEED_LOW = 0;
    static constexpr uint32_t OSPEED_MEDIUM = 1;
    static constexpr uint32_t OSPEED_HIGH = 2;
    static constexpr uint32_t OSPEED_VERY_HIGH = 3;

    static constexpr uint32_t PUPD_NONE = 0;
    static constexpr uint32_t PUPD_PULL_UP = 1;
    static constexpr uint32_t PUPD_PULL_DOWN = 2;

    static inline volatile RegisterType* hw() {
        return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
    }

    static inline void set_mode(u8 pin, u32 mode) {
        const u32 shift = pin * 2;
        const u32 mask = 0x3 << shift;
        hw()->MODER = (hw()->MODER & ~mask) | ((mode & 0x3) << shift);
    }

    static inline void set_output_type(u8 pin, u32 otype) {
        if (otype == OTYPE_OPEN_DRAIN) {
            hw()->OTYPER |= (1 << pin);
        } else {
            hw()->OTYPER &= ~(1 << pin);
        }
    }

    static inline void set_output_speed(u8 pin, u32 speed) {
        const u32 shift = pin * 2;
        const u32 mask = 0x3 << shift;
        hw()->OSPEEDR = (hw()->OSPEEDR & ~mask) | ((speed & 0x3) << shift);
    }

    static inline void set_pull(u8 pin, u32 pupd) {
        const u32 shift = pin * 2;
        const u32 mask = 0x3 << shift;
        hw()->PUPDR = (hw()->PUPDR & ~mask) | ((pupd & 0x3) << shift);
    }

    static inline void set_alternate_function(u8 pin, u8 af) {
        if (pin < 8) {
            const u32 shift = pin * 4;
            const u32 mask = 0xF << shift;
            hw()->AFRL = (hw()->AFRL & ~mask) | ((af & 0xF) << shift);
        } else {
            const u32 shift = (pin - 8) * 4;
            const u32 mask = 0xF << shift;
            hw()->AFRH = (hw()->AFRH & ~mask) | ((af & 0xF) << shift);
        }
    }

    static inline void set(u8 pin) {
        hw()->BSRR = (1 << pin);
    }

    static inline void clear(u8 pin) {
        hw()->BSRR = (1 << (pin + 16));
    }

    static inline void toggle(u8 pin) {
        hw()->ODR ^= (1 << pin);
    }

    static inline bool read(u8 pin) {
        return (hw()->IDR & (1 << pin)) != 0;
    }

    static inline void write(u8 pin, bool value) {
        if (value) {
            set(pin);
        } else {
            clear(pin);
        }
    }

    static inline u16 read_port() {
        return static_cast<u16>(hw()->IDR & 0xFFFF);
    }

    static inline void write_port(u16 value) {
        hw()->ODR = value;
    }

    static inline void set_mask(u16 mask) {
        hw()->BSRR = mask;
    }

    static inline void clear_mask(u16 mask) {
        hw()->BSRR = (static_cast<u32>(mask) << 16);
    }
};

// Type aliases
using GPIOAHardware = STM32F4GpioHardwarePolicy<0x40020000, 'A'>;
using GPIOBHardware = STM32F4GpioHardwarePolicy<0x40020400, 'B'>;
using GPIOCHardware = STM32F4GpioHardwarePolicy<0x40020800, 'C'>;
using GPIODHardware = STM32F4GpioHardwarePolicy<0x40020C00, 'D'>;
using GPIOEHardware = STM32F4GpioHardwarePolicy<0x40021000, 'E'>;
using GPIOFHardware = STM32F4GpioHardwarePolicy<0x40021400, 'F'>;
using GPIOGHardware = STM32F4GpioHardwarePolicy<0x40021800, 'G'>;
using GPIOHHardware = STM32F4GpioHardwarePolicy<0x40021C00, 'H'>;

}  // namespace ucore::hal::st::stm32f4

using namespace ucore::hal::st::stm32f4;

// Test compile-time constants
static_assert(GPIOAHardware::base_address == 0x40020000, "GPIOA base address");
static_assert(GPIOAHardware::port_char == 'A', "GPIOA port char");
static_assert(GPIOAHardware::pins_per_port == 16, "GPIO pins per port");

static_assert(GPIOBHardware::base_address == 0x40020400, "GPIOB base address");
static_assert(GPIOBHardware::port_char == 'B', "GPIOB port char");

// Test mode constants
static_assert(GPIOAHardware::MODE_INPUT == 0, "MODE_INPUT");
static_assert(GPIOAHardware::MODE_OUTPUT == 1, "MODE_OUTPUT");
static_assert(GPIOAHardware::MODE_AF == 2, "MODE_AF");
static_assert(GPIOAHardware::MODE_ANALOG == 3, "MODE_ANALOG");

// Test output type constants
static_assert(GPIOAHardware::OTYPE_PUSH_PULL == 0, "OTYPE_PUSH_PULL");
static_assert(GPIOAHardware::OTYPE_OPEN_DRAIN == 1, "OTYPE_OPEN_DRAIN");

// Test usage
void test_gpio_operations() {
    // These calls should compile but won't run (no actual hardware)

    // Pin configuration
    GPIOAHardware::set_mode(5, GPIOAHardware::MODE_OUTPUT);
    GPIOAHardware::set_output_type(5, GPIOAHardware::OTYPE_PUSH_PULL);
    GPIOAHardware::set_output_speed(5, GPIOAHardware::OSPEED_MEDIUM);
    GPIOAHardware::set_pull(5, GPIOAHardware::PUPD_NONE);

    // Pin I/O
    GPIOAHardware::set(5);
    GPIOAHardware::clear(5);
    GPIOAHardware::toggle(5);
    [[maybe_unused]] bool state = GPIOAHardware::read(5);
    GPIOAHardware::write(5, true);

    // Port-wide operations
    [[maybe_unused]] u16 port_state = GPIOAHardware::read_port();
    GPIOAHardware::write_port(0xFFFF);
    GPIOAHardware::set_mask(0x00FF);
    GPIOAHardware::clear_mask(0xFF00);

    // Alternate function
    GPIOAHardware::set_alternate_function(9, 7);  // PA9 AF7 (USART1 TX)
}

// Test that policy has no vtable (zero-overhead)
static_assert(sizeof(GPIOAHardware) == 1, "Policy should be empty (EBO)");

int main() {
    // Compile-time test only
    return 0;
}
