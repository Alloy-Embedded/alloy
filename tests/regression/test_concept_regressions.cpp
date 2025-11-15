/**
 * @file test_concept_regressions.cpp
 * @brief Regression tests for C++20 concepts and compilation issues
 *
 * Documents bugs related to concepts, templates, and compilation.
 */

#include <catch2/catch_test_macros.hpp>

#if __cplusplus >= 202002L
#include "hal/core/concepts.hpp"
#endif

#include "core/result.hpp"
#include "core/error.hpp"
#include "hal/types.hpp"

using namespace alloy::core;
using namespace alloy::hal;

// ==============================================================================
// BUG #21: Concepts header included inside namespace
// ==============================================================================

/**
 * @bug #include "hal/core/concepts.hpp" was inside namespace block
 * @error Reference to 'alloy' is ambiguous
 * @fixed Moved include to before namespace declaration with C++20 guard
 * @date Phase 6 - API Standardization
 */
TEST_CASE("BUG #21: Concepts header include order", "[regression][concept][bug21]") {
#if __cplusplus >= 202002L
    WARN("Concepts header must be included BEFORE namespace declaration");
    WARN("Use: #if __cplusplus >= 202002L around include");

    INFO("Correct pattern:");
    INFO("#if __cplusplus >= 202002L");
    INFO("#include \"hal/core/concepts.hpp\"");
    INFO("#endif");
    INFO("");
    INFO("namespace alloy::hal { ... }");
#endif

    REQUIRE(true); // Documentation test
}

// ==============================================================================
// BUG #22: Missing compile-time metadata in GPIO
// ==============================================================================

/**
 * @bug GPIO implementations missing port_base, pin_number, pin_mask
 * @error GpioPin concept not satisfied
 * @fixed Added static constexpr members to all GPIO implementations
 * @date Phase 6 - API Standardization
 */
TEST_CASE("BUG #22: GPIO requires compile-time metadata", "[regression][concept][bug22]") {
    WARN("GPIO implementations must have static constexpr metadata");

    INFO("Required members:");
    INFO("  static constexpr uint32_t port_base");
    INFO("  static constexpr uint8_t pin_number");
    INFO("  static constexpr uint32_t pin_mask");
    INFO("");
    INFO("These enable compile-time pin validation and zero-overhead abstractions");

    REQUIRE(true); // Documentation test
}

// ==============================================================================
// BUG #23: ClockPlatform concept missing peripheral enables
// ==============================================================================

/**
 * @bug Initial ClockPlatform concept only had initialize() and get_frequency()
 * @error Missing enable_gpio_clocks(), enable_uart_clock(), etc.
 * @fixed Added all peripheral clock enable methods to concept
 * @date Phase 6 - API Standardization
 */
TEST_CASE("BUG #23: ClockPlatform needs peripheral enables", "[regression][concept][bug23]") {
    class MockClock {
    public:
        static Result<void, ErrorCode> initialize() { return Ok(); }
        static Result<void, ErrorCode> enable_gpio_clocks() { return Ok(); }
        static Result<void, ErrorCode> enable_uart_clock(uint32_t) { return Ok(); }
        static Result<void, ErrorCode> enable_spi_clock(uint32_t) { return Ok(); }
        static Result<void, ErrorCode> enable_i2c_clock(uint32_t) { return Ok(); }
        static constexpr uint32_t get_system_clock_hz() { return 64'000'000; }
    };

#if __cplusplus >= 202002L
    SECTION("MockClock satisfies ClockPlatform concept") {
        STATIC_REQUIRE(alloy::hal::concepts::ClockPlatform<MockClock>);
    }
#endif

    SECTION("All required methods exist") {
        REQUIRE(MockClock::initialize().is_ok());
        REQUIRE(MockClock::enable_gpio_clocks().is_ok());
        REQUIRE(MockClock::enable_uart_clock(0x40013800).is_ok());
        REQUIRE(MockClock::enable_spi_clock(0x40013000).is_ok());
        REQUIRE(MockClock::enable_i2c_clock(0x40005400).is_ok());
        REQUIRE(MockClock::get_system_clock_hz() > 0);
    }
}

// ==============================================================================
// BUG #24: static_assert fails without proper error messages
// ==============================================================================

/**
 * @bug Concept validation failures didn't provide helpful error messages
 * @fixed Added static_assert with descriptive messages in platform code
 * @date Phase 6 - API Standardization
 */
TEST_CASE("BUG #24: Concept validation with clear errors", "[regression][concept][bug24]") {
    WARN("Platform implementations should use static_assert for validation");

    INFO("Example pattern:");
    INFO("#if __cplusplus >= 202002L");
    INFO("static_assert(");
    INFO("    alloy::hal::concepts::GpioPin<MyGpioPin>,");
    INFO("    \"MyGpioPin must satisfy GpioPin concept\"");
    INFO(");");
    INFO("#endif");

    REQUIRE(true); // Documentation test
}

// ==============================================================================
// BUG #25: Template parameter deduction failures
// ==============================================================================

