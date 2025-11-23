/**
 * @file gpio_toggle_bench.cpp
 * @brief GPIO Toggle Frequency Benchmark
 *
 * Measures the maximum GPIO toggle frequency achievable with MicroCore.
 * This is a key performance metric for embedded HAL libraries.
 *
 * Comparison targets:
 * - Arduino:     ~100 kHz (digitalWrite)
 * - mbed:        ~500 kHz (DigitalOut)
 * - STM32 HAL:   ~1 MHz (HAL_GPIO_TogglePin)
 * - Direct reg:  ~10+ MHz (register access)
 * - MicroCore:   Target >5 MHz (zero-overhead abstraction)
 *
 * Build:
 *   ./ucore build nucleo_f401re gpio_toggle_bench
 *   ./ucore flash nucleo_f401re gpio_toggle_bench
 */

#include "benchmark.hpp"

#ifdef PLATFORM_STM32F4
#include "hal/platform/stm32f4/gpio.hpp"
using namespace ucore::hal::stm32f4;
#elif defined(PLATFORM_STM32F7)
#include "hal/platform/stm32f7/gpio.hpp"
using namespace ucore::hal::stm32f7;
#elif defined(PLATFORM_HOST)
#include "hal/platform/host/gpio.hpp"
using namespace ucore::hal::host;
#else
#error "Unsupported platform for GPIO benchmark"
#endif

using namespace ucore::benchmark;

// ============================================================================
// GPIO Toggle Benchmarks
// ============================================================================

/**
 * @brief Benchmark: GPIO toggle using HAL API
 *
 * Measures toggle() method performance (most convenient API)
 */
void bench_gpio_toggle_hal() {
    constexpr uint32_t GPIOA_BASE_ADDR = 0x40020000;
    using Led = GpioPin<GPIOA_BASE_ADDR, 5>;

    Led led;
    led.setDirection(PinDirection::Output);

    BenchmarkConfig config{};
    config.iterations = 100000;
    config.warmup_iterations = 1000;

    auto result = run_benchmark("GPIO toggle (HAL)", [&led] {
        led.toggle();
    }, config);

    // Expected: 10-20 cycles on Cortex-M4 @ 84 MHz
    // Toggle frequency: ~4-8 MHz
}

/**
 * @brief Benchmark: GPIO set/clear using HAL API
 *
 * Measures set() + clear() performance
 */
void bench_gpio_set_clear_hal() {
    constexpr uint32_t GPIOA_BASE_ADDR = 0x40020000;
    using Led = GpioPin<GPIOA_BASE_ADDR, 5>;

    Led led;
    led.setDirection(PinDirection::Output);

    BenchmarkConfig config{};
    config.iterations = 100000;

    auto result = run_benchmark("GPIO set+clear (HAL)", [&led] {
        led.set();
        led.clear();
    }, config);

    // Expected: 8-16 cycles total (4-8 cycles per operation)
    // Using BSRR atomic register
}

/**
 * @brief Benchmark: GPIO write using HAL API
 *
 * Measures write(bool) performance
 */
void bench_gpio_write_hal() {
    constexpr uint32_t GPIOA_BASE_ADDR = 0x40020000;
    using Led = GpioPin<GPIOA_BASE_ADDR, 5>;

    Led led;
    led.setDirection(PinDirection::Output);

    BenchmarkConfig config{};
    config.iterations = 100000;

    bool state = false;
    auto result = run_benchmark("GPIO write (HAL)", [&led, &state] {
        led.write(state);
        state = !state;
    }, config);

    // Expected: 10-15 cycles (includes branch for set/clear decision)
}

/**
 * @brief Benchmark: Multiple GPIO toggles (parallel)
 *
 * Measures performance of controlling multiple pins
 */
void bench_multiple_gpio_toggle() {
    constexpr uint32_t GPIOA_BASE_ADDR = 0x40020000;
    using Pin0 = GpioPin<GPIOA_BASE_ADDR, 0>;
    using Pin1 = GpioPin<GPIOA_BASE_ADDR, 1>;
    using Pin2 = GpioPin<GPIOA_BASE_ADDR, 2>;
    using Pin3 = GpioPin<GPIOA_BASE_ADDR, 3>;

    Pin0 pin0; pin0.setDirection(PinDirection::Output);
    Pin1 pin1; pin1.setDirection(PinDirection::Output);
    Pin2 pin2; pin2.setDirection(PinDirection::Output);
    Pin3 pin3; pin3.setDirection(PinDirection::Output);

    BenchmarkConfig config{};
    config.iterations = 50000;

    auto result = run_benchmark("4x GPIO toggle (parallel)", [&] {
        pin0.toggle();
        pin1.toggle();
        pin2.toggle();
        pin3.toggle();
    }, config);

    // Expected: ~40-80 cycles for 4 toggles
    // Should scale linearly with number of pins
}

// ============================================================================
// Comparison Benchmarks
// ============================================================================

/**
 * @brief Baseline: Direct register access
 *
 * Fastest possible GPIO toggle (no abstraction)
 * Used as baseline for comparison
 */
void bench_gpio_direct_register() {
#ifndef PLATFORM_HOST
    // Direct STM32 BSRR access
    volatile uint32_t* GPIOA_BSRR = (volatile uint32_t*)0x40020018;
    constexpr uint32_t PIN5_SET = (1 << 5);
    constexpr uint32_t PIN5_RESET = (1 << (5 + 16));

    BenchmarkConfig config{};
    config.iterations = 100000;

    bool state = false;
    auto result = run_benchmark("GPIO toggle (direct reg)", [&] {
        if (state) {
            *GPIOA_BSRR = PIN5_SET;
        } else {
            *GPIOA_BSRR = PIN5_RESET;
        }
        state = !state;
    }, config);

    // Expected: 6-10 cycles (theoretical minimum)
    // Includes branch overhead
#endif
}

// ============================================================================
// Main Benchmark Runner
// ============================================================================

int main() {
    // Initialize benchmark system
    init_cycle_counter();

    // Print header
    // (Would use UART or printf on real hardware)

    // Run benchmarks
    bench_gpio_toggle_hal();
    bench_gpio_set_clear_hal();
    bench_gpio_write_hal();
    bench_multiple_gpio_toggle();
    bench_gpio_direct_register();

    // Print summary
    // Compare with baseline values
    // Calculate overhead percentage

    while (1) {
        // Keep running for analysis
    }

    return 0;
}
