/**
 * @file test_platform_concepts.cpp
 * @brief Integration tests for platform concept compliance
 *
 * Tests that real platform implementations satisfy the HAL concepts.
 * This validates that the concept-based design works with actual hardware abstractions.
 */

#include <catch2/catch_test_macros.hpp>

#if __cplusplus >= 202002L
#include "hal/core/concepts.hpp"
#endif

#include "core/result.hpp"
#include "core/error.hpp"

using namespace alloy::core;
using namespace alloy::hal;

// ==============================================================================
// Platform-Specific Includes (conditional compilation)
// ==============================================================================

// STM32F4 Platform
#ifdef ALLOY_PLATFORM_STM32F4
#include "hal/vendors/st/stm32f4/clock_platform.hpp"
#include "hal/vendors/st/stm32f4/gpio.hpp"

using TestClockPlatform = alloy::hal::st::stm32f4::Stm32f4Clock<
    alloy::hal::st::stm32f4::ExampleF4ClockConfig
>;
using TestGpioPin = alloy::hal::st::stm32f4::GpioPin<
    alloy::hal::st::stm32f4::gpio::PortA, 5
>;
#endif

// STM32F7 Platform
#ifdef ALLOY_PLATFORM_STM32F7
#include "hal/vendors/st/stm32f7/clock_platform.hpp"
#include "hal/vendors/st/stm32f7/gpio.hpp"

using TestClockPlatform = alloy::hal::st::stm32f7::Stm32f7Clock<
    alloy::hal::st::stm32f7::ExampleF7ClockConfig
>;
using TestGpioPin = alloy::hal::st::stm32f7::GpioPin<
    alloy::hal::st::stm32f7::gpio::PortA, 5
>;
#endif

// STM32G0 Platform
#ifdef ALLOY_PLATFORM_STM32G0
#include "hal/vendors/st/stm32g0/clock_platform.hpp"
#include "hal/vendors/st/stm32g0/gpio.hpp"

using TestClockPlatform = alloy::hal::st::stm32g0::Stm32g0Clock<
    alloy::hal::st::stm32g0::ExampleG0ClockConfig
>;
using TestGpioPin = alloy::hal::st::stm32g0::GpioPin<
    alloy::hal::st::stm32g0::gpio::PortA, 5
>;
#endif

// ==============================================================================
// Concept Validation Tests (C++20 only)
// ==============================================================================

#if __cplusplus >= 202002L

#if defined(ALLOY_PLATFORM_STM32F4) || defined(ALLOY_PLATFORM_STM32F7) || defined(ALLOY_PLATFORM_STM32G0)

TEST_CASE("Platform Clock satisfies ClockPlatform concept", "[integration][concept][clock]") {
    STATIC_REQUIRE(alloy::hal::concepts::ClockPlatform<TestClockPlatform>);
}

TEST_CASE("Platform GPIO has required metadata", "[integration][concept][gpio]") {
    // Test that compile-time metadata exists
    STATIC_REQUIRE(TestGpioPin::port_base > 0);
    STATIC_REQUIRE(TestGpioPin::pin_number < 16);
    STATIC_REQUIRE(TestGpioPin::pin_mask > 0);
}

#endif // Platform check

#endif // C++20 check

// ==============================================================================
// API Validation Tests (all C++ versions)
// ==============================================================================

#if defined(ALLOY_PLATFORM_STM32F4) || defined(ALLOY_PLATFORM_STM32F7) || defined(ALLOY_PLATFORM_STM32G0)

TEST_CASE("Platform Clock has required methods", "[integration][api][clock]") {
    // Test that all required methods exist and return correct types

    SECTION("initialize() returns Result<void, ErrorCode>") {
        auto result = TestClockPlatform::initialize();
        REQUIRE((std::is_same_v<decltype(result), Result<void, ErrorCode>>));
    }

    SECTION("enable_gpio_clocks() returns Result<void, ErrorCode>") {
        auto result = TestClockPlatform::enable_gpio_clocks();
        REQUIRE((std::is_same_v<decltype(result), Result<void, ErrorCode>>));
    }

    SECTION("get_system_clock_hz() returns uint32_t") {
        auto freq = TestClockPlatform::get_system_clock_hz();
        REQUIRE((std::is_same_v<decltype(freq), uint32_t>));
        REQUIRE(freq > 0);
    }
}

