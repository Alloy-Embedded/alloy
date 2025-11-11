/**
 * @file test_timer_apis.cpp
 * @brief Unit tests for Timer multi-level APIs
 * @note Part of Phase 6.5: Timer Implementation
 */

#include <iostream>
#include "hal/timer_simple.hpp"
#include "hal/timer_fluent.hpp"
#include "hal/timer_expert.hpp"
#include "hal/timer_dma.hpp"

using namespace alloy::hal;
using namespace alloy::core;

// Test counters
int tests_run = 0;
int tests_passed = 0;

#define TEST(name) \
    std::cout << "  "; \
    tests_run++; \
    if (test_##name()) { \
        std::cout << "✓ " << #name << std::endl; \
        tests_passed++; \
    } else { \
        std::cout << "✗ " << #name << std::endl; \
    }

// ============================================================================
// Timer Simple API Tests
// ============================================================================

bool test_quick_setup_default() {
    constexpr auto config = Timer<PeripheralId::TIMER0>::quick_setup();
    static_assert(config.mode == TimerMode::Periodic, "Default should be periodic");
    static_assert(config.period_us == 1000, "Default should be 1ms");
    return true;
}

bool test_quick_setup_custom() {
    constexpr auto config = Timer<PeripheralId::TIMER0>::quick_setup(
        TimerMode::OneShot, 5000);
    static_assert(config.mode == TimerMode::OneShot, "Should be one-shot");
    static_assert(config.period_us == 5000, "Should be 5ms");
    return true;
}

bool test_preset_ms_1() {
    constexpr auto config = Timer<PeripheralId::TIMER0>::ms_1();
    static_assert(config.mode == TimerMode::Periodic, "");
    static_assert(config.period_us == 1000, "Should be 1ms");
    return true;
}

bool test_preset_one_shot() {
    constexpr auto config = Timer<PeripheralId::TIMER0>::one_shot(10000);
    static_assert(config.mode == TimerMode::OneShot, "");
    static_assert(config.period_us == 10000, "Should be 10ms");
    return true;
}

// ============================================================================
// Timer Fluent API Tests
// ============================================================================

bool test_builder_basic() {
    auto result = TimerBuilder<PeripheralId::TIMER0>()
        .periodic()
        .ms(10)
        .initialize();

    if (!result.is_ok()) {
        return false;
    }

    auto config = std::move(result).unwrap();
    return config.config.mode == TimerMode::Periodic &&
           config.config.period_us == 10000;
}

bool test_builder_one_shot() {
    auto result = TimerBuilder<PeripheralId::TIMER0>()
        .one_shot()
        .us(5000)
        .initialize();

    if (!result.is_ok()) {
        return false;
    }

    auto config = std::move(result).unwrap();
    return config.config.mode == TimerMode::OneShot &&
           config.config.period_us == 5000;
}

bool test_builder_input_capture() {
    auto result = TimerBuilder<PeripheralId::TIMER0>()
        .input_capture()
        .capture_rising()
        .initialize();

    if (!result.is_ok()) {
        return false;
    }

    auto config = std::move(result).unwrap();
    return config.config.mode == TimerMode::InputCapture &&
           config.config.capture_edge == CaptureEdge::Rising;
}

bool test_builder_output_compare() {
    auto result = TimerBuilder<PeripheralId::TIMER0>()
        .output_compare()
        .compare(1000)
        .initialize();

    if (!result.is_ok()) {
        return false;
    }

    auto config = std::move(result).unwrap();
    return config.config.mode == TimerMode::OutputCompare &&
           config.config.compare_value == 1000;
}

// ============================================================================
// Timer Expert API Tests
// ============================================================================

bool test_expert_config_basic() {
    constexpr TimerExpertConfig config = {
        .peripheral = PeripheralId::TIMER0,
        .mode = TimerMode::Periodic,
        .period_us = 1000,
        .prescaler = 1,
        .capture_edge = CaptureEdge::Rising,
        .compare_value = 0,
        .enable_interrupts = true,
        .enable_dma = false,
        .auto_reload = true
    };
    
    static_assert(config.is_valid(), "Config should be valid");
    return true;
}

bool test_expert_preset_periodic() {
    constexpr auto config = TimerExpertConfig::periodic_interrupt(
        PeripheralId::TIMER0, 10000);
    
    static_assert(config.mode == TimerMode::Periodic, "");
    static_assert(config.enable_interrupts == true, "");
    static_assert(config.auto_reload == true, "");
    return true;
}

bool test_expert_preset_input_capture() {
    constexpr auto config = TimerExpertConfig::input_capture_dma(
        PeripheralId::TIMER0, CaptureEdge::Both);
    
    static_assert(config.mode == TimerMode::InputCapture, "");
    static_assert(config.capture_edge == CaptureEdge::Both, "");
    static_assert(config.enable_dma == true, "");
    return true;
}

// ============================================================================
// Timer DMA Integration Tests
// ============================================================================

bool test_timer_dma_config() {
    using Timer0UpdateDma = DmaConnection<
        PeripheralId::TIMER0,
        DmaRequest::TIMER0_UPDATE,
        DmaStream::Stream5
    >;

    constexpr auto config = TimerDmaConfig<Timer0UpdateDma>::create(
        PeripheralId::TIMER0,
        TimerMode::Periodic,
        1000
    );

    static_assert(config.is_valid(), "DMA config should be valid");
    static_assert(config.has_dma(), "Should have DMA enabled");
    return true;
}

bool test_timer_input_capture_dma() {
    using Timer0UpdateDma = DmaConnection<
        PeripheralId::TIMER0,
        DmaRequest::TIMER0_UPDATE,
        DmaStream::Stream5
    >;

    constexpr auto config = TimerDmaConfig<Timer0UpdateDma>::input_capture(
        PeripheralId::TIMER0,
        CaptureEdge::Rising
    );

    static_assert(config.timer_config.mode == TimerMode::InputCapture, "");
    static_assert(config.timer_config.enable_dma == true, "");
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "TimerSimpleApi:" << std::endl;
    TEST(quick_setup_default);
    TEST(quick_setup_custom);
    TEST(preset_ms_1);
    TEST(preset_one_shot);

    std::cout << "\nTimerFluentApi:" << std::endl;
    TEST(builder_basic);
    TEST(builder_one_shot);
    TEST(builder_input_capture);
    TEST(builder_output_compare);

    std::cout << "\nTimerExpertApi:" << std::endl;
    TEST(expert_config_basic);
    TEST(expert_preset_periodic);
    TEST(expert_preset_input_capture);

    std::cout << "\nTimerDmaIntegration:" << std::endl;
    TEST(timer_dma_config);
    TEST(timer_input_capture_dma);

    std::cout << "\n========================================" << std::endl;
    std::cout << "Tests run: " << tests_run << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << (tests_run - tests_passed) << std::endl;
    std::cout << "========================================" << std::endl;

    return (tests_run == tests_passed) ? 0 : 1;
}
