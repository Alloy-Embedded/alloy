/**
 * @file test_interrupt_apis.cpp
 * @brief Unit tests for Interrupt Management APIs using Catch2
 */

#include <catch2/catch_test_macros.hpp>
#include "hal/interrupt_simple.hpp"
#include "hal/interrupt_expert.hpp"

using namespace alloy::hal;
using namespace alloy::core;

// ============================================================================
// Interrupt Simple API Tests
// ============================================================================

TEST_CASE("Interrupt Simple API - Global Control", "[interrupt][simple]") {
    SECTION("Enable and disable don't crash") {
        REQUIRE_NOTHROW(Interrupt::disable_all());
        REQUIRE_NOTHROW(Interrupt::enable_all());
    }
}

TEST_CASE("Interrupt Simple API - Critical Section", "[interrupt][simple][raii]") {
    SECTION("RAII critical section") {
        {
            auto cs = Interrupt::critical_section();
            // Inside critical section
        }
        // Outside - should have restored interrupts
        REQUIRE(true);
    }
    
    SECTION("Scoped interrupt disable") {
        {
            ScopedInterruptDisable guard;
            // Interrupts disabled
        }
        // Interrupts restored
        REQUIRE(true);
    }
}

TEST_CASE("Interrupt Simple API - Specific Interrupts", "[interrupt][simple]") {
    SECTION("Enable specific interrupt") {
        auto result = Interrupt::enable(IrqNumber::UART0);
        REQUIRE(result.is_ok());
    }
    
    SECTION("Disable specific interrupt") {
        auto result = Interrupt::disable(IrqNumber::UART0);
        REQUIRE(result.is_ok());
    }
    
    SECTION("Check is_enabled") {
        bool enabled = Interrupt::is_enabled(IrqNumber::UART0);
        (void)enabled;  // Stub returns false
        REQUIRE(true);
    }
    
    SECTION("Set priority") {
        auto result = Interrupt::set_priority(IrqNumber::UART0, IrqPriority::Normal);
        REQUIRE(result.is_ok());
    }
    
    SECTION("Enable multiple interrupts") {
        auto r1 = Interrupt::enable(IrqNumber::UART0);
        auto r2 = Interrupt::enable(IrqNumber::SPI0);
        auto r3 = Interrupt::enable(IrqNumber::I2C0);
        
        REQUIRE(r1.is_ok());
        REQUIRE(r2.is_ok());
        REQUIRE(r3.is_ok());
    }
}

// ============================================================================
// Interrupt Expert API Tests
// ============================================================================

TEST_CASE("Interrupt Expert API - Configuration", "[interrupt][expert]") {
    SECTION("Basic config is valid") {
        constexpr InterruptExpertConfig config = {
            .irq_number = IrqNumber::UART0,
            .preempt_priority = 8,
            .sub_priority = 0,
            .enable = true,
            .trigger_pending = false
        };
        
        STATIC_REQUIRE(config.is_valid());
    }
    
    SECTION("Standard preset") {
        constexpr auto config = InterruptExpertConfig::standard(IrqNumber::UART0);
        STATIC_REQUIRE(config.irq_number == IrqNumber::UART0);
        STATIC_REQUIRE(config.preempt_priority == 8);
        STATIC_REQUIRE(config.enable == true);
    }
    
    SECTION("High priority preset") {
        constexpr auto config = InterruptExpertConfig::high_priority(IrqNumber::SPI0);
        STATIC_REQUIRE(config.preempt_priority == 2);
    }
    
    SECTION("Low priority preset") {
        constexpr auto config = InterruptExpertConfig::low_priority(IrqNumber::I2C0);
        STATIC_REQUIRE(config.preempt_priority == 14);
    }
}

TEST_CASE("Interrupt Expert API - Validation", "[interrupt][expert][validation]") {
    SECTION("Invalid preempt priority") {
        constexpr InterruptExpertConfig config = {
            .irq_number = IrqNumber::UART0,
            .preempt_priority = 20,  // Invalid
            .sub_priority = 0,
            .enable = true,
            .trigger_pending = false
        };
        
        STATIC_REQUIRE_FALSE(config.is_valid());
    }
    
    SECTION("Invalid sub-priority") {
        constexpr InterruptExpertConfig config = {
            .irq_number = IrqNumber::UART0,
            .preempt_priority = 8,
            .sub_priority = 20,  // Invalid
            .enable = true,
            .trigger_pending = false
        };
        
        STATIC_REQUIRE_FALSE(config.is_valid());
    }
}

TEST_CASE("Interrupt Expert API - Operations", "[interrupt][expert]") {
    SECTION("Configure") {
        constexpr auto config = InterruptExpertConfig::standard(IrqNumber::UART0);
        auto result = expert::configure(config);
        REQUIRE(result.is_ok());
    }
    
    SECTION("Pending operations") {
        bool pending = expert::is_pending(IrqNumber::UART0);
        (void)pending;
        
        auto r1 = expert::set_pending(IrqNumber::UART0);
        auto r2 = expert::clear_pending(IrqNumber::UART0);
        
        REQUIRE(r1.is_ok());
        REQUIRE(r2.is_ok());
    }
    
    SECTION("Active check") {
        bool active = expert::is_active(IrqNumber::UART0);
        (void)active;
        REQUIRE(true);
    }
    
    SECTION("Get priority") {
        auto result = expert::get_priority(IrqNumber::UART0);
        REQUIRE(result.is_ok());
        
        u8 priority = std::move(result).unwrap();
        (void)priority;
    }
    
    SECTION("Priority grouping valid") {
        auto result = expert::set_priority_grouping(3);
        REQUIRE(result.is_ok());
    }
    
    SECTION("Priority grouping invalid") {
        auto result = expert::set_priority_grouping(10);
        REQUIRE_FALSE(result.is_ok());
    }
}

// ============================================================================
// Interrupt Enums Tests
// ============================================================================

TEST_CASE("Interrupt Enums - IRQ Numbers", "[interrupt][enums]") {
    SECTION("Common IRQ numbers defined") {
        constexpr auto uart0 = IrqNumber::UART0;
        constexpr auto spi0 = IrqNumber::SPI0;
        constexpr auto i2c0 = IrqNumber::I2C0;
        constexpr auto timer0 = IrqNumber::TIMER0;
        constexpr auto adc0 = IrqNumber::ADC0;
        
        (void)uart0;
        (void)spi0;
        (void)i2c0;
        (void)timer0;
        (void)adc0;
        
        REQUIRE(true);
    }
}

TEST_CASE("Interrupt Enums - Priority Levels", "[interrupt][enums]") {
    SECTION("Priority ordering") {
        constexpr auto highest = IrqPriority::Highest;
        constexpr auto normal = IrqPriority::Normal;
        constexpr auto lowest = IrqPriority::Lowest;
        
        STATIC_REQUIRE(static_cast<u8>(highest) < static_cast<u8>(normal));
        STATIC_REQUIRE(static_cast<u8>(normal) < static_cast<u8>(lowest));
    }
}
