// One codebase, MANY PRODUCTS — the board pattern applied to the product
// dimension. products/family.toml declares the space, products/<name>.toml is
// one product (one file, one clean git diff), and the generated
// <alloy/product.hpp> turns the choice into constexpr params, caps and
// strategy type aliases. Switching products is `alloy build --product <name>`
// — the same src/, zero #ifdefs.
#include <alloy/board.hpp>

#include <cstdint>

// The app provides the strategy types the family names. They are declared
// BEFORE <alloy/product.hpp> so its `using control = ::product_strategy::…;`
// alias can bind; a product choosing an option with no type here fails the
// compile at that alias line.
namespace product_strategy {
struct six_step {
    static constexpr const char* banner = "control: six-step\r\n";
};
struct foc {
    static constexpr const char* banner = "control: foc\r\n";
};
}  // namespace product_strategy

#include <alloy/product.hpp>
#include <alloy/product_nvm.hpp>

namespace {
template <class Uart>
void print_u32(const Uart& uart, std::uint32_t v) {
    char buf[10];
    int i = 10;
    do {
        buf[--i] = static_cast<char>('0' + v % 10u);
        v /= 10u;
    } while (v != 0u);
    while (i < 10) {
        uart.write(static_cast<std::uint8_t>(buf[i++]));
    }
}
}  // namespace

// Rule-derived facts hold at compile time: the family derives a slower PWM
// for the large compressor, and the products can rely on it.
static_assert(product::params::pwm.hz() > 0u);
static_assert(product::params::compressor != product::compressor::large ||
              product::params::pwm.hz() <= 16000u);

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("product: ");
    uart.write(product::name);
    uart.write(" (family: ");
    uart.write(product::family);
    uart.write(")\r\n");

    uart.write(product::control::banner);

    uart.write("pwm hz: ");
    print_u32(uart, product::params::pwm.hz());
    uart.write("\r\nmax phase current ma: ");
    print_u32(uart, static_cast<std::uint32_t>(product::params::max_phase_current_ma));
    uart.write("\r\n");

    if constexpr (product::caps::has_display) {
        uart.write("display: present\r\n");
    } else {
        uart.write("display: none\r\n");
    }

    // An nvm field is configured at RUNTIME: the product ships a DEFAULT
    // (product_nvm.hpp), the live value sits in flash behind board::nvm on
    // boards that have the role — and the same bytes compile where it is
    // absent.
    std::uint32_t baud = product::nvm::modbus_baud_default;
    if constexpr (board::caps::nvm) {
        baud = board::nvm.get(product::nvm::k_modbus_baud,
                              product::nvm::modbus_baud_default);
    }
    uart.write("modbus baud: ");
    print_u32(uart, baud);
    uart.write("\r\n");

    std::uint32_t last_toggle = alloy::uptime_ms();
    while (true) {
        if (alloy::uptime_ms() - last_toggle >= 500u) {
            board::status_led().toggle();
            last_toggle = alloy::uptime_ms();
        }
    }
}
