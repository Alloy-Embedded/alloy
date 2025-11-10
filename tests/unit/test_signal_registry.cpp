/**
 * @file test_signal_registry.cpp
 * @brief Unit tests for Signal Routing Registry (Phase 3.3)
 *
 * Tests the compile-time registry for tracking pin allocations and
 * detecting conflicts.
 *
 * @see openspec/changes/modernize-peripheral-architecture/specs/signal-routing/spec.md
 */

#include <cassert>
#include <iostream>

#include "../../src/hal/signal_registry.hpp"
#include "../../src/hal/signals.hpp"

using namespace alloy::hal::signal_registry;
using namespace alloy::hal::signals;

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                                \
    void test_##name();                                           \
    void run_test_##name() {                                      \
        tests_run++;                                              \
        std::cout << "Running test: " #name << "...";             \
        try {                                                     \
            test_##name();                                        \
            tests_passed++;                                       \
            std::cout << " PASS" << std::endl;                    \
        } catch (const std::exception& e) {                       \
            std::cout << " FAIL: " << e.what() << std::endl;      \
        } catch (...) {                                           \
            std::cout << " FAIL: Unknown exception" << std::endl; \
        }                                                         \
    }                                                             \
    void test_##name()

#define ASSERT(condition)                                              \
    do {                                                               \
        if (!(condition)) {                                            \
            throw std::runtime_error("Assertion failed: " #condition); \
        }                                                              \
    } while (0)

// =============================================================================
// Test Registries
// =============================================================================

// Empty registry
using EmptyReg = EmptyRegistry;

// Single allocation
constexpr auto alloc1 = ALLOC_PIN(PinId::PD4, PeripheralId::USART0,
                                   SignalType::RX);
using Registry1 = AddAllocation_t<EmptyReg, alloc1>;

// Two allocations (no conflict)
constexpr auto alloc2 = ALLOC_PIN(PinId::PA3, PeripheralId::TWI0,
                                   SignalType::SDA);
using Registry2 = AddAllocation_t<Registry1, alloc2>;

// Three allocations (no conflict)
constexpr auto alloc3 = ALLOC_PIN(PinId::PA4, PeripheralId::TWI0,
                                   SignalType::SCL);
using Registry3 = AddAllocation_t<Registry2, alloc3>;

// Conflicting allocation (same pin as alloc1)
constexpr auto conflict_alloc = ALLOC_PIN(PinId::PD4, PeripheralId::SPI0,
                                           SignalType::MOSI);
using ConflictRegistry = AddAllocation_t<Registry1, conflict_alloc>;

// =============================================================================
// Empty Registry Tests
// =============================================================================

TEST(empty_registry_size_zero) {
    ASSERT(EmptyReg::size == 0);
}

TEST(empty_registry_no_allocations) {
    ASSERT(!EmptyReg::is_pin_allocated(PinId::PD4));
    ASSERT(!EmptyReg::is_signal_allocated(PeripheralId::USART0, SignalType::RX));
}

TEST(empty_registry_no_conflicts) {
    ASSERT(!EmptyReg::has_conflicts());
}

// =============================================================================
// Single Allocation Tests
// =============================================================================

TEST(single_allocation_size) {
    ASSERT(Registry1::size == 1);
}

TEST(single_allocation_pin_allocated) {
    ASSERT(Registry1::is_pin_allocated(PinId::PD4));
    ASSERT(!Registry1::is_pin_allocated(PinId::PA3));
}

TEST(single_allocation_signal_allocated) {
    ASSERT(Registry1::is_signal_allocated(PeripheralId::USART0, SignalType::RX));
    ASSERT(!Registry1::is_signal_allocated(PeripheralId::SPI0, SignalType::MOSI));
}

TEST(single_allocation_no_conflicts) {
    ASSERT(!Registry1::has_conflicts());
}

TEST(single_allocation_count) {
    ASSERT(Registry1::count_pin_allocations(PinId::PD4) == 1);
    ASSERT(Registry1::count_pin_allocations(PinId::PA3) == 0);
}

// =============================================================================
// Multiple Allocations Tests
// =============================================================================

TEST(multiple_allocations_size) {
    ASSERT(Registry2::size == 2);
    ASSERT(Registry3::size == 3);
}

TEST(multiple_allocations_all_pins_tracked) {
    ASSERT(Registry3::is_pin_allocated(PinId::PD4));  // USART0 RX
    ASSERT(Registry3::is_pin_allocated(PinId::PA3));  // TWI0 SDA
    ASSERT(Registry3::is_pin_allocated(PinId::PA4));  // TWI0 SCL
    ASSERT(!Registry3::is_pin_allocated(PinId::PA0));
}

TEST(multiple_allocations_all_signals_tracked) {
    ASSERT(Registry3::is_signal_allocated(PeripheralId::USART0, SignalType::RX));
    ASSERT(Registry3::is_signal_allocated(PeripheralId::TWI0, SignalType::SDA));
    ASSERT(Registry3::is_signal_allocated(PeripheralId::TWI0, SignalType::SCL));
    ASSERT(!Registry3::is_signal_allocated(PeripheralId::SPI0, SignalType::MOSI));
}

TEST(multiple_allocations_no_conflicts) {
    ASSERT(!Registry2::has_conflicts());
    ASSERT(!Registry3::has_conflicts());
}

// =============================================================================
// Conflict Detection Tests
// =============================================================================

TEST(conflict_registry_detects_conflict) {
    // ConflictRegistry has PD4 allocated twice
    ASSERT(ConflictRegistry::has_conflicts());
}

TEST(conflict_registry_pin_count) {
    // PD4 should be allocated twice
    ASSERT(ConflictRegistry::count_pin_allocations(PinId::PD4) == 2);
}

TEST(conflict_registry_identifies_conflicting_pin) {
    // Should identify PD4 as conflicting
    ASSERT(ConflictRegistry::get_first_conflict() == PinId::PD4);
}

TEST(would_conflict_detection) {
    // Adding PD4 to Registry1 would conflict (PD4 already used)
    ASSERT(would_conflict<Registry1>(PinId::PD4) == true);

    // Adding PA5 to Registry1 would NOT conflict (PA5 not used)
    ASSERT(would_conflict<Registry1>(PinId::PA5) == false);
}

// =============================================================================
// Get Allocation Tests
// =============================================================================

TEST(get_allocation_existing) {
    auto alloc = Registry1::get_allocation(PinId::PD4);
    ASSERT(alloc.pin == PinId::PD4);
    ASSERT(alloc.peripheral == PeripheralId::USART0);
    ASSERT(alloc.signal_type == SignalType::RX);
}

TEST(get_allocation_non_existing) {
    auto alloc = Registry1::get_allocation(PinId::PA0);
    // Should return empty allocation (default constructed)
    ASSERT(alloc.pin == PinId::PA0);  // Default value
}

// =============================================================================
// Query Methods Tests
// =============================================================================

TEST(is_pin_allocated_various_pins) {
    // Test multiple pins in Registry3
    ASSERT(Registry3::is_pin_allocated(PinId::PD4) == true);
    ASSERT(Registry3::is_pin_allocated(PinId::PA3) == true);
    ASSERT(Registry3::is_pin_allocated(PinId::PA4) == true);
    ASSERT(Registry3::is_pin_allocated(PinId::PA0) == false);
    ASSERT(Registry3::is_pin_allocated(PinId::PB0) == false);
}

TEST(is_signal_allocated_various_signals) {
    ASSERT(Registry3::is_signal_allocated(PeripheralId::USART0, SignalType::RX) == true);
    ASSERT(Registry3::is_signal_allocated(PeripheralId::USART0, SignalType::TX) == false);
    ASSERT(Registry3::is_signal_allocated(PeripheralId::TWI0, SignalType::SDA) == true);
    ASSERT(Registry3::is_signal_allocated(PeripheralId::TWI0, SignalType::SCL) == true);
    ASSERT(Registry3::is_signal_allocated(PeripheralId::SPI0, SignalType::MOSI) == false);
}

// =============================================================================
// Constexpr Evaluation Tests
// =============================================================================

TEST(registry_operations_are_constexpr) {
    // All registry operations should be compile-time evaluatable
    constexpr bool is_alloc = Registry1::is_pin_allocated(PinId::PD4);
    constexpr usize count = Registry1::count_pin_allocations(PinId::PD4);
    constexpr bool has_conf = Registry1::has_conflicts();

    ASSERT(is_alloc == true);
    ASSERT(count == 1);
    ASSERT(has_conf == false);
}

TEST(allocation_creation_is_constexpr) {
    // PinAllocation creation should be constexpr
    constexpr auto alloc = ALLOC_PIN(PinId::PA5, PeripheralId::USART1,
                                      SignalType::TX);
    ASSERT(alloc.pin == PinId::PA5);
    ASSERT(alloc.peripheral == PeripheralId::USART1);
}

// =============================================================================
// Static Assertions (Compile-Time Validation)
// =============================================================================

// Verify empty registry works
static_assert(EmptyReg::size == 0);
static_assert(!EmptyReg::has_conflicts());

// Verify single allocation works
static_assert(Registry1::size == 1);
static_assert(Registry1::is_pin_allocated(PinId::PD4));
static_assert(!Registry1::has_conflicts());

// Verify multiple allocations work
static_assert(Registry3::size == 3);
static_assert(!Registry3::has_conflicts());

// Verify conflict detection works
static_assert(ConflictRegistry::has_conflicts());
static_assert(ConflictRegistry::count_pin_allocations(PinId::PD4) == 2);

TEST(compile_time_validation_works) {
    // If we reach here, all static_asserts passed
    ASSERT(true);
}

// =============================================================================
// Error Message Tests
// =============================================================================

TEST(conflict_message_available) {
    const char* msg = get_conflict_message<ConflictRegistry>(PinId::PD4);
    ASSERT(msg != nullptr);
    ASSERT(std::string(msg).length() > 0);
}

TEST(conflict_suggestion_available) {
    const char* suggestion = get_conflict_suggestion();
    ASSERT(suggestion != nullptr);
    ASSERT(std::string(suggestion).length() > 0);
}

// =============================================================================
// Edge Cases Tests
// =============================================================================

TEST(same_peripheral_different_signals) {
    // TWI0 has both SDA and SCL allocated - should not conflict
    // (different signals on same peripheral)
    ASSERT(Registry3::is_signal_allocated(PeripheralId::TWI0, SignalType::SDA));
    ASSERT(Registry3::is_signal_allocated(PeripheralId::TWI0, SignalType::SCL));
    ASSERT(!Registry3::has_conflicts());
}

TEST(count_zero_for_unallocated) {
    ASSERT(Registry3::count_pin_allocations(PinId::PB5) == 0);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Signal Registry Tests (Phase 3.3)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Empty registry tests
    run_test_empty_registry_size_zero();
    run_test_empty_registry_no_allocations();
    run_test_empty_registry_no_conflicts();

    // Single allocation tests
    run_test_single_allocation_size();
    run_test_single_allocation_pin_allocated();
    run_test_single_allocation_signal_allocated();
    run_test_single_allocation_no_conflicts();
    run_test_single_allocation_count();

    // Multiple allocations tests
    run_test_multiple_allocations_size();
    run_test_multiple_allocations_all_pins_tracked();
    run_test_multiple_allocations_all_signals_tracked();
    run_test_multiple_allocations_no_conflicts();

    // Conflict detection tests
    run_test_conflict_registry_detects_conflict();
    run_test_conflict_registry_pin_count();
    run_test_conflict_registry_identifies_conflicting_pin();
    run_test_would_conflict_detection();

    // Get allocation tests
    run_test_get_allocation_existing();
    run_test_get_allocation_non_existing();

    // Query methods tests
    run_test_is_pin_allocated_various_pins();
    run_test_is_signal_allocated_various_signals();

    // Constexpr tests
    run_test_registry_operations_are_constexpr();
    run_test_allocation_creation_is_constexpr();
    run_test_compile_time_validation_works();

    // Error message tests
    run_test_conflict_message_available();
    run_test_conflict_suggestion_available();

    // Edge cases
    run_test_same_peripheral_different_signals();
    run_test_count_zero_for_unallocated();

    // Print summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total:  " << tests_run << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << (tests_run - tests_passed) << std::endl;
    std::cout << "========================================" << std::endl;

    return (tests_run == tests_passed) ? 0 : 1;
}
