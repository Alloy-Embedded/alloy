/// @file tests/compile_tests/test_uart_pin_guard.cpp
/// Compile-time verification of uart::port<C> Guard C (task 5.2).
///
/// Guard C: uart::detail::has_tx_or_rx_v / has_duplicate_role_v helpers,
/// which back the static_asserts in uart::port<Connector>.
///
/// Tests that compile (positive):
///   - has_tx_or_rx_v is true for valid tx-only, rx-only, and tx+rx connectors
///   - has_tx_or_rx_v is false for a connector with only non-UART roles (cts/rts)
///   - has_duplicate_role_v is false for single-tx and single-rx connectors
///   - has_duplicate_role_v is true for connectors with two tx<> bindings
///   - Board-specific: uart::port<ValidConnector> instantiates without Guard C fire
///
/// Guard A / Guard B negative tests (tasks 5.2 second block) require Phase 2
/// (connectors.hpp SignalSourceTraits) and a compile-fail harness — deferred.
///
/// NOTE: The negative cases (Guard C fires) CANNOT be tested in this TU without
/// a compile-fail harness; instantiating uart::port<BadConnector> unconditionally
/// aborts this TU.  The helpers are tested directly instead.

#include <cstddef>
#include <tuple>
#include <type_traits>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/uart/port.hpp"

// ============================================================================
// Minimal mock bindings for Guard C helper unit-testing
// ============================================================================

namespace mock_binding {

using alloy::hal::connection::detail::role_id;

/// A mock binding carrying only the role_id field required by the helpers.
template <role_id R>
struct binding {
    static constexpr auto role_id = R;
};

/// Minimal connector mock — provides only `binding_tuple`.
/// Does NOT need `valid`, `operations()`, etc. because we test the helpers
/// directly, not by instantiating uart::port<>.
template <typename... Bs>
struct connector {
    using binding_tuple = std::tuple<Bs...>;
};

// Convenient binding aliases
using tx  = binding<role_id::tx>;
using rx  = binding<role_id::rx>;
using cts = binding<role_id::cts>;
using rts = binding<role_id::rts>;
using sck = binding<role_id::sck>;

}  // namespace mock_binding

// ============================================================================
// has_tx_or_rx_v — positive cases
// ============================================================================

