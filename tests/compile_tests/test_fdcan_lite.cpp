/// @file tests/compile_tests/test_fdcan_lite.cpp
/// Compile-time verification of the FDCAN lite driver (task 2.1.3).
///
/// Uses a self-contained mock PeripheralSpec — no real device artifact needed.
/// Verifies:
///   - StFdcan concept gates on kTemplate == "fdcan"
///   - controller<P, MsgRamBase> instantiates without error
///   - write_tx / read_rx return types are correct
///   - irq_number() / irq_count() work as per the device-data bridge

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "hal/fdcan/lite.hpp"

// ============================================================================
// Mock peripheral specs
// ============================================================================

namespace mock {

/// Simulates STM32G4 FDCAN1 flat-struct entry (kTemplate == "fdcan").
struct fdcan1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40006400u;
    static constexpr const char*    kName        = "fdcan1";
    static constexpr const char*    kTemplate    = "fdcan";
    static constexpr const char*    kIpVersion   = "m_can_3v3";
    static constexpr unsigned       kIrqLines[]  = { 21u, 22u };  // intr0, intr1
    static constexpr unsigned       kIrqCount    = 2u;
};

/// A non-FDCAN peripheral — must NOT satisfy StFdcan.
struct usart1 {
    static constexpr std::uintptr_t kBaseAddress = 0x40013800u;
    static constexpr const char*    kName        = "usart1";
    static constexpr const char*    kTemplate    = "usart";
    static constexpr const char*    kIpVersion   = "sci3_v1_3";
};

}  // namespace mock

// ============================================================================
// Concept checks
// ============================================================================

namespace {

static_assert( alloy::hal::fdcan::lite::StFdcan<mock::fdcan1>,
    "mock::fdcan1 must satisfy StFdcan");

static_assert(!alloy::hal::fdcan::lite::StFdcan<mock::usart1>,
    "mock::usart1 must NOT satisfy StFdcan (wrong kTemplate)");

// ============================================================================
// Instantiation and method return-type checks
// ============================================================================

// MsgRamBase = 0x4000AC00 (STM32G4 FDCAN1 message RAM)
using Fdcan1 = alloy::hal::fdcan::lite::controller<mock::fdcan1, 0x4000AC00u>;

// write_tx returns bool
static_assert(std::is_same_v<bool, decltype(
    Fdcan1::write_tx(0u, false, static_cast<const std::uint8_t*>(nullptr), 0u))>);

// tx_pending / rx_available return bool
static_assert(std::is_same_v<bool, decltype(Fdcan1::tx_pending())>);
static_assert(std::is_same_v<bool, decltype(Fdcan1::rx_available())>);

// irq_number returns uint32_t, matches kIrqLines
static_assert(std::is_same_v<std::uint32_t, decltype(Fdcan1::irq_number())>);
static_assert(Fdcan1::irq_number()    == 21u);
static_assert(Fdcan1::irq_number(1u) == 22u);

// irq_count returns size_t, matches kIrqCount
static_assert(std::is_same_v<std::size_t, decltype(Fdcan1::irq_count())>);
static_assert(Fdcan1::irq_count() == 2u);

// noexcept
static_assert(noexcept(Fdcan1::tx_pending()));
static_assert(noexcept(Fdcan1::rx_available()));
static_assert(noexcept(Fdcan1::irq_number()));
static_assert(noexcept(Fdcan1::irq_count()));

}  // namespace
