// One codebase, many products — with the strategies as REAL code. The family
// (products/family.toml) names two control strategies; this file provides both
// as small mock control loops satisfying ONE concept (app::control_loop), and
// the generated <alloy/product.hpp> picks one per product as a type alias.
// `alloy build --product fan_eco` and `--product pump_pro` compile DIFFERENT
// control code from the same src/ — zero #ifdefs anywhere in this file.
#include <alloy/board.hpp>

#include <concepts>
#include <cstdint>

// ---- the contract every strategy must satisfy ------------------------------
namespace app {
template <class S>
concept control_loop = requires(S s, std::int32_t rpm_target) {
    { S::banner } -> std::convertible_to<const char*>;
    { s.step(rpm_target) } -> std::same_as<std::int32_t>;
};
}  // namespace app

// ---- the strategies the family names ---------------------------------------
// Declared BEFORE <alloy/product.hpp> so its `using control = ...;` alias can
// bind. Mock loops: real products would put their PWM/current math here.
namespace product_strategy {

// Open-loop V/f: output voltage is a fixed ramp of the speed target —
// stateless, the same command for the same target every cycle.
struct vf_scalar {
    static constexpr const char* banner = "control: v/f scalar\r\n";
    std::int32_t step(std::int32_t rpm_target) {
        return rpm_target * 23 / 10;  // mock 2.3 mV per rpm
    }
};

// Sensorless FOC: a flux-observer mock with STATE — the torque command chases
// the target first-order, so successive steps yield different values (the
// emulation leg asserts this difference; v/f cannot produce it).
struct sensorless_foc {
    static constexpr const char* banner = "control: sensorless foc\r\n";
    std::int32_t iq_ma = 0;
    std::int32_t step(std::int32_t rpm_target) {
        iq_ma += (rpm_target - iq_ma) / 4;
        return iq_ma;
    }
};

}  // namespace product_strategy

static_assert(app::control_loop<product_strategy::vf_scalar>);
static_assert(app::control_loop<product_strategy::sensorless_foc>);

#include <alloy/product.hpp>

// The product's CHOICE must satisfy the app's contract. A strategy type that
// exists but lacks the concept fails HERE, naming the concept — see
// scripts/check_compile_errors.py for the pinned diagnostic.
static_assert(app::control_loop<product::control>,
              "the product's control strategy must satisfy app::control_loop");

// Rule-derived facts hold at compile time: the family derives 16 kHz PWM for
// a PMSM, and every product can rely on it without restating it.
static_assert(product::params::motor != product::motor::pmsm ||
              product::params::pwm.hz() == 16000u);

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

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy product_demo ready\r\n");
    uart.write("product: ");
    uart.write(product::name);
    uart.write(" (family: ");
    uart.write(product::family);
    uart.write(")\r\n");
    uart.write(product::control::banner);

    uart.write("pwm hz: ");
    print_u32(uart, product::params::pwm.hz());
    uart.write("\r\nrated power w: ");
    print_u32(uart, static_cast<std::uint32_t>(product::params::rated_power_w));
    uart.write("\r\n");

    if constexpr (product::params::motor == product::motor::pmsm) {
        uart.write("motor: pmsm\r\n");
    } else {
        uart.write("motor: induction\r\n");
    }

    // Run the chosen strategy: the printed numbers come from the strategy's
    // OWN arithmetic, so the two products' UART output can only agree if the
    // wrong code was linked. v/f: 4140 4140 4140 — foc: 450 787 1040.
    product::control ctrl{};
    for (int n = 0; n < 3; ++n) {
        uart.write("step: ");
        print_u32(uart, static_cast<std::uint32_t>(ctrl.step(1800)));
        uart.write("\r\n");
    }

    if constexpr (product::caps::has_night_mode) {
        uart.write("night mode: fitted\r\n");
    } else {
        uart.write("night mode: none\r\n");
    }

    std::uint32_t last_toggle = alloy::uptime_ms();
    while (true) {
        if (alloy::uptime_ms() - last_toggle >= 500u) {
            board::status_led().toggle();
            last_toggle = alloy::uptime_ms();
        }
    }
}
