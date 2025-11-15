/**
 * @file test_initialization_sequence.cpp
 * @brief Integration tests for complete system initialization
 *
 * Tests typical embedded system startup sequences:
 * 1. Clock initialization
 * 2. Peripheral clock enables
 * 3. GPIO configuration
 * 4. Basic peripheral operations
 */

#include <catch2/catch_test_macros.hpp>

#include "core/result.hpp"
#include "core/error.hpp"
#include "hal/types.hpp"

using namespace alloy::core;
using namespace alloy::hal;

// ==============================================================================
// System Initialization State Machine
// ==============================================================================

/**
 * @brief System initialization states
 */
enum class SystemState {
    PowerOn,        // Just powered on, nothing initialized
    ClockReady,     // System clock configured
    PeripheralsReady, // Peripheral clocks enabled
    GpioReady,      // GPIO configured
    FullyInitialized // All systems ready
};

/**
 * @brief System initialization manager
 *
 * Tracks the initialization sequence and enforces proper ordering.
 */
class SystemInitializer {
private:
    SystemState state = SystemState::PowerOn;
    uint32_t system_freq = 0;
    bool gpio_clocks_enabled = false;
    bool uart_clocks_enabled = false;
    bool spi_clocks_enabled = false;

public:
    /**
     * @brief Initialize system clock
     */
    Result<void, ErrorCode> init_clock(uint32_t target_freq_hz) {
        if (state != SystemState::PowerOn) {
            return Err(ErrorCode::AlreadyInitialized);
        }

        system_freq = target_freq_hz;
        state = SystemState::ClockReady;
        return Ok();
    }

    /**
     * @brief Enable peripheral clocks
     */
    Result<void, ErrorCode> enable_peripheral_clocks() {
        if (state != SystemState::ClockReady) {
            return Err(ErrorCode::NotInitialized);
        }

        gpio_clocks_enabled = true;
        uart_clocks_enabled = true;
        spi_clocks_enabled = true;

        state = SystemState::PeripheralsReady;
        return Ok();
    }

    /**
     * @brief Configure GPIO
     */
    Result<void, ErrorCode> configure_gpio() {
        if (state != SystemState::PeripheralsReady) {
            return Err(ErrorCode::NotInitialized);
        }

        state = SystemState::GpioReady;
        return Ok();
    }

    /**
     * @brief Finalize initialization
     */
    Result<void, ErrorCode> finalize() {
        if (state != SystemState::GpioReady) {
            return Err(ErrorCode::NotInitialized);
        }

        state = SystemState::FullyInitialized;
        return Ok();
    }

    /**
     * @brief Get current state
     */
    SystemState get_state() const {
        return state;
    }

    /**
     * @brief Get system frequency
     */
    uint32_t get_frequency() const {
        return system_freq;
    }

    /**
     * @brief Check if peripheral clocks are enabled
     */
    bool are_gpio_clocks_enabled() const {
        return gpio_clocks_enabled;
    }

    /**
     * @brief Reset to power-on state
     */
    void reset() {
        state = SystemState::PowerOn;
        system_freq = 0;
        gpio_clocks_enabled = false;
        uart_clocks_enabled = false;
        spi_clocks_enabled = false;
    }
};

// ==============================================================================
// Initialization Sequence Tests
// ==============================================================================

TEST_CASE("Correct initialization sequence succeeds", "[integration][init][sequence]") {
    SystemInitializer system;

    SECTION("Step-by-step initialization") {
        // Step 1: Power on state
        REQUIRE(system.get_state() == SystemState::PowerOn);

        // Step 2: Initialize clock
        auto clock_result = system.init_clock(64'000'000);
        REQUIRE(clock_result.is_ok());
        REQUIRE(system.get_state() == SystemState::ClockReady);
        REQUIRE(system.get_frequency() == 64'000'000);

        // Step 3: Enable peripherals
        auto periph_result = system.enable_peripheral_clocks();
        REQUIRE(periph_result.is_ok());
        REQUIRE(system.get_state() == SystemState::PeripheralsReady);
        REQUIRE(system.are_gpio_clocks_enabled());

        // Step 4: Configure GPIO
        auto gpio_result = system.configure_gpio();
        REQUIRE(gpio_result.is_ok());
        REQUIRE(system.get_state() == SystemState::GpioReady);

        // Step 5: Finalize
        auto final_result = system.finalize();
        REQUIRE(final_result.is_ok());
        REQUIRE(system.get_state() == SystemState::FullyInitialized);
    }
}

TEST_CASE("Wrong initialization order fails", "[integration][init][error]") {
    SystemInitializer system;

    SECTION("Cannot enable peripherals before clock") {
        auto result = system.enable_peripheral_clocks();

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::NotInitialized);
    }

    SECTION("Cannot configure GPIO before peripherals") {
        system.init_clock(64'000'000);

        auto result = system.configure_gpio();

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::NotInitialized);
    }

    SECTION("Cannot finalize without GPIO configuration") {
        system.init_clock(64'000'000);
        system.enable_peripheral_clocks();

        auto result = system.finalize();

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::NotInitialized);
    }
}