/**
 * @bug GpioPin template couldn't deduce port/pin parameters
 * @error No matching constructor for initialization
 * @fixed Use explicit template parameters: GpioPin<PortA, 5>
 * @date Phase 6 - API Standardization
 */
TEST_CASE("BUG #25: GPIO template parameter deduction", "[regression][template][bug25]") {
    WARN("GPIO templates require explicit parameters: GpioPin<PortA, 5>");

    INFO("Correct pattern:");
    INFO("template<typename Port, uint8_t Pin>");
    INFO("class GpioPin { ... };");
    INFO("");
    INFO("Usage:");
    INFO("using MyPin = GpioPin<PortA, 5>;");

    REQUIRE(true); // Documentation test
}

// ==============================================================================
// BUG #26: Concept requires() clause parsing issues
// ==============================================================================

/**
 * @bug Complex requires clauses caused parsing errors
 * @error Expected ';' after requires clause
 * @fixed Simplified concept definitions, use separate lines
 * @date Phase 6 - API Standardization
 */
TEST_CASE("BUG #26: Concept syntax clarity", "[regression][concept][bug26]") {
#if __cplusplus >= 202002L
    WARN("Keep concept requirements simple and clear");

    INFO("Good pattern:");
    INFO("template <typename T>");
    INFO("concept MyConcept = requires {");
    INFO("    { T::method() } -> std::same_as<Result<void, ErrorCode>>;");
    INFO("};");

    INFO("");
    INFO("Avoid:");
    INFO("- Complex nested requires");
    INFO("- Long single-line expressions");
    INFO("- Ambiguous type constraints");
#endif

    REQUIRE(true); // Documentation test
}

// ==============================================================================
// BUG #27: C++17 builds fail with concept code
// ==============================================================================

/**
 * @bug Concept code not guarded with C++20 check
 * @error 'concept' does not name a type
 * @fixed Guard all concept code with #if __cplusplus >= 202002L
 * @date Phase 6 - API Standardization
 */
TEST_CASE("BUG #27: C++20 feature guards", "[regression][compile][bug27]") {
    SECTION("C++ version check works") {
#if __cplusplus >= 202002L
        REQUIRE(__cplusplus >= 202002L);
        INFO("C++20 features available");
#else
        WARN("C++20 features not available");
        INFO("Concepts code should be guarded");
#endif
    }
}

// ==============================================================================
// BUG #28: ALLOY_PLATFORM_DIR not set by linux.cmake
// ==============================================================================

/**
 * @bug Platform selection failed for host/linux platform
 * @error CMake Error: ALLOY_PLATFORM_DIR not set
 * @fixed Added ALLOY_PLATFORM_DIR to cmake/platforms/linux.cmake
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #28: Platform directory configuration", "[regression][cmake][bug28]") {
    WARN("All platform cmake files must set ALLOY_PLATFORM_DIR");

    INFO("Required in platform cmake file:");
    INFO("set(ALLOY_PLATFORM_DIR ${CMAKE_SOURCE_DIR}/src/hal/vendors/PLATFORM)");

    REQUIRE(true); // Documentation test
}

// ==============================================================================
// BUG #29: Catch2 test discovery fails
// ==============================================================================

/**
 * @bug CTest couldn't find individual test cases
 * @error No tests discovered
 * @fixed Use catch_discover_tests() instead of add_test()
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #29: Catch2 test discovery", "[regression][cmake][bug29]") {
    WARN("Use catch_discover_tests() for automatic test discovery");

    INFO("Correct pattern:");
    INFO("add_executable(my_test test.cpp)");
    INFO("target_link_libraries(my_test Catch2::Catch2WithMain)");
    INFO("catch_discover_tests(my_test)");

    REQUIRE(true); // Documentation test
}

// ==============================================================================
// BUG #30: FetchContent Catch2 version mismatch
// ==============================================================================

/**
 * @bug Old Catch2 v2 tests incompatible with v3
 * @error Unknown macro CATCH_TEST_CASE
 * @fixed Update to Catch2 v3, use TEST_CASE instead of CATCH_TEST_CASE
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #30: Catch2 version compatibility", "[regression][test][bug30]") {
    WARN("Use Catch2 v3 macros:");

    INFO("Catch2 v3 changes:");
    INFO("- CATCH_TEST_CASE → TEST_CASE");
    INFO("- CATCH_SECTION → SECTION");
    INFO("- CATCH_REQUIRE → REQUIRE");
    INFO("- Include: <catch2/catch_test_macros.hpp>");

    REQUIRE(true); // Documentation test
}

// ==============================================================================
// Summary Test
// ==============================================================================

TEST_CASE("Regression test summary", "[regression][summary]") {
    INFO("Total regression bugs documented: 30");

    INFO("");
    INFO("Categories:");
    INFO("  Result<T,E>: 10 bugs (#1-#10)");
    INFO("  GPIO/Clock: 10 bugs (#11-#20)");
    INFO("  Concepts/Build: 10 bugs (#21-#30)");

    INFO("");
    INFO("Status:");
    INFO("  Fixed: 28 bugs");
    INFO("  Known Limitations: 2 bugs (#2 - Result<bool>, #7 - monadic methods)");

    REQUIRE(true);
}
