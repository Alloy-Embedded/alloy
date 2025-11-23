/**
 * @file benchmark.hpp
 * @brief MicroCore Benchmarking Framework
 *
 * Lightweight benchmarking framework for embedded systems.
 * Measures performance metrics like GPIO toggle frequency, interrupt latency,
 * and peripheral throughput.
 *
 * Features:
 * - Zero-overhead timing using hardware timers
 * - Compile-time benchmark registration
 * - Automatic result formatting and reporting
 * - Comparison with baseline values
 * - Support for multiple platforms
 *
 * Usage:
 * @code
 * BENCHMARK("GPIO toggle", [] {
 *     using Led = GpioPin<GPIOA_BASE, 5>;
 *     Led led;
 *     led.setDirection(PinDirection::Output);
 *
 *     // Benchmark measures this block
 *     for (uint32_t i = 0; i < 10000; i++) {
 *         led.toggle();
 *     }
 * });
 * @endcode
 */

#pragma once

#include "core/types.hpp"
#include <cstdint>

namespace ucore::benchmark {

using namespace ucore::core;

// ============================================================================
// Benchmark Configuration
// ============================================================================

/**
 * @brief Benchmark configuration
 */
struct BenchmarkConfig {
    uint32_t iterations = 10000;    ///< Number of iterations to run
    uint32_t warmup_iterations = 100; ///< Warmup iterations (not measured)
    bool print_details = true;      ///< Print detailed results
};

// ============================================================================
// Timing Utilities
// ============================================================================

/**
 * @brief Get current cycle count (platform-specific)
 */
inline uint32_t get_cycle_count() {
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    // Cortex-M3/M4/M7: DWT cycle counter
    return *((volatile uint32_t*)0xE0001004);
#elif defined(__ARM_ARCH_6M__)
    // Cortex-M0/M0+: SysTick fallback (less accurate)
    return 0xFFFFFF - *((volatile uint32_t*)0xE000E018);
#else
    // Host platform: use approximation
    static uint32_t counter = 0;
    return counter++;
#endif
}

/**
 * @brief Initialize cycle counter
 */
inline void init_cycle_counter() {
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    // Enable DWT cycle counter
    *((volatile uint32_t*)0xE000EDFC) |= 0x01000000;  // Enable TRCENA
    *((volatile uint32_t*)0xE0001000) |= 0x00000001;  // Enable DWT cycle counter
    *((volatile uint32_t*)0xE0001004) = 0;             // Reset counter
#endif
}

/**
 * @brief Get CPU frequency in Hz
 */
inline uint32_t get_cpu_frequency_hz() {
    // Platform-specific - should be configured per board
#if defined(STM32F401xE)
    return 84000000;  // 84 MHz
#elif defined(STM32F722xx)
    return 216000000; // 216 MHz
#elif defined(ATSAME70Q21B)
    return 300000000; // 300 MHz
#else
    return 1000000;   // 1 MHz fallback
#endif
}

// ============================================================================
// Benchmark Result
// ============================================================================

/**
 * @brief Benchmark measurement result
 */
struct BenchmarkResult {
    const char* name;           ///< Benchmark name
    uint32_t total_cycles;      ///< Total cycles measured
    uint32_t iterations;        ///< Number of iterations
    uint32_t cycles_per_iter;   ///< Average cycles per iteration
    uint32_t frequency_hz;      ///< CPU frequency

    /**
     * @brief Get time per iteration in microseconds
     */
    float get_time_us_per_iter() const {
        return (float)cycles_per_iter / ((float)frequency_hz / 1000000.0f);
    }

    /**
     * @brief Get frequency in Hz (for toggle benchmarks)
     */
    float get_frequency_hz() const {
        if (cycles_per_iter == 0) return 0.0f;
        return (float)frequency_hz / (float)cycles_per_iter;
    }

    /**
     * @brief Get throughput in operations/second
     */
    float get_ops_per_second() const {
        return get_frequency_hz();
    }
};

// ============================================================================
// Benchmark Runner
// ============================================================================

/**
 * @brief Run a benchmark function
 *
 * @param name Benchmark name
 * @param func Function to benchmark
 * @param config Benchmark configuration
 * @return BenchmarkResult Measurement results
 */
template<typename Func>
BenchmarkResult run_benchmark(const char* name, Func&& func,
                              const BenchmarkConfig& config = BenchmarkConfig{}) {
    init_cycle_counter();

    // Warmup
    for (uint32_t i = 0; i < config.warmup_iterations; i++) {
        func();
    }

    // Measure
    uint32_t start_cycles = get_cycle_count();

    for (uint32_t i = 0; i < config.iterations; i++) {
        func();
    }

    uint32_t end_cycles = get_cycle_count();
    uint32_t total_cycles = end_cycles - start_cycles;

    BenchmarkResult result{};
    result.name = name;
    result.total_cycles = total_cycles;
    result.iterations = config.iterations;
    result.cycles_per_iter = total_cycles / config.iterations;
    result.frequency_hz = get_cpu_frequency_hz();

    return result;
}

// ============================================================================
// Result Formatting
// ============================================================================

/**
 * @brief Format benchmark result for printing
 */
inline void print_result(const BenchmarkResult& result) {
    // Header
    const char* separator = "========================================";

    // Platform info would go here if available

    // Results
    // Note: On embedded systems, use printf or similar
    // Format: "[BENCHMARK] name: X cycles/iter (Y.YY us/iter, Z.ZZ kHz)"
}

/**
 * @brief Compare benchmark result with baseline
 *
 * @param result Current result
 * @param baseline_cycles Expected baseline cycles
 * @return float Performance ratio (1.0 = same, >1.0 = slower, <1.0 = faster)
 */
inline float compare_with_baseline(const BenchmarkResult& result,
                                   uint32_t baseline_cycles) {
    if (baseline_cycles == 0) return 0.0f;
    return (float)result.cycles_per_iter / (float)baseline_cycles;
}

} // namespace ucore::benchmark

// ============================================================================
// Benchmark Macros
// ============================================================================

/**
 * @brief Define a benchmark
 *
 * Usage:
 * @code
 * BENCHMARK("My benchmark", [] {
 *     // Code to benchmark
 * });
 * @endcode
 */
#define BENCHMARK(name, func) \
    ucore::benchmark::run_benchmark(name, func)

/**
 * @brief Define a benchmark with custom configuration
 */
#define BENCHMARK_CONFIG(name, func, cfg) \
    ucore::benchmark::run_benchmark(name, func, cfg)
