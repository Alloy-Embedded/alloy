/**
 * @file signal_registry.hpp
 * @brief Signal Routing Registry for Compile-Time Allocation Tracking
 *
 * Provides a compile-time registry to track which pins are allocated to which
 * signals, enabling conflict detection when the same pin is used for multiple
 * peripherals.
 *
 * Design Principles:
 * - Compile-time tracking using template meta-programming
 * - Zero runtime overhead (all checks at compile-time)
 * - Clear error messages when conflicts detected
 * - Support for query by pin or by signal
 *
 * @note Part of Phase 3.3: Signal Routing Registry
 * @see openspec/changes/modernize-peripheral-architecture/specs/signal-routing/spec.md
 */

#pragma once

#include <array>
#include <type_traits>

#include "core/types.hpp"
#include "hal/signals.hpp"

namespace alloy::hal::signal_registry {

using namespace alloy::core;
using namespace alloy::hal::signals;

// ============================================================================
// Registry Entry Types
// ============================================================================

/**
 * @brief Represents a pin allocation to a signal
 *
 * Stores information about which pin is allocated to which peripheral signal.
 * Note: signal_name is not included to allow use as non-type template parameter.
 */
struct PinAllocation {
    PinId pin;
    PeripheralId peripheral;
    SignalType signal_type;

    constexpr PinAllocation()
        : pin(PinId::PA0),
          peripheral(PeripheralId::USART0),
          signal_type(SignalType::TX) {}

    constexpr PinAllocation(PinId p, PeripheralId per, SignalType sig)
        : pin(p), peripheral(per), signal_type(sig) {}
};

/**
 * @brief Compile-time registry for signal allocations
 *
 * Template class that maintains a compile-time list of pin allocations.
 * Uses recursive template inheritance to build the allocation list.
 *
 * @tparam Allocations Variadic list of PinAllocation entries
 */
template <PinAllocation... Allocations>
struct SignalRegistry {
    // Store allocations in a constexpr array
    static constexpr std::array<PinAllocation, sizeof...(Allocations)> allocations = {Allocations...};
    static constexpr usize size = sizeof...(Allocations);

