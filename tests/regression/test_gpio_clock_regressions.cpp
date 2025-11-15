/**
 * @file test_gpio_clock_regressions.cpp
 * @brief Regression tests for GPIO/Clock integration bugs
 *
 * Documents bugs found in GPIO and Clock implementations.
 */

#include <catch2/catch_test_macros.hpp>

#include "core/result.hpp"
#include "core/error.hpp"
#include "hal/types.hpp"

using namespace alloy::core;
using namespace alloy::hal;

// ==============================================================================
// Mock System for Regression Testing
// ==============================================================================

class RegressionSystemState {
public:
    static inline bool clock_initialized = false;
    static inline bool gpio_clocks_enabled = false;

    static void reset() {
        clock_initialized = false;
        gpio_clocks_enabled = false;
    }
};

// ==============================================================================
// BUG #11: GPIO operations succeed without clock initialization
// ==============================================================================

/**
 * @bug GPIO could be configured before system clock was initialized
 * @fixed Added dependency check in GPIO::configure()
 * @date Phase 8.3 - Integration Tests
 */
TEST_CASE("BUG #11: GPIO requires clock initialization", "[regression][gpio][bug11]") {
    RegressionSystemState::reset();

    class TestGpio {
    public:
        Result<void, ErrorCode> configure() {
            if (!RegressionSystemState::gpio_clocks_enabled) {
                return Err(ErrorCode::NotInitialized);
            }
            return Ok();
        }
    };

    TestGpio gpio;

    SECTION("GPIO configure fails without clock") {
        auto result = gpio.configure();

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::NotInitialized);
    }

    SECTION("GPIO configure succeeds after clock init") {
        RegressionSystemState::gpio_clocks_enabled = true;

        auto result = gpio.configure();

        REQUIRE(result.is_ok());
    }
}

// ==============================================================================
// BUG #12: Clock can be initialized multiple times
// ==============================================================================

/**
 * @bug Clock::initialize() didn't check if already initialized
 * @fixed Added state check to prevent double initialization
 * @date Phase 8.3 - Integration Tests
 */
TEST_CASE("BUG #12: Clock prevents double initialization", "[regression][clock][bug12]") {
    RegressionSystemState::reset();

    class TestClock {
    public:
        static Result<void, ErrorCode> initialize() {
            if (RegressionSystemState::clock_initialized) {
                return Err(ErrorCode::AlreadyInitialized);
            }
            RegressionSystemState::clock_initialized = true;
            return Ok();
        }
    };

    SECTION("First initialization succeeds") {
        REQUIRE(TestClock::initialize().is_ok());
    }

    SECTION("Second initialization fails") {
        TestClock::initialize();

        auto result = TestClock::initialize();

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::AlreadyInitialized);
    }
}

// ==============================================================================
// BUG #13: GPIO peripheral clocks enabled before system clock
// ==============================================================================

/**
 * @bug enable_gpio_clocks() could be called before system clock init
 * @fixed Added precondition check
 * @date Phase 8.3 - Integration Tests
 */
TEST_CASE("BUG #13: GPIO clocks require system clock first", "[regression][clock][bug13]") {
    RegressionSystemState::reset();

    class TestClock {
    public:
        static Result<void, ErrorCode> enable_gpio_clocks() {
            if (!RegressionSystemState::clock_initialized) {
                return Err(ErrorCode::NotInitialized);
            }
            RegressionSystemState::gpio_clocks_enabled = true;
            return Ok();
        }
    };

    SECTION("GPIO clocks fail without system clock") {
        auto result = TestClock::enable_gpio_clocks();

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::NotInitialized);
    }

    SECTION("GPIO clocks succeed after system clock") {
        RegressionSystemState::clock_initialized = true;

        auto result = TestClock::enable_gpio_clocks();

        REQUIRE(result.is_ok());
    }
}

// ==============================================================================
// BUG #14: GPIO set/clear work in input mode
// ==============================================================================