TEST_CASE("Cannot initialize clock twice", "[integration][init][error]") {
    SystemInitializer system;

    // First initialization succeeds
    REQUIRE(system.init_clock(64'000'000).is_ok());

    // Second initialization fails
    auto result = system.init_clock(80'000'000);

    REQUIRE(result.is_err());
    REQUIRE(result.err() == ErrorCode::AlreadyInitialized);
}

TEST_CASE("System reset allows re-initialization", "[integration][init][reset]") {
    SystemInitializer system;

    // First initialization
    REQUIRE(system.init_clock(64'000'000).is_ok());
    REQUIRE(system.get_state() == SystemState::ClockReady);

    // Reset
    system.reset();
    REQUIRE(system.get_state() == SystemState::PowerOn);

    // Can initialize again
    REQUIRE(system.init_clock(80'000'000).is_ok());
    REQUIRE(system.get_frequency() == 80'000'000);
}

// ==============================================================================
// Multi-step Initialization Scenarios
// ==============================================================================

TEST_CASE("Typical embedded system startup", "[integration][scenario][startup]") {
    SystemInitializer system;

    // Startup sequence
    INFO("1. Initializing system clock...");
    REQUIRE(system.init_clock(64'000'000).is_ok());

    INFO("2. Enabling peripheral clocks...");
    REQUIRE(system.enable_peripheral_clocks().is_ok());

    INFO("3. Configuring GPIO pins...");
    REQUIRE(system.configure_gpio().is_ok());

    INFO("4. Finalizing initialization...");
    REQUIRE(system.finalize().is_ok());

    // Verify final state
    REQUIRE(system.get_state() == SystemState::FullyInitialized);
    REQUIRE(system.get_frequency() == 64'000'000);
    REQUIRE(system.are_gpio_clocks_enabled());
}

TEST_CASE("Different clock frequencies", "[integration][clock][freq]") {
    SystemInitializer system;

    SECTION("64 MHz system") {
        REQUIRE(system.init_clock(64'000'000).is_ok());
        REQUIRE(system.get_frequency() == 64'000'000);
    }

    SECTION("80 MHz system") {
        REQUIRE(system.init_clock(80'000'000).is_ok());
        REQUIRE(system.get_frequency() == 80'000'000);
    }

    SECTION("100 MHz system") {
        REQUIRE(system.init_clock(100'000'000).is_ok());
        REQUIRE(system.get_frequency() == 100'000'000);
    }

    SECTION("168 MHz system (high performance)") {
        REQUIRE(system.init_clock(168'000'000).is_ok());
        REQUIRE(system.get_frequency() == 168'000'000);
    }
}

// ==============================================================================
// Error Recovery Tests
// ==============================================================================

TEST_CASE("Initialization error recovery", "[integration][error][recovery]") {
    SystemInitializer system;

    SECTION("Failed peripheral enable can be retried after clock init") {
        // Try to enable peripherals too early
        REQUIRE(system.enable_peripheral_clocks().is_err());

        // Initialize clock
        REQUIRE(system.init_clock(64'000'000).is_ok());

        // Now peripheral enable should succeed
        REQUIRE(system.enable_peripheral_clocks().is_ok());
    }

    SECTION("Failed GPIO config can be retried after peripheral enable") {
        // Setup: clock initialized
        REQUIRE(system.init_clock(64'000'000).is_ok());

        // Try GPIO config too early
        REQUIRE(system.configure_gpio().is_err());

        // Enable peripherals
        REQUIRE(system.enable_peripheral_clocks().is_ok());

        // Now GPIO config should succeed
        REQUIRE(system.configure_gpio().is_ok());
    }
}

TEST_CASE("State transitions are atomic", "[integration][state]") {
    SystemInitializer system;

    // Each step should only transition to the next state
    REQUIRE(system.get_state() == SystemState::PowerOn);

    system.init_clock(64'000'000);
    REQUIRE(system.get_state() == SystemState::ClockReady);

    system.enable_peripheral_clocks();
    REQUIRE(system.get_state() == SystemState::PeripheralsReady);

    system.configure_gpio();
    REQUIRE(system.get_state() == SystemState::GpioReady);

    system.finalize();
    REQUIRE(system.get_state() == SystemState::FullyInitialized);
}

// ==============================================================================
// Integration with Result<T, E> Pattern
// ==============================================================================

TEST_CASE("Initialization uses Result<T, E> correctly", "[integration][result]") {
    SystemInitializer system;

    SECTION("Success path returns Ok()") {
        auto result = system.init_clock(64'000'000);

        REQUIRE(result.is_ok());
        REQUIRE_FALSE(result.is_err());
    }

    SECTION("Error path returns Err with proper error code") {
        auto result = system.enable_peripheral_clocks();

        REQUIRE(result.is_err());
        REQUIRE_FALSE(result.is_ok());
        REQUIRE(result.err() == ErrorCode::NotInitialized);
    }

    SECTION("Results can be chained with early return pattern") {
        auto init_sequence = [&system]() -> Result<void, ErrorCode> {
            auto clock_result = system.init_clock(64'000'000);
            if (clock_result.is_err()) {
                return clock_result;
            }

            auto periph_result = system.enable_peripheral_clocks();
            if (periph_result.is_err()) {
                return periph_result;
            }

            return system.configure_gpio();
        };

        auto result = init_sequence();
        REQUIRE(result.is_ok());
    }
}
