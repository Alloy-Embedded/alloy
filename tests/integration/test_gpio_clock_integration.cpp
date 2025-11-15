/**
 * @file test_gpio_clock_integration.cpp
 * @brief Integration tests for GPIO + Clock interaction
 *
 * Tests that GPIO and Clock components work together correctly.
 * Validates typical embedded initialization sequences.
 */

#include <catch2/catch_test_macros.hpp>

#include "hal/core/concepts.hpp"
#include "core/result.hpp"
#include "core/error.hpp"
#include "hal/types.hpp"

using namespace alloy::core;
using namespace alloy::hal;

// ==============================================================================
// Mock Integrated System
// ==============================================================================

/**
 * @brief Mock system that combines Clock and GPIO
 *
 * Simulates a typical embedded system initialization where:
 * 1. System clock is configured
 * 2. Peripheral clocks are enabled
 * 3. GPIO pins are configured
 */
class MockEmbeddedSystem {
private:
    static inline bool clock_initialized = false;
    static inline bool gpio_clocks_enabled = false;
    static inline uint32_t system_freq_hz = 0;

public:
    static constexpr uint32_t TARGET_FREQ_HZ = 64'000'000;

    /**
     * @brief Initialize system clock
     */
    static Result<void, ErrorCode> init_system_clock() {
        if (clock_initialized) {
            return Err(ErrorCode::AlreadyInitialized);
        }
        system_freq_hz = TARGET_FREQ_HZ;
        clock_initialized = true;
        return Ok();
    }

    /**
     * @brief Enable GPIO peripheral clocks
     */
    static Result<void, ErrorCode> enable_gpio_peripherals() {
        if (!clock_initialized) {
            return Err(ErrorCode::NotInitialized);
        }
        gpio_clocks_enabled = true;
        return Ok();
    }

    /**
     * @brief Get system frequency
     */
    static uint32_t get_system_frequency() {
        return system_freq_hz;
    }

    /**
     * @brief Check if GPIO clocks are enabled
     */
    static bool are_gpio_clocks_enabled() {
        return gpio_clocks_enabled;
    }

    /**
     * @brief Reset system state (for testing)
     */
    static void reset() {
        clock_initialized = false;
        gpio_clocks_enabled = false;
        system_freq_hz = 0;
    }
};

/**
 * @brief Mock GPIO pin that requires clock initialization
 */
class MockSystemGpioPin {
private:
    bool configured = false;
    bool state = false;
    PinDirection direction = PinDirection::Input;

public:
    static constexpr uint32_t port_base = 0x50000000;
    static constexpr uint8_t pin_number = 5;
    static constexpr uint32_t pin_mask = (1U << pin_number);

    /**
     * @brief Configure GPIO pin
     * @note Requires GPIO clocks to be enabled first
     */
    Result<void, ErrorCode> configure(PinDirection dir) {
        if (!MockEmbeddedSystem::are_gpio_clocks_enabled()) {
            return Err(ErrorCode::NotInitialized);
        }
        direction = dir;
        configured = true;
        return Ok();
    }

    /**
     * @brief Set pin HIGH
     */
    Result<void, ErrorCode> set() {
        if (!configured) {
            return Err(ErrorCode::NotInitialized);
        }
        if (direction != PinDirection::Output) {
            return Err(ErrorCode::InvalidParameter);
        }
        state = true;
        return Ok();
    }

    /**
     * @brief Set pin LOW
     */
    Result<void, ErrorCode> clear() {
        if (!configured) {
            return Err(ErrorCode::NotInitialized);
        }
        if (direction != PinDirection::Output) {
            return Err(ErrorCode::InvalidParameter);
        }
        state = false;
        return Ok();
    }

    /**
     * @brief Read pin state
     */
    bool read() const {
        return state;
    }

    /**
     * @brief Check if configured
     */
    bool is_configured() const {
        return configured;
    }
};

// ==============================================================================
// Integration Test Cases
// ==============================================================================

TEST_CASE("GPIO requires clock initialization", "[integration][gpio][clock]") {
    MockEmbeddedSystem::reset();
    MockSystemGpioPin pin;

    SECTION("GPIO configuration fails without clock") {
        auto result = pin.configure(PinDirection::Output);

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::NotInitialized);
    }

    SECTION("GPIO configuration succeeds after clock init") {
        // Initialize system
        REQUIRE(MockEmbeddedSystem::init_system_clock().is_ok());
        REQUIRE(MockEmbeddedSystem::enable_gpio_peripherals().is_ok());

        // Now GPIO should work
        auto result = pin.configure(PinDirection::Output);

        REQUIRE(result.is_ok());
        REQUIRE(pin.is_configured());
    }
}

