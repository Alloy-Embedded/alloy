// examples/modbus_tcp_slave/main.cpp
//
// SAME70 Xplained Ultra — Modbus TCP slave on port 502.
//
// Exposes the same 10-variable registry as modbus_slave_basic but over TCP
// instead of RS-485 RTU. The TcpStream satisfies alloy::modbus::ByteStream so
// Slave<TcpStream> is a direct drop-in for Slave<UartStream>.
//
// Variable map:
//   0x0000 uint16  kStatus      RW   device status flags
//   0x0001 int16   kTempRaw     RO   raw ADC temperature
//   0x0002 uint16  kSetpoint    RW   target setpoint
//   0x0003 int16   kTrimOffset  RW   trim offset
//   0x0004–0x0005 float kPressure RW  pressure (2 regs, word order ABCD)
//   0x0006–0x0007 float kVoltage  RO  supply voltage
//   0x0008–0x0009 int32 kPosition RW  encoder position
//   0x000A–0x000B uint32 kUptime  RO  uptime seconds
//   0x000C bool kEnable          RW   output enable coil
//   0x000D uint16 kErrors        RW   error code register
//
// Override slave ID: -DSLAVE_ID=0x02

#include <cstdint>

#include BOARD_HEADER
#include BOARD_UART_HEADER

#include "alloy/modbus/registry.hpp"
#include "alloy/modbus/slave.hpp"
#include "alloy/modbus/var.hpp"
#include "boards/same70_xplained/board_ethernet.hpp"
#include "device/runtime.hpp"
#include "drivers/net/lwip/lwip_adapter.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

#ifndef SLAVE_ID
    #define SLAVE_ID 0x01u
#endif

namespace {

using namespace alloy::modbus;
using namespace alloy::examples::uart_console;

// ---- Variable descriptors -----------------------------------------------

constexpr auto kStatus     = Var<std::uint16_t>{0x0000u, Access::ReadWrite};
constexpr auto kTempRaw    = Var<std::int16_t> {0x0001u, Access::ReadOnly};
constexpr auto kSetpoint   = Var<std::uint16_t>{0x0002u, Access::ReadWrite};
constexpr auto kTrimOffset = Var<std::int16_t> {0x0003u, Access::ReadWrite};
constexpr auto kPressure   = Var<float>         {0x0004u, Access::ReadWrite};
constexpr auto kVoltage    = Var<float>         {0x0006u, Access::ReadOnly};
constexpr auto kPosition   = Var<std::int32_t>  {0x0008u, Access::ReadWrite};
constexpr auto kUptime     = Var<std::uint32_t> {0x000Au, Access::ReadOnly};
constexpr auto kEnable     = Var<bool>          {0x000Cu, Access::ReadWrite};
constexpr auto kErrors     = Var<std::uint16_t> {0x000Du, Access::ReadWrite};

// ---- Storage -----------------------------------------------

std::uint16_t g_status      = 0x0001u;
std::int16_t  g_temp_raw    = 250;        // 25.0°C raw
std::uint16_t g_setpoint    = 500u;
std::int16_t  g_trim_offset = 0;
float         g_pressure    = 101.3f;
float         g_voltage     = 3.300f;
std::int32_t  g_position    = 0;
std::uint32_t g_uptime      = 0u;
bool          g_enable      = false;
std::uint16_t g_errors      = 0u;

auto g_registry = make_registry(
    kStatus.bind(g_status),
    kTempRaw.bind(g_temp_raw),
    kSetpoint.bind(g_setpoint),
    kTrimOffset.bind(g_trim_offset),
    kPressure.bind(g_pressure),
    kVoltage.bind(g_voltage),
    kPosition.bind(g_position),
    kUptime.bind(g_uptime),
    kEnable.bind(g_enable),
    kErrors.bind(g_errors)
);

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    (void)uart.configure();
    write_line(uart, "modbus_tcp_slave: ready (slave 0x" \
               + std::to_string(SLAVE_ID) + ", port 502)");

    // ---- Ethernet bring-up ----
    board::enable_ethernet_clocks();
    board::mux_ethernet_pins();
    board::release_phy_reset();

    auto mdio     = board::make_mdio();
    mdio.enable_management();

    auto mac_bytes = board::mac_from_eui48();
    auto gmac      = board::make_gmac(std::span<const std::uint8_t, 6u>{mac_bytes});
    auto phy       = board::make_phy(mdio);

    if (phy.init().is_err()) {
        write_line(uart, "modbus_tcp_slave: FAIL — PHY init");
        while (true) {}
    }

    gmac.init();

    auto eth     = board::make_ethernet_interface(gmac, phy);
    alloy::net::LwipAdapter<board::BoardEth> adapter{eth};
    adapter.init();

    // Wait for DHCP.
    write_line(uart, "modbus_tcp_slave: waiting for DHCP ...");
    while (!adapter.dhcp_bound()) {
        eth.poll_link();
        adapter.poll();
    }
    write_text(uart, "modbus_tcp_slave: IP ");
    const auto ip = adapter.ip_address();
    // Quick print without helpers for brevity.
    write_line(uart, " assigned — listening on port 502");

    // ---- TCP listener ----
    auto listener_r = adapter.listen(502u);
    if (listener_r.is_err()) {
        write_line(uart, "modbus_tcp_slave: FAIL — listen()");
        while (true) {}
    }
    auto& listener = listener_r.unwrap();
    write_line(uart, "modbus_tcp_slave: ready — waiting for connections");

    // ---- Cooperative main loop ----
    while (true) {
        eth.poll_link();
        adapter.poll();

        // Accept a connection (non-blocking: 1 ms timeout).
        auto conn_r = listener.accept(1'000u);
        if (conn_r.is_ok()) {
            auto& stream = conn_r.unwrap();
            write_line(uart, "modbus_tcp_slave: client connected");

            // Create a slave for this connection; poll until the client disconnects.
            Slave slave{stream, static_cast<std::uint8_t>(SLAVE_ID), g_registry};
            while (stream.connected()) {
                (void)slave.poll(5'000u);
                eth.poll_link();
                adapter.poll();
                g_uptime++;
            }
            write_line(uart, "modbus_tcp_slave: client disconnected");
        }
    }
}
