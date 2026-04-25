// examples/modbus_slave_basic/main.cpp
//
// Modbus RTU slave — 10 mixed-type variables, foundational board.
//
// Slave ID  : 0x01 (override SLAVE_ID via CMake compile definition)
// Baud rate : 115200 (matches board debug UART default)
// Variables (holding registers, FC03/06/10):
//
//   Addr  Type     Access     Name
//   0x00  uint16   ReadWrite  status_word
//   0x01  int16    ReadOnly   temperature_raw     (e.g. ADC counts)
//   0x02  uint16   ReadWrite  setpoint
//   0x03  int16    ReadWrite  trim_offset
//   0x04  float    ReadWrite  pressure_hpa        (2 registers: 0x04-0x05)
//   0x06  float    ReadOnly   voltage_v           (2 registers: 0x06-0x07)
//   0x08  int32    ReadWrite  position_steps      (2 registers: 0x08-0x09)
//   0x0A  uint32   ReadOnly   uptime_s            (2 registers: 0x0A-0x0B)
//   0x0C  bool     ReadWrite  enable_coil         (FC01/05 compatible)
//   0x0D  uint16   ReadWrite  error_flags
//
// Usage: flash, connect RS-485 adapter to the board UART, query with any
// Modbus master (e.g. modpoll, pymodbus, or a PLC).
//
// Note: requires alloy/modbus/transport/uart_stream.hpp (task 4.2).

#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
    #error "modbus_slave_basic requires BOARD_UART_HEADER for the selected board"
#endif
#include BOARD_UART_HEADER

#include "alloy/modbus/registry.hpp"
#include "alloy/modbus/slave.hpp"
#include "alloy/modbus/transport/uart_stream.hpp"
#include "alloy/modbus/var.hpp"

#ifndef SLAVE_ID
#define SLAVE_ID 0x01u
#endif

namespace {

using namespace alloy::modbus;

// ============================================================================
// Variable declarations (compile-time descriptors)
// ============================================================================

constexpr Var<std::uint16_t> kStatus    {.address=0x00u, .access=Access::ReadWrite, .name="status_word"};
constexpr Var<std::int16_t>  kTempRaw   {.address=0x01u, .access=Access::ReadOnly,  .name="temperature_raw"};
constexpr Var<std::uint16_t> kSetpoint  {.address=0x02u, .access=Access::ReadWrite, .name="setpoint"};
constexpr Var<std::int16_t>  kTrimOffset{.address=0x03u, .access=Access::ReadWrite, .name="trim_offset"};
constexpr Var<float>         kPressure  {.address=0x04u, .access=Access::ReadWrite, .name="pressure_hpa"};
constexpr Var<float>         kVoltage   {.address=0x06u, .access=Access::ReadOnly,  .name="voltage_v"};
constexpr Var<std::int32_t>  kPosition  {.address=0x08u, .access=Access::ReadWrite, .name="position_steps"};
constexpr Var<std::uint32_t> kUptime    {.address=0x0Au, .access=Access::ReadOnly,  .name="uptime_s"};
constexpr Var<bool>          kEnable    {.address=0x0Cu, .access=Access::ReadWrite, .name="enable_coil"};
constexpr Var<std::uint16_t> kErrors    {.address=0x0Du, .access=Access::ReadWrite, .name="error_flags"};

// ============================================================================
// Backing storage (updated by application logic)
// ============================================================================

std::uint16_t g_status{0u};
std::int16_t  g_temp_raw{0};
std::uint16_t g_setpoint{500u};
std::int16_t  g_trim{0};
float         g_pressure{1013.25f};
float         g_voltage{3.3f};
std::int32_t  g_position{0};
std::uint32_t g_uptime{0u};
bool          g_enable{false};
std::uint16_t g_errors{0u};

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    if (uart.configure().is_err()) {
        while (true) { board::led::toggle(); }
    }

    // UartStream wraps the HAL UART handle as a ByteStream (requires task 4.2).
    alloy::modbus::UartStream stream{uart};

    // Bind variables to their backing storage and build the registry.
    auto registry = alloy::modbus::Registry<10u>{std::array<alloy::modbus::VarDescriptor, 10u>{
        bind(kStatus,     g_status),
        bind(kTempRaw,    g_temp_raw),
        bind(kSetpoint,   g_setpoint),
        bind(kTrimOffset, g_trim),
        bind(kPressure,   g_pressure),
        bind(kVoltage,    g_voltage),
        bind(kPosition,   g_position),
        bind(kUptime,     g_uptime),
        bind(kEnable,     g_enable),
        bind(kErrors,     g_errors),
    }};

    alloy::modbus::Slave slave{stream, static_cast<std::uint8_t>(SLAVE_ID), registry};

    while (true) {
        // Application logic: update read-only vars from sensors/counters.
        g_temp_raw = board::adc::read_raw();
        g_voltage  = board::adc::read_voltage();
        ++g_uptime;

        // Process one Modbus request (10 ms timeout between polls).
        (void)slave.poll(10'000u);
    }
}
