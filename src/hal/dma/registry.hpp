/**
 * @file dma_registry.hpp
 * @brief Compile-Time DMA Channel Registry
 *
 * Provides compile-time tracking of DMA channel allocations and
 * conflict detection.
 *
 * Design Principles:
 * - Compile-time allocation tracking
 * - Automatic conflict detection
 * - Type-safe channel management
 * - Clear error messages
 * - Zero runtime overhead
 *
 * Example Usage:
 * @code
 * // Start with empty registry
 * using EmptyDmaReg = DmaRegistry<>;
 *
 * // Allocate USART0 TX DMA
 * using Uart0TxDma = DmaConnection<
 *     PeripheralId::USART0,
 *     DmaRequest::USART0_TX,
 *     DmaStream::Stream0
 * >;
 * using Reg1 = AddDmaAllocation_t<EmptyDmaReg, Uart0TxDma>;
 *
 * // Try to allocate same stream (will detect conflict)
 * using ConflictDma = DmaConnection<
 *     PeripheralId::SPI0,
 *     DmaRequest::SPI0_TX,
 *     DmaStream::Stream0  // Conflict!
 * >;
 * using Reg2 = AddDmaAllocation_t<Reg1, ConflictDma>;
 * static_assert(!Reg2::has_conflicts(), "DMA conflict detected");
 * @endcode
 *
 * @note Part of Phase 5.2: DMA Channel Registry
 * @see openspec/changes/modernize-peripheral-architecture/specs/dma-integration/spec.md
 */

#pragma once

#include "core/types.hpp"
#include "hal/dma/connection.hpp"

#include <array>

namespace alloy::hal {

using namespace alloy::core;

// ============================================================================
// DMA Allocation Record
// ============================================================================

/**
 * @brief DMA allocation record
 *
 * Records a single DMA channel allocation with peripheral and request info.
 */
struct DmaAllocation {
    PeripheralId peripheral;
    DmaRequest request;
    DmaStream stream;

    constexpr DmaAllocation(PeripheralId p, DmaRequest r, DmaStream s)
        : peripheral(p), request(r), stream(s) {}
};

// ============================================================================
// DMA Registry
// ============================================================================

/**
 * @brief Compile-time DMA channel registry
 *
 * Tracks all DMA channel allocations and detects conflicts.
 * Built using variadic template parameters.
 *
 * @tparam Allocations Variadic list of DmaAllocation records
 */
template <DmaAllocation... Allocations>
struct DmaRegistry {
    static constexpr std::array<DmaAllocation, sizeof...(Allocations)> allocations = {Allocations...};
    static constexpr usize size = sizeof...(Allocations);

    /**
     * @brief Check if a DMA stream is allocated
     *
     * @param stream The stream to check
     * @return true if allocated, false otherwise
     */
    static constexpr bool is_stream_allocated(DmaStream stream) {
        for (const auto& alloc : allocations) {
            if (alloc.stream == stream) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Check if any conflicts exist
     *
     * A conflict occurs when the same stream is allocated multiple times.
     *
     * @return true if conflicts exist, false otherwise
     */
    static constexpr bool has_conflicts() {
        for (usize i = 0; i < size; ++i) {
            for (usize j = i + 1; j < size; ++j) {
                if (allocations[i].stream == allocations[j].stream) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief Count allocations for a specific stream
     *
     * @param stream The stream to count
     * @return Number of allocations (should be 0 or 1)
     */
    static constexpr usize count_stream_allocations(DmaStream stream) {
        usize count = 0;
        for (const auto& alloc : allocations) {
            if (alloc.stream == stream) {
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief Check if a peripheral has DMA allocated
     *
     * @param peripheral The peripheral to check
     * @return true if peripheral has any DMA allocated
     */
    static constexpr bool has_peripheral_dma(PeripheralId peripheral) {
        for (const auto& alloc : allocations) {
            if (alloc.peripheral == peripheral) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Check if a specific request is allocated
     *
     * @param request The DMA request to check
     * @return true if this request is allocated
     */
    static constexpr bool is_request_allocated(DmaRequest request) {
        for (const auto& alloc : allocations) {
            if (alloc.request == request) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Get stream for a specific request (if allocated)
     *
     * @param request The DMA request
     * @return The allocated stream (or Stream0 if not found)
     */
    static constexpr DmaStream get_stream_for_request(DmaRequest request) {
        for (const auto& alloc : allocations) {
            if (alloc.request == request) {
                return alloc.stream;
            }
        }
        return DmaStream::Stream0; // Not found
    }

    /**
     * @brief Check if registry is valid (no conflicts)
     *
     * @return true if valid, false if conflicts exist
     */
    static constexpr bool is_valid() {
        return !has_conflicts();
    }

    /**
     * @brief Get error message if invalid
     *
     * @return Error message string
     */
    static constexpr const char* error_message() {
        if (has_conflicts()) {
            return "DMA stream conflict detected: same stream allocated multiple times";
        }
        return "Valid DMA registry";
    }
};

// ============================================================================
// Registry Manipulation
// ============================================================================

/**
 * @brief Add allocation to registry
 *
 * Creates a new registry with the additional allocation.
 *
 * @tparam Registry The existing registry type
 * @tparam Connection The DMA connection to add
 */
template <typename Registry, typename Connection>
struct AddDmaAllocation;

template <DmaAllocation... Existing, typename Connection>
struct AddDmaAllocation<DmaRegistry<Existing...>, Connection> {
    static constexpr DmaAllocation new_alloc{
        Connection::peripheral,
        Connection::request,
        Connection::stream
    };

    using type = DmaRegistry<Existing..., new_alloc>;
};

/**
 * @brief Helper alias for adding DMA allocation
 */
template <typename Registry, typename Connection>
using AddDmaAllocation_t = typename AddDmaAllocation<Registry, Connection>::type;

// ============================================================================
// Helper Macros
// ============================================================================

/**
 * @brief Create DMA allocation record
 *
 * Convenience macro for creating allocation records.
 */
#define ALLOC_DMA(periph, req, strm) \
    DmaAllocation{PeripheralId::periph, DmaRequest::req, DmaStream::strm}

/**
 * @brief Create DMA connection and add to registry
 *
 * Convenience macro for the common pattern of creating a connection
 * and adding it to a registry.
 */
#define ADD_DMA_CONNECTION(Registry, Periph, Request, Stream) \
    AddDmaAllocation_t<Registry, \
        DmaConnection<PeripheralId::Periph, DmaRequest::Request, DmaStream::Stream>>

// ============================================================================
// Empty Registry Type
// ============================================================================

/**
 * @brief Empty DMA registry
 *
 * Starting point for building a registry.
 */
using EmptyDmaRegistry = DmaRegistry<>;

}  // namespace alloy::hal
