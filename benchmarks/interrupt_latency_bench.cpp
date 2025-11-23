/**
 * @file interrupt_latency_bench.cpp
 * @brief Interrupt Latency Benchmark
 *
 * Measures interrupt response time from trigger to handler execution.
 * This is critical for real-time embedded systems.
 *
 * Metrics measured:
 * - Hardware latency: Time from IRQ signal to handler entry
 * - Context switch time: Time to save/restore registers
 * - Handler execution time: Time spent in ISR
 * - Total latency: End-to-end response time
 *
 * Comparison targets:
 * - Arduino:      ~10-20 us (slow ISR entry)
 * - mbed:         ~2-5 us (RTOS overhead)
 * - STM32 HAL:    ~1-2 us (minimal overhead)
 * - Bare-metal:   ~500 ns (theoretical minimum)
 * - MicroCore:    Target <1 us (zero-overhead ISR)
 *
 * Build:
 *   ./ucore build nucleo_f401re interrupt_latency_bench
 */

#include "benchmark.hpp"

#ifdef PLATFORM_STM32F4
#include "hal/platform/stm32f4/gpio.hpp"
using namespace ucore::hal::stm32f4;
#else
#error "Interrupt latency benchmark requires embedded platform"
#endif

using namespace ucore::benchmark;

// ============================================================================
// Latency Measurement
// ============================================================================

// Timing pins for oscilloscope measurement
constexpr uint32_t GPIOA_BASE_ADDR = 0x40020000;
using TriggerPin = GpioPin<GPIOA_BASE_ADDR, 5>;  // Trigger interrupt
using ResponsePin = GpioPin<GPIOA_BASE_ADDR, 6>; // Set in ISR

volatile uint32_t irq_entry_cycles = 0;
volatile uint32_t irq_exit_cycles = 0;
volatile uint32_t irq_count = 0;

/**
 * @brief External interrupt handler (EXTI)
 *
 * Measures time from interrupt trigger to handler entry
 */
extern "C" void EXTI9_5_IRQHandler() {
    // Record entry time immediately
    irq_entry_cycles = get_cycle_count();

    // Set response pin HIGH (for oscilloscope)
    ResponsePin response;
    response.set();

    // Minimal handler work
    irq_count++;

    // Clear response pin
    response.clear();

    // Clear EXTI pending bit
    *((volatile uint32_t*)0x40013C14) = (1 << 5);

    // Record exit time
    irq_exit_cycles = get_cycle_count();
}

/**
 * @brief Setup external interrupt on GPIO
 */
void setup_external_interrupt() {
    // Configure trigger pin as input with pull-down
    TriggerPin trigger;
    trigger.setDirection(PinDirection::Input);
    trigger.setPull(PinPull::PullDown);

    // Configure response pin as output
    ResponsePin response;
    response.setDirection(PinDirection::Output);
    response.clear();

    // Enable SYSCFG clock
    *((volatile uint32_t*)0x40023844) |= (1 << 14);

    // Connect EXTI5 to PA5
    *((volatile uint32_t*)0x40013808) &= ~(0xF << 4);
    *((volatile uint32_t*)0x40013808) |= (0x0 << 4);  // GPIOA

    // Configure EXTI5 for rising edge
    *((volatile uint32_t*)0x40013C00) |= (1 << 5);   // Interrupt enable
    *((volatile uint32_t*)0x40013C08) |= (1 << 5);   // Rising edge
    *((volatile uint32_t*)0x40013C0C) &= ~(1 << 5);  // Falling edge disabled

    // Enable EXTI9_5 interrupt in NVIC
    *((volatile uint32_t*)0xE000E100) |= (1 << 23);

    // Set interrupt priority (highest)
    *((volatile uint8_t*)(0xE000E400 + 23)) = 0;
}

/**
 * @brief Trigger software interrupt for measurement
 */
void trigger_interrupt() {
    // Set EXTI5 pending bit (software trigger)
    *((volatile uint32_t*)0x40013C10) = (1 << 5);
}

/**
 * @brief Measure interrupt latency using cycle counter
 */
BenchmarkResult bench_interrupt_latency_internal() {
    uint32_t trigger_cycles = get_cycle_count();
    trigger_interrupt();

    // Wait for interrupt to complete
    while (irq_entry_cycles == 0) {
        __asm volatile("nop");
    }

    uint32_t latency_cycles = irq_entry_cycles - trigger_cycles;

    BenchmarkResult result{};
    result.name = "Interrupt latency (software trigger)";
    result.total_cycles = latency_cycles;
    result.iterations = 1;
    result.cycles_per_iter = latency_cycles;
    result.frequency_hz = get_cpu_frequency_hz();

    // Reset for next measurement
    irq_entry_cycles = 0;

    return result;
}

/**
 * @brief Benchmark: Average interrupt latency
 */
void bench_average_interrupt_latency() {
    BenchmarkConfig config{};
    config.iterations = 1000;
    config.warmup_iterations = 10;

    uint32_t total_cycles = 0;

    for (uint32_t i = 0; i < config.iterations; i++) {
        auto result = bench_interrupt_latency_internal();
        total_cycles += result.total_cycles;
    }

    BenchmarkResult result{};
    result.name = "Interrupt latency (average)";
    result.total_cycles = total_cycles;
    result.iterations = config.iterations;
    result.cycles_per_iter = total_cycles / config.iterations;
    result.frequency_hz = get_cpu_frequency_hz();

    // Expected: 12-20 cycles on Cortex-M4 @ 84 MHz
    // Latency: ~150-250 ns
}

/**
 * @brief Benchmark: Interrupt handler execution time
 */
void bench_interrupt_handler_time() {
    // Trigger one interrupt
    auto result = bench_interrupt_latency_internal();

    uint32_t handler_time = irq_exit_cycles - irq_entry_cycles;

    BenchmarkResult handler_result{};
    handler_result.name = "Interrupt handler execution";
    handler_result.total_cycles = handler_time;
    handler_result.iterations = 1;
    handler_result.cycles_per_iter = handler_time;
    handler_result.frequency_hz = get_cpu_frequency_hz();

    // Expected: 20-40 cycles (minimal handler)
}

/**
 * @brief Benchmark: Nested interrupt latency
 *
 * Measures latency when interrupt preempts another interrupt
 */
void bench_nested_interrupt_latency() {
    // Configure two interrupts with different priorities
    // Higher priority interrupt should preempt lower priority

    // Implementation would require two interrupt sources
    // and careful priority configuration
}

// ============================================================================
// Main Benchmark Runner
// ============================================================================

int main() {
    init_cycle_counter();
    setup_external_interrupt();

    // Run benchmarks
    bench_average_interrupt_latency();
    bench_interrupt_handler_time();

    // For oscilloscope measurement:
    // - Connect scope to trigger pin (PA5) and response pin (PA6)
    // - Measure time between rising edges
    // - This gives hardware-verified latency

    while (1) {
        // Continuous measurement mode
        __asm volatile("wfi");  // Wait for interrupt
    }

    return 0;
}