TEST_CASE("Complete system initialization sequence", "[integration][system]") {
    MockEmbeddedSystem::reset();
    MockSystemGpioPin led;

    // Step 1: Initialize system clock
    auto clock_init = MockEmbeddedSystem::init_system_clock();
    REQUIRE(clock_init.is_ok());
    REQUIRE(MockEmbeddedSystem::get_system_frequency() == MockEmbeddedSystem::TARGET_FREQ_HZ);

    // Step 2: Enable GPIO clocks
    auto gpio_clocks = MockEmbeddedSystem::enable_gpio_peripherals();
    REQUIRE(gpio_clocks.is_ok());

    // Step 3: Configure GPIO pin
    auto gpio_config = led.configure(PinDirection::Output);
    REQUIRE(gpio_config.is_ok());

    // Step 4: Use GPIO
    REQUIRE(led.set().is_ok());
    REQUIRE(led.read() == true);

    REQUIRE(led.clear().is_ok());
    REQUIRE(led.read() == false);
}

TEST_CASE("Clock must be initialized before GPIO clocks", "[integration][clock]") {
    MockEmbeddedSystem::reset();

    auto result = MockEmbeddedSystem::enable_gpio_peripherals();

    REQUIRE(result.is_err());
    REQUIRE(result.err() == ErrorCode::NotInitialized);
}

TEST_CASE("Clock cannot be initialized twice", "[integration][clock][error]") {
    MockEmbeddedSystem::reset();

    // First initialization succeeds
    REQUIRE(MockEmbeddedSystem::init_system_clock().is_ok());

    // Second initialization fails
    auto result = MockEmbeddedSystem::init_system_clock();

    REQUIRE(result.is_err());
    REQUIRE(result.err() == ErrorCode::AlreadyInitialized);
}

TEST_CASE("Blink LED simulation with proper initialization", "[integration][scenario]") {
    MockEmbeddedSystem::reset();
    MockSystemGpioPin led;

    // Initialize system properly
    REQUIRE(MockEmbeddedSystem::init_system_clock().is_ok());
    REQUIRE(MockEmbeddedSystem::enable_gpio_peripherals().is_ok());
    REQUIRE(led.configure(PinDirection::Output).is_ok());

    // Simulate blink pattern
    for (int i = 0; i < 5; i++) {
        REQUIRE(led.set().is_ok());
        REQUIRE(led.read() == true);

        REQUIRE(led.clear().is_ok());
        REQUIRE(led.read() == false);
    }
}

TEST_CASE("GPIO operations fail without proper initialization", "[integration][error]") {
    MockEmbeddedSystem::reset();
    MockSystemGpioPin pin;

    SECTION("set() fails when not configured") {
        auto result = pin.set();
        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::NotInitialized);
    }

    SECTION("clear() fails when not configured") {
        auto result = pin.clear();
        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::NotInitialized);
    }
}

TEST_CASE("System clock frequency is available", "[integration][clock]") {
    MockEmbeddedSystem::reset();

    // Before init, frequency is 0
    REQUIRE(MockEmbeddedSystem::get_system_frequency() == 0);

    // After init, frequency is set
    REQUIRE(MockEmbeddedSystem::init_system_clock().is_ok());
    REQUIRE(MockEmbeddedSystem::get_system_frequency() == 64'000'000);
}

TEST_CASE("Multiple GPIO pins can be configured", "[integration][gpio][multi]") {
    MockEmbeddedSystem::reset();
    MockSystemGpioPin led1, led2, led3;

    // Initialize system once
    REQUIRE(MockEmbeddedSystem::init_system_clock().is_ok());
    REQUIRE(MockEmbeddedSystem::enable_gpio_peripherals().is_ok());

    // Configure multiple pins
    REQUIRE(led1.configure(PinDirection::Output).is_ok());
    REQUIRE(led2.configure(PinDirection::Output).is_ok());
    REQUIRE(led3.configure(PinDirection::Output).is_ok());

    // Use all pins independently
    REQUIRE(led1.set().is_ok());
    REQUIRE(led2.clear().is_ok());
    REQUIRE(led3.set().is_ok());

    REQUIRE(led1.read() == true);
    REQUIRE(led2.read() == false);
    REQUIRE(led3.read() == true);
}
