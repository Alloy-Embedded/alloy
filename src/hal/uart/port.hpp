/// @file hal/uart/port.hpp
/// Connector-typed UART port — the public user-facing type.
///
/// ``uart::port<Connector>`` wraps ``uart::port_handle<Connector>`` and adds:
///
///   * A static ``connect()`` method that applies GPIO AF / route operations
///     *before* the UART registers are configured.  Decoupled from ``configure()``
///     so callers can connect all peripherals in one phase and configure later.
///
///   * Guard C static_asserts that fire at the call site if the connector has no
///     tx<> or rx<> binding, or has duplicate bindings of the same role.
///
/// Usage::
///
///   #include "hal/uart/port.hpp"
///   #include "hal/rcc.hpp"
///
///   namespace dev = alloy::device;
///   using Uart1 = alloy::hal::uart::port<
///       dev::connection::connector<dev::PeripheralId::kUsart1,
///           dev::connection::tx<dev::PinId::kPb6>,
///           dev::connection::rx<dev::PinId::kPb7>>>;
///
///   Uart1::connect();     // configure GPIO AF
///   rcc::peripheral_on<Uart1::peripheral_id>();
///   auto p = Uart1{};
///   p.configure();        // set baud rate, parity, etc.
///   p.write("hello\n");
#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>

#include "core/result.hpp"
#include "core/error_code.hpp"
#include "hal/connect/connector.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/uart/uart.hpp"

namespace alloy::hal::uart {

// ---------------------------------------------------------------------------
// Guard C helpers — introspect Connector binding roles
// ---------------------------------------------------------------------------

namespace detail {

/// Count the number of bindings in Connector whose ``role_id`` equals ``Role``.
template <typename Connector, connection::detail::role_id Role>
[[nodiscard]] consteval auto count_bindings_with_role() noexcept -> std::size_t {
    using Tuple = typename Connector::binding_tuple;
    return []<std::size_t... I>(std::index_sequence<I...>) -> std::size_t {
        return ((std::tuple_element_t<I, Tuple>::role_id == Role ? 1u : 0u) + ...);
    }(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <typename Connector>
inline constexpr bool has_tx_or_rx_v =
    (count_bindings_with_role<Connector, connection::detail::role_id::tx>() > 0u) ||
    (count_bindings_with_role<Connector, connection::detail::role_id::rx>() > 0u);

template <typename Connector>
inline constexpr bool has_duplicate_role_v =
    (count_bindings_with_role<Connector, connection::detail::role_id::tx>() > 1u) ||
    (count_bindings_with_role<Connector, connection::detail::role_id::rx>() > 1u);

}  // namespace detail

// ---------------------------------------------------------------------------
// uart::port<Connector>
// ---------------------------------------------------------------------------

/// Connector-typed UART port.
///
/// Inherits the full ``port_handle<Connector>`` register surface (Phase 1–3
/// methods: configure, write, read, set_baudrate, enable_de, enable_fifo,
/// set_half_duplex, enable_lin, enable_hardware_flow_control, …).
///
/// Adds:
///   * ``static connect()`` — applies GPIO alternate-function configuration.
///   * Guard C static_asserts — compile error when Connector carries no
///     tx<>/rx<> binding or has duplicate roles.
template <typename Connector>
class port : public port_handle<Connector> {
   public:
    // --- Guard C -----------------------------------------------------------
    static_assert(
        detail::has_tx_or_rx_v<Connector>,
        "uart::port<C> requires at least one tx<> or rx<> binding. "
        "Example: uart::port<connector<PeripheralId::kUsart1, tx<PinId::kPb6>>>");

    static_assert(
        !detail::has_duplicate_role_v<Connector>,
        "uart::port<C>: duplicate tx<> or rx<> binding roles. "
        "Each signal role (tx, rx) may appear at most once in the connector.");

    // --- Inherited constructors -------------------------------------------
    using port_handle<Connector>::port_handle;

    // --- Static connect() -------------------------------------------------

    /// Apply GPIO alternate-function / route operations for all bindings in
    /// the connector.  Call before ``configure()`` and before enabling the
    /// peripheral clock.
    ///
    /// Idempotent — safe to call multiple times; the GPIO AF registers are
    /// written unconditionally so calling ``connect()`` again after a deep-
    /// sleep GPIO reset restores the routing.
    ///
    /// Returns ``InvalidParameter`` if the connector has no valid route (this
    /// happens when the target device has not yet been synthesised by
    /// alloy-codegen, or the connector template arguments are wrong).
    [[nodiscard]] static auto connect() noexcept -> core::Result<void, core::ErrorCode> {
        if constexpr (!Connector::valid) {
            return core::Err(core::ErrorCode::InvalidParameter);
        } else {
            return alloy::hal::detail::runtime::apply_route_operations(Connector::operations());
        }
    }

    // --- configure() override: connect first, then UART registers ----------

    /// Connect GPIO pins (via ``connect()``) then configure the UART
    /// peripheral (baud rate, parity, stop bits, etc.).
    ///
    /// Equivalent to calling ``connect()`` then ``port_handle::configure()``,
    /// with early-return on ``connect()`` failure.
    [[nodiscard]] auto configure() const noexcept -> core::Result<void, core::ErrorCode> {
        if (auto r = connect(); r.is_err()) {
            return r;
        }
        return port_handle<Connector>::configure();
    }
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

/// Construct a ``uart::port<Connector>`` with the given config.
///
/// Asserts at compile time that the connector is valid (same as
/// ``uart::open``).  Prefer this factory over direct construction when the
/// config is known at the call site.
template <typename Connector>
[[nodiscard]] constexpr auto open(Config config = {}) -> port<Connector> {
    static_assert(
        port_handle<Connector>::valid,
        "uart::port::open<C>: no valid route for this connector. "
        "Verify the PeripheralId, pin ids, and signal bindings against the "
        "device connector table generated by alloy-codegen.");
    return port<Connector>{config};
}

}  // namespace alloy::hal::uart