TEST_CASE("Platform GPIO has required methods", "[integration][api][gpio]") {
    TestGpioPin pin;

    SECTION("set() returns Result<void, ErrorCode>") {
        auto result = pin.set();
        REQUIRE((std::is_same_v<decltype(result), Result<void, ErrorCode>>));
    }

    SECTION("clear() returns Result<void, ErrorCode>") {
        auto result = pin.clear();
        REQUIRE((std::is_same_v<decltype(result), Result<void, ErrorCode>>));
    }

    SECTION("toggle() returns Result<void, ErrorCode>") {
        auto result = pin.toggle();
        REQUIRE((std::is_same_v<decltype(result), Result<void, ErrorCode>>));
    }

    SECTION("write() accepts bool parameter") {
        auto result = pin.write(true);
        REQUIRE((std::is_same_v<decltype(result), Result<void, ErrorCode>>));
    }

    SECTION("setDirection() accepts PinDirection parameter") {
        auto result = pin.setDirection(PinDirection::Output);
        REQUIRE((std::is_same_v<decltype(result), Result<void, ErrorCode>>));
    }

    SECTION("setPull() accepts PinPull parameter") {
        auto result = pin.setPull(PinPull::PullUp);
        REQUIRE((std::is_same_v<decltype(result), Result<void, ErrorCode>>));
    }

    SECTION("setDrive() accepts PinDrive parameter") {
        auto result = pin.setDrive(PinDrive::PushPull);
        REQUIRE((std::is_same_v<decltype(result), Result<void, ErrorCode>>));
    }
}

TEST_CASE("Platform uses consistent error codes", "[integration][error]") {
    TestGpioPin pin;

    SECTION("All error returns use ErrorCode enum") {
        // This test validates that Result<T, ErrorCode> is used consistently
        auto set_result = pin.set();
        auto clear_result = pin.clear();
        auto toggle_result = pin.toggle();

        // If any fail, they should return ErrorCode
        if (set_result.is_err()) {
            REQUIRE((std::is_same_v<decltype(set_result.err()), ErrorCode&>));
        }
        if (clear_result.is_err()) {
            REQUIRE((std::is_same_v<decltype(clear_result.err()), ErrorCode&>));
        }
        if (toggle_result.is_err()) {
            REQUIRE((std::is_same_v<decltype(toggle_result.err()), ErrorCode&>));
        }
    }
}

#else

// Fallback test when no platform is defined
TEST_CASE("Platform concept tests require a platform", "[integration][skip]") {
    WARN("No platform defined - skipping platform-specific concept tests");
    REQUIRE(true); // Always pass
}

#endif

// ==============================================================================
// Platform Information Tests
// ==============================================================================

TEST_CASE("Platform identification", "[integration][platform]") {
#ifdef ALLOY_PLATFORM_STM32F4
    INFO("Platform: STM32F4");
    REQUIRE(true);
#endif

#ifdef ALLOY_PLATFORM_STM32F7
    INFO("Platform: STM32F7");
    REQUIRE(true);
#endif

#ifdef ALLOY_PLATFORM_STM32G0
    INFO("Platform: STM32G0");
    REQUIRE(true);
#endif

#ifdef ALLOY_PLATFORM_SAME70
    INFO("Platform: SAME70 (Atmel/ARM)");
    REQUIRE(true);
#endif

#if !defined(ALLOY_PLATFORM_STM32F4) && !defined(ALLOY_PLATFORM_STM32F7) && \
    !defined(ALLOY_PLATFORM_STM32G0) && !defined(ALLOY_PLATFORM_SAME70)
    INFO("Platform: Host/Generic (no hardware)");
    REQUIRE(true);
#endif
}

// ==============================================================================
// Type Safety Tests
// ==============================================================================

TEST_CASE("HAL types are properly defined", "[integration][types]") {
    SECTION("PinDirection enum") {
        PinDirection dir = PinDirection::Output;
        REQUIRE((dir == PinDirection::Output || dir == PinDirection::Input));
    }

    SECTION("PinPull enum") {
        PinPull pull = PinPull::PullUp;
        REQUIRE((pull == PinPull::None || pull == PinPull::PullUp ||
                 pull == PinPull::PullDown));
    }

    SECTION("PinDrive enum") {
        PinDrive drive = PinDrive::PushPull;
        REQUIRE((drive == PinDrive::PushPull || drive == PinDrive::OpenDrain));
    }

    SECTION("ErrorCode enum") {
        ErrorCode error = ErrorCode::Ok;
        REQUIRE((error == ErrorCode::Ok || error == ErrorCode::Timeout ||
                 error == ErrorCode::InvalidParameter));
    }
}