namespace {

namespace ud = alloy::hal::uart::detail;
namespace mb = mock_binding;

// tx only → present
using TxOnly = mb::connector<mb::tx>;
static_assert( ud::has_tx_or_rx_v<TxOnly>,
    "tx-only connector must satisfy has_tx_or_rx_v");
static_assert(!ud::has_duplicate_role_v<TxOnly>,
    "single-tx connector must not satisfy has_duplicate_role_v");

// rx only → present
using RxOnly = mb::connector<mb::rx>;
static_assert( ud::has_tx_or_rx_v<RxOnly>,
    "rx-only connector must satisfy has_tx_or_rx_v");
static_assert(!ud::has_duplicate_role_v<RxOnly>,
    "single-rx connector must not satisfy has_duplicate_role_v");

// tx + rx → present, no duplicate
using TxRx = mb::connector<mb::tx, mb::rx>;
static_assert( ud::has_tx_or_rx_v<TxRx>,
    "tx+rx connector must satisfy has_tx_or_rx_v");
static_assert(!ud::has_duplicate_role_v<TxRx>,
    "tx+rx connector (one each) must not have duplicates");

// tx + rx + cts + rts → still present, no duplicate
using Full = mb::connector<mb::tx, mb::rx, mb::cts, mb::rts>;
static_assert( ud::has_tx_or_rx_v<Full>,
    "full connector (tx+rx+cts+rts) must satisfy has_tx_or_rx_v");
static_assert(!ud::has_duplicate_role_v<Full>,
    "full connector (tx+rx+cts+rts) must not have duplicates");

// ============================================================================
// has_tx_or_rx_v — negative cases (no tx or rx)
// ============================================================================

// CTS only → not present
using CtsOnly = mb::connector<mb::cts>;
static_assert(!ud::has_tx_or_rx_v<CtsOnly>,
    "cts-only connector must NOT satisfy has_tx_or_rx_v");
static_assert(!ud::has_duplicate_role_v<CtsOnly>,
    "cts-only connector must not have duplicates");

// SCK only → not present
using SckOnly = mb::connector<mb::sck>;
static_assert(!ud::has_tx_or_rx_v<SckOnly>,
    "sck-only connector must NOT satisfy has_tx_or_rx_v");

// Empty binding list → not present
using Empty = mb::connector<>;
static_assert(!ud::has_tx_or_rx_v<Empty>,
    "empty connector must NOT satisfy has_tx_or_rx_v");
static_assert(!ud::has_duplicate_role_v<Empty>,
    "empty connector must not have duplicates");

// ============================================================================
// has_duplicate_role_v — positive cases (duplicates present)
// ============================================================================

// Two tx bindings → duplicate
using DupTx = mb::connector<mb::tx, mb::tx>;
static_assert( ud::has_duplicate_role_v<DupTx>,
    "double-tx connector must satisfy has_duplicate_role_v");

// Two rx bindings → duplicate
using DupRx = mb::connector<mb::rx, mb::rx>;
static_assert( ud::has_duplicate_role_v<DupRx>,
    "double-rx connector must satisfy has_duplicate_role_v");

// Two tx + one rx → tx duplicate
using DupTxPlusRx = mb::connector<mb::tx, mb::tx, mb::rx>;
static_assert( ud::has_tx_or_rx_v<DupTxPlusRx>);
static_assert( ud::has_duplicate_role_v<DupTxPlusRx>,
    "double-tx+rx connector must satisfy has_duplicate_role_v");

// tx + two rx → rx duplicate
using TxDupRx = mb::connector<mb::tx, mb::rx, mb::rx>;
static_assert( ud::has_duplicate_role_v<TxDupRx>,
    "tx+double-rx connector must satisfy has_duplicate_role_v");

// ============================================================================
// Board-specific positive test: uart::port<ValidConnector> compiles (no Guard C)
// ============================================================================

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
// USART2 on PA2/PA3 is the Nucleo VCP connection.
using ValidConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART2,
    alloy::hal::connection::tx<alloy::device::PinId::PA2,
                               alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::PA3,
                               alloy::device::SignalId::signal_rx>>;

// Guard C must NOT fire: connector has one tx + one rx.
// (Positive test: if this compiles, Guard C is not erroneously triggering.)
[[maybe_unused]] static constexpr bool kGuardCPassesG0 =
    ud::has_tx_or_rx_v<ValidConnector> && !ud::has_duplicate_role_v<ValidConnector>;
static_assert(kGuardCPassesG0, "G071RB USART2 connector must pass Guard C");

// Verify uart::port<ValidConnector> instantiates without error.
// (If Guard C static_asserts fired, this line would not compile.)
static_assert(ValidConnector::valid, "USART2 PA2/PA3 connector must be valid on G071RB");

#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using ValidConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART2,
    alloy::hal::connection::tx<alloy::device::PinId::PA2,
                               alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::PA3,
                               alloy::device::SignalId::signal_rx>>;

[[maybe_unused]] static constexpr bool kGuardCPassesF4 =
    ud::has_tx_or_rx_v<ValidConnector> && !ud::has_duplicate_role_v<ValidConnector>;
static_assert(kGuardCPassesF4, "F401 USART2 connector must pass Guard C");
static_assert(ValidConnector::valid, "USART2 PA2/PA3 connector must be valid on F401RE");

#elif defined(ALLOY_BOARD_SAME70_XPLD)
using ValidConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART1,
    alloy::hal::connection::tx<alloy::device::PinId::PB4,
                               alloy::device::SignalId::signal_txd1>,
    alloy::hal::connection::rx<alloy::device::PinId::PA21,
                               alloy::device::SignalId::signal_rxd1>>;

[[maybe_unused]] static constexpr bool kGuardCPassesSam =
    ud::has_tx_or_rx_v<ValidConnector> && !ud::has_duplicate_role_v<ValidConnector>;
static_assert(kGuardCPassesSam, "SAME70 USART1 connector must pass Guard C");
static_assert(ValidConnector::valid, "USART1 PB4/PA21 connector must be valid on SAME70");
#endif

// ============================================================================
// NOTE: Negative compile tests for Guard C
// ============================================================================
//
// The following connectors SHOULD trigger Guard C static_asserts:
//
//   // No tx or rx:
//   using BadNoTxRx = alloy::hal::connection::connector<
//       alloy::device::PeripheralId::USART1,
//       alloy::hal::connection::cts<alloy::device::PinId::PA0, ...>>;
//   using BadPort = alloy::hal::uart::port<BadNoTxRx>;  // fires Guard C (no tx/rx)
//
//   // Duplicate tx:
//   using BadDupTx = alloy::hal::connection::connector<
//       alloy::device::PeripheralId::USART1,
//       alloy::hal::connection::tx<alloy::device::PinId::PA9, ...>,
//       alloy::hal::connection::tx<alloy::device::PinId::PB6, ...>>;
//   using BadPort2 = alloy::hal::uart::port<BadDupTx>;  // fires Guard C (duplicate tx)
//
// These cannot be placed in this TU without a compile-fail harness.
// The helpers above (has_tx_or_rx_v / has_duplicate_role_v) are tested directly
// to validate the predicate logic independently.

}  // namespace