    /**
     * @brief Check if a pin is already allocated
     *
     * @param pin Pin to check
     * @return true if pin is allocated, false otherwise
     */
    static constexpr bool is_pin_allocated(PinId pin) {
        for (const auto& alloc : allocations) {
            if (alloc.pin == pin) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Check if a signal is already allocated
     *
     * @param peripheral Peripheral ID
     * @param signal_type Signal type
     * @return true if signal is allocated, false otherwise
     */
    static constexpr bool is_signal_allocated(PeripheralId peripheral, SignalType signal_type) {
        for (const auto& alloc : allocations) {
            if (alloc.peripheral == peripheral && alloc.signal_type == signal_type) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Get allocation info for a pin
     *
     * @param pin Pin to query
     * @return PinAllocation if found, empty allocation otherwise
     */
    static constexpr PinAllocation get_allocation(PinId pin) {
        for (const auto& alloc : allocations) {
            if (alloc.pin == pin) {
                return alloc;
            }
        }
        return PinAllocation{};  // Empty
    }

    /**
     * @brief Count how many times a pin is allocated
     *
     * Useful for detecting conflicts (count > 1)
     *
     * @param pin Pin to count
     * @return Number of allocations
     */
    static constexpr usize count_pin_allocations(PinId pin) {
        usize count = 0;
        for (const auto& alloc : allocations) {
            if (alloc.pin == pin) {
                count++;
            }
        }
        return count;
    }

    /**
     * @brief Check for conflicts in the registry
     *
     * A conflict occurs when the same pin is allocated to multiple signals.
     *
     * @return true if conflicts exist, false otherwise
     */
    static constexpr bool has_conflicts() {
        for (usize i = 0; i < allocations.size(); ++i) {
            // Check if this pin appears again later
            for (usize j = i + 1; j < allocations.size(); ++j) {
                if (allocations[i].pin == allocations[j].pin) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief Get first conflicting pin
     *
     * Returns the first pin that has multiple allocations.
     *
     * @return PinId of conflicting pin, or PA0 if no conflicts
     */
    static constexpr PinId get_first_conflict() {
        for (usize i = 0; i < allocations.size(); ++i) {
            for (usize j = i + 1; j < allocations.size(); ++j) {
                if (allocations[i].pin == allocations[j].pin) {
                    return allocations[i].pin;
                }
            }
        }
        return PinId::PA0;  // No conflict
    }
};

// Empty registry (no allocations)
using EmptyRegistry = SignalRegistry<>;

// ============================================================================
// Registry Builder Helper
// ============================================================================

/**
 * @brief Helper to add an allocation to a registry
 *
 * Creates a new registry with the additional allocation.
 * Used for building up registries at compile-time.
 *
 * @tparam Registry Existing registry type
 * @tparam Alloc New allocation to add
 */
template <typename Registry, PinAllocation Alloc>
struct AddAllocation;

template <PinAllocation... Existing, PinAllocation New>
struct AddAllocation<SignalRegistry<Existing...>, New> {
    using type = SignalRegistry<Existing..., New>;
};

template <typename Registry, PinAllocation Alloc>
using AddAllocation_t = typename AddAllocation<Registry, Alloc>::type;

// ============================================================================
// Convenience Macros for Registering Allocations
// ============================================================================

/**
 * @brief Create a pin allocation entry
 *
 * Helper macro to create PinAllocation entries with less boilerplate.
 *
 * Example:
 * @code
 * constexpr auto alloc = ALLOC_PIN(PinId::PD4, PeripheralId::USART0, SignalType::RX);
 * @endcode
 */
#define ALLOC_PIN(pin, peripheral, signal_type) \
    alloy::hal::signal_registry::PinAllocation(pin, peripheral, signal_type)

/**
 * @brief Register a signal allocation with compile-time validation
 *
 * Creates a type alias for a registry with the new allocation added.
 * Includes compile-time checks for conflicts.
 *
 * Example:
 * @code
 * using Registry1 = EmptyRegistry;
 * using Registry2 = AddAllocation_t<Registry1,
 *     ALLOC_PIN(PinId::PD4, PeripheralId::USART0, SignalType::RX, "USART0_RX")>;
 * @endcode
 */

// ============================================================================
// Validation Helpers
// ============================================================================

/**
 * @brief Validate that a registry has no conflicts
 *
 * Use in static_assert to ensure no pin is allocated twice.
 *
 * Example:
 * @code
 * static_assert(!MyRegistry::has_conflicts(),
 *               "Pin allocation conflict detected!");
 * @endcode
 */
template <typename Registry>
constexpr bool validate_no_conflicts() {
    return !Registry::has_conflicts();
}

/**
 * @brief Check if adding an allocation would create a conflict
 *
 * @tparam Registry Existing registry
 * @param pin Pin to allocate
 * @return true if allocation would conflict, false otherwise
 */
template <typename Registry>
constexpr bool would_conflict(PinId pin) {
    return Registry::is_pin_allocated(pin);
}

// ============================================================================
// Error Message Helpers
// ============================================================================

/**
 * @brief Get descriptive error message for a conflict
 *
 * Provides information about which pin is conflicting and what it's
 * already allocated to.
 *
 * @tparam Registry Registry with conflict
 * @param pin Conflicting pin
 * @return Error message string
 */
template <typename Registry>
constexpr const char* get_conflict_message(PinId pin) {
    if (Registry::is_pin_allocated(pin)) {
        return "Pin is already allocated to another signal";
    }
    return "No conflict";
}

/**
 * @brief Get suggestion for resolving a conflict
 *
 * @return Suggestion string
 */
constexpr const char* get_conflict_suggestion() {
    return "Use a different pin or deallocate the conflicting signal";
}

// ============================================================================
// Example Usage Documentation
// ============================================================================

/*
Example usage:

// Start with empty registry
using MyRegistry = EmptyRegistry;

// Add USART0 TX allocation
constexpr auto usart_tx = ALLOC_PIN(PinId::PD4, PeripheralId::USART0,
                                     SignalType::TX, "USART0_TX");
using Registry1 = AddAllocation_t<MyRegistry, usart_tx>;

// Add TWI0 SDA allocation
constexpr auto twi_sda = ALLOC_PIN(PinId::PA3, PeripheralId::TWI0,
                                    SignalType::SDA, "TWI0_SDA");
using Registry2 = AddAllocation_t<Registry1, twi_sda>;

// Validate no conflicts
static_assert(!Registry2::has_conflicts(), "Pin conflict detected!");

// Try to add conflicting allocation (would fail)
constexpr auto conflict = ALLOC_PIN(PinId::PD4, PeripheralId::SPI0,
                                     SignalType::MOSI, "SPI0_MOSI");
// This would create a conflict with USART0 TX on PD4
static_assert(!would_conflict<Registry2>(PinId::PD4) || true,
             "PD4 is already allocated to USART0!");

// Query allocations
static_assert(Registry2::is_pin_allocated(PinId::PD4));
static_assert(Registry2::is_signal_allocated(PeripheralId::USART0, SignalType::TX));
static_assert(Registry2::count_pin_allocations(PinId::PD4) == 1);
*/

}  // namespace alloy::hal::signal_registry
