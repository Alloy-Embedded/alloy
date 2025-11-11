/**
 * @file test_systick_apis.cpp
 * @brief Unit tests for SysTick multi-level APIs
 * @note Part of Phase 6.6: SysTick Implementation
 */

#include <iostream>
#include "hal/systick_simple.hpp"
#include "hal/systick_fluent.hpp"
#include "hal/systick_expert.hpp"

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
// SysTick Simple API Tests
// ============================================================================

bool test_quick_setup() {
    constexpr auto config = SysTick::quick_setup();
    static_assert(config.tick_frequency_hz == 1000000, "Default should be 1MHz");
    return true;
}

bool test_preset_rtos() {
    constexpr auto config = SysTick::rtos_1ms();
    static_assert(config.tick_frequency_hz == 1000, "RTOS should be 1kHz");
    return true;
}

bool test_preset_micros() {
    constexpr auto config = SysTick::micros_1us();
    static_assert(config.tick_frequency_hz == 1000000, "Micros should be 1MHz");
    return true;
}

bool test_preset_millis() {
    constexpr auto config = SysTick::millis_1ms();
    static_assert(config.tick_frequency_hz == 1000, "Millis should be 1kHz");
    return true;
}

bool test_custom_frequency() {
    constexpr auto config = SysTick::custom(10000);
    static_assert(config.tick_frequency_hz == 10000, "Custom should be 10kHz");
    return true;
}

// ============================================================================
// SysTick Fluent API Tests
// ============================================================================

bool test_builder_micros() {
    auto result = SysTickBuilder()
        .micros()
        .initialize();

    if (!result.is_ok()) {
        return false;
    }

    auto config = std::move(result).unwrap();
    return config.config.tick_frequency_hz == 1000000;
}

bool test_builder_millis() {
    auto result = SysTickBuilder()
        .millis()
        .initialize();

    if (!result.is_ok()) {
        return false;
    }

    auto config = std::move(result).unwrap();
    return config.config.tick_frequency_hz == 1000;
}

bool test_builder_rtos() {
    auto result = SysTickBuilder()
        .rtos()
        .initialize();

    if (!result.is_ok()) {
        return false;
    }

    auto config = std::move(result).unwrap();
    return config.config.tick_frequency_hz == 1000;
}

bool test_builder_custom() {
    auto result = SysTickBuilder()
        .frequency_hz(50000)
        .initialize();

    if (!result.is_ok()) {
        return false;
    }

    auto config = std::move(result).unwrap();
    return config.config.tick_frequency_hz == 50000;
}

// ============================================================================
// SysTick Expert API Tests
// ============================================================================

bool test_expert_config_basic() {
    constexpr SysTickExpertConfig config = {
        .frequency_hz = 1000000,
        .interrupt_priority = 15,
        .enable_interrupt = true,
        .use_processor_clock = true
    };

    static_assert(config.is_valid(), "Config should be valid");
    return true;
}

bool test_expert_preset_micros() {
    constexpr auto config = SysTickExpertConfig::micros_timer();
    static_assert(config.frequency_hz == 1000000, "");
    static_assert(config.enable_interrupt == true, "");
    return true;
}

bool test_expert_preset_rtos() {
    constexpr auto config = SysTickExpertConfig::rtos_timebase();
    static_assert(config.frequency_hz == 1000, "");
    static_assert(config.enable_interrupt == true, "");
    return true;
}

bool test_expert_preset_polling() {
    constexpr auto config = SysTickExpertConfig::polling_mode(1000000);
    static_assert(config.frequency_hz == 1000000, "");
    static_assert(config.enable_interrupt == false, "");
    return true;
}

bool test_expert_validation_zero_freq() {
    constexpr SysTickExpertConfig config = {
        .frequency_hz = 0,
        .interrupt_priority = 15,
        .enable_interrupt = true,
        .use_processor_clock = true
    };

    static_assert(!config.is_valid(), "Zero frequency should be invalid");
    return true;
}

bool test_expert_validation_high_freq() {
    constexpr SysTickExpertConfig config = {
        .frequency_hz = 200000000,  // 200MHz - too high
        .interrupt_priority = 15,
        .enable_interrupt = true,
        .use_processor_clock = true
    };

    static_assert(!config.is_valid(), "200MHz should be invalid");
    return true;
}

bool test_expert_custom() {
    constexpr auto config = SysTickExpertConfig::custom(
        100000,  // 100kHz
        10,      // priority
        false,   // no interrupts
        true     // processor clock
    );

    static_assert(config.frequency_hz == 100000, "");
    static_assert(config.interrupt_priority == 10, "");
    static_assert(config.enable_interrupt == false, "");
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "SysTickSimpleApi:" << std::endl;
    TEST(quick_setup);
    TEST(preset_rtos);
    TEST(preset_micros);
    TEST(preset_millis);
    TEST(custom_frequency);

    std::cout << "\nSysTickFluentApi:" << std::endl;
    TEST(builder_micros);
    TEST(builder_millis);
    TEST(builder_rtos);
    TEST(builder_custom);

    std::cout << "\nSysTickExpertApi:" << std::endl;
    TEST(expert_config_basic);
    TEST(expert_preset_micros);
    TEST(expert_preset_rtos);
    TEST(expert_preset_polling);
    TEST(expert_validation_zero_freq);
    TEST(expert_validation_high_freq);
    TEST(expert_custom);

    std::cout << "\n========================================" << std::endl;
    std::cout << "Tests run: " << tests_run << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << (tests_run - tests_passed) << std::endl;
    std::cout << "========================================" << std::endl;

    return (tests_run == tests_passed) ? 0 : 1;
}