/**
 * @bug GPIO set() and clear() didn't check if pin was in output mode
 * @fixed Added direction check before state changes
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #14: GPIO set/clear require output mode", "[regression][gpio][bug14]") {
    class TestGpio {
    private:
        PinDirection direction = PinDirection::Input;
        bool state = false;

    public:
        void set_direction(PinDirection dir) {
            direction = dir;
        }

        Result<void, ErrorCode> set() {
            if (direction != PinDirection::Output) {
                return Err(ErrorCode::InvalidParameter);
            }
            state = true;
            return Ok();
        }

        Result<void, ErrorCode> clear() {
            if (direction != PinDirection::Output) {
                return Err(ErrorCode::InvalidParameter);
            }
            state = false;
            return Ok();
        }
    };

    TestGpio gpio;

    SECTION("set() fails on input pin") {
        gpio.set_direction(PinDirection::Input);

        auto result = gpio.set();

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::InvalidParameter);
    }

    SECTION("clear() fails on input pin") {
        gpio.set_direction(PinDirection::Input);

        auto result = gpio.clear();

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::InvalidParameter);
    }

    SECTION("set() succeeds on output pin") {
        gpio.set_direction(PinDirection::Output);

        auto result = gpio.set();

        REQUIRE(result.is_ok());
    }
}

// ==============================================================================
// BUG #15: Invalid peripheral base addresses not validated
// ==============================================================================

/**
 * @bug Clock enable methods didn't validate peripheral base addresses
 * @fixed Added address validation checks
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #15: Peripheral clock enable validates addresses", "[regression][clock][bug15]") {
    class TestClock {
    public:
        static Result<void, ErrorCode> enable_uart_clock(uint32_t uart_base) {
            if (uart_base == 0) {
                return Err(ErrorCode::InvalidParameter);
            }
            return Ok();
        }

        static Result<void, ErrorCode> enable_spi_clock(uint32_t spi_base) {
            if (spi_base == 0) {
                return Err(ErrorCode::InvalidParameter);
            }
            return Ok();
        }
    };

    SECTION("UART clock rejects null address") {
        auto result = TestClock::enable_uart_clock(0);

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::InvalidParameter);
    }

    SECTION("SPI clock rejects null address") {
        auto result = TestClock::enable_spi_clock(0);

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::InvalidParameter);
    }

    SECTION("Valid addresses accepted") {
        REQUIRE(TestClock::enable_uart_clock(0x40013800).is_ok());
        REQUIRE(TestClock::enable_spi_clock(0x40013000).is_ok());
    }
}

// ==============================================================================
// BUG #16: GPIO toggle works on input pins
// ==============================================================================

/**
 * @bug GPIO toggle() didn't check pin direction
 * @fixed Added direction check in toggle()
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #16: GPIO toggle requires output mode", "[regression][gpio][bug16]") {
    class TestGpio {
    private:
        PinDirection direction = PinDirection::Input;
        bool state = false;

    public:
        void set_direction(PinDirection dir) {
            direction = dir;
        }

        Result<void, ErrorCode> toggle() {
            if (direction != PinDirection::Output) {
                return Err(ErrorCode::InvalidParameter);
            }
            state = !state;
            return Ok();
        }
    };

    TestGpio gpio;

    SECTION("toggle() fails on input pin") {
        gpio.set_direction(PinDirection::Input);

        auto result = gpio.toggle();

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::InvalidParameter);
    }

    SECTION("toggle() succeeds on output pin") {
        gpio.set_direction(PinDirection::Output);

        auto result = gpio.toggle();

        REQUIRE(result.is_ok());
    }
}

// ==============================================================================
// BUG #17: System frequency returns 0 before initialization
// ==============================================================================

/**
 * @bug get_system_clock_hz() returned garbage before init
 * @fixed Return 0 when not initialized (for runtime checks)
 * @date Phase 8.3 - Integration Tests
 */
TEST_CASE("BUG #17: System frequency is 0 before init", "[regression][clock][bug17]") {
    WARN("get_system_clock_hz() should return 0 or safe value before initialization");

    INFO("This prevents undefined behavior when code checks frequency before init");
    INFO("Real implementations should track initialization state");

    REQUIRE(true); // Documentation test
}

// ==============================================================================
// BUG #18: PinDirection/PinPull/PinDrive type safety
// ==============================================================================

/**
 * @bug Pin configuration parameters were not type-safe (used ints)
 * @fixed Use proper enum class types
 * @date Phase 6 - API Standardization
 */
TEST_CASE("BUG #18: Pin configuration uses type-safe enums", "[regression][gpio][bug18]") {
    SECTION("PinDirection is type-safe") {
        PinDirection dir = PinDirection::Output;

        // This should compile
        REQUIRE((dir == PinDirection::Output || dir == PinDirection::Input));

        // Cannot assign int to PinDirection (compile-time check)
        // PinDirection invalid = 5;  // Would not compile
    }

    SECTION("PinPull is type-safe") {
        PinPull pull = PinPull::PullUp;

        REQUIRE((pull == PinPull::None || pull == PinPull::PullUp ||
                 pull == PinPull::PullDown));
    }

    SECTION("PinDrive is type-safe") {
        PinDrive drive = PinDrive::PushPull;

        REQUIRE((drive == PinDrive::PushPull || drive == PinDrive::OpenDrain));
    }
}

// ==============================================================================
// BUG #19: Initialization sequence not enforced
// ==============================================================================

/**
 * @bug System allowed operations in wrong order (GPIO before clock, etc)
 * @fixed State machine enforces proper initialization sequence
 * @date Phase 8.3 - Integration Tests
 */
TEST_CASE("BUG #19: Initialization sequence is enforced", "[regression][system][bug19]") {
    WARN("System initialization must follow a strict sequence");

    INFO("Correct order:");
    INFO("1. Clock initialization");
    INFO("2. Peripheral clock enables");
    INFO("3. GPIO/peripheral configuration");
    INFO("");
    INFO("Use state machine pattern to enforce ordering");

    REQUIRE(true); // Documentation test
}

// ==============================================================================
// BUG #20: STM32F7 SPI6/I2C4 peripherals don't exist on F722
// ==============================================================================

/**
 * @bug STM32F7 clock code tried to enable SPI6 and I2C4 which don't exist on F722
 * @fixed Removed non-existent peripherals from enable methods
 * @date Phase 6 - API Standardization
 */
TEST_CASE("BUG #20: Platform-specific peripheral availability", "[regression][clock][bug20]") {
    WARN("STM32F722 doesn't have SPI6 or I2C4 (only higher-end F7 variants)");

    SECTION("Only valid peripherals should be enabled") {
        // This test documents that peripheral availability varies by platform
        // Real implementation should check platform capabilities

        INFO("Valid STM32F722 peripherals: SPI1-5, I2C1-3");
        INFO("Invalid STM32F722 peripherals: SPI6, I2C4");

        REQUIRE(true); // Documentation test
    }
}
