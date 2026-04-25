// examples/modbus_slave_rich/main.cpp
//
// Modbus RTU slave — 30 variables with full VarMeta + discovery FC 0x65.
//
// A master (or dashboard tool) can issue a discovery probe (FC 0x65 sub-fn
// 0x02) to get the complete variable table — addresses, types, access modes,
// names, units, descriptions, and engineering ranges — without pre-configured
// map files.
//
// Variable groups:
//   Environmental  (0x00–0x07):  temperature, humidity, pressure, altitude
//   Power          (0x08–0x0F):  voltage, current, power, energy
//   Control        (0x10–0x1B):  setpoints, PID coefficients, enable flags
//   Diagnostics    (0x1C–0x27):  uptime, error counts, watchdog, resets
//
// Note: requires alloy/modbus/transport/uart_stream.hpp (task 4.2).

#include <array>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
    #error "modbus_slave_rich requires BOARD_UART_HEADER for the selected board"
#endif
#include BOARD_UART_HEADER

#include "alloy/modbus/discovery.hpp"
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
// Metadata (stored in flash via constexpr)
// ============================================================================

constexpr VarMeta kMetaTemp    {.unit="degC", .desc="ambient temperature",  .range_min=-40.0f, .range_max=125.0f};
constexpr VarMeta kMetaHumid   {.unit="%RH",  .desc="relative humidity",    .range_min=0.0f,   .range_max=100.0f};
constexpr VarMeta kMetaPressure{.unit="hPa",  .desc="barometric pressure",  .range_min=300.0f, .range_max=1100.0f};
constexpr VarMeta kMetaAltitude{.unit="m",    .desc="altitude from sea lev",.range_min=-500.0f,.range_max=9000.0f};
constexpr VarMeta kMetaVoltage {.unit="V",    .desc="supply voltage",       .range_min=0.0f,   .range_max=36.0f};
constexpr VarMeta kMetaCurrent {.unit="A",    .desc="load current",         .range_min=0.0f,   .range_max=10.0f};
constexpr VarMeta kMetaPower   {.unit="W",    .desc="active power",         .range_min=0.0f,   .range_max=360.0f};
constexpr VarMeta kMetaEnergy  {.unit="Wh",   .desc="accumulated energy",   .range_min=0.0f,   .range_max=1e9f};
constexpr VarMeta kMetaKp      {.unit="",     .desc="PID proportional gain",.range_min=0.0f,   .range_max=100.0f};
constexpr VarMeta kMetaKi      {.unit="",     .desc="PID integral gain",    .range_min=0.0f,   .range_max=100.0f};
constexpr VarMeta kMetaKd      {.unit="",     .desc="PID derivative gain",  .range_min=0.0f,   .range_max=100.0f};

// ============================================================================
// Variable descriptors
// ============================================================================

// Environmental (0x00–0x07)
constexpr Var<float>         kTemp        {.address=0x00u, .access=Access::ReadOnly,  .name="temperature"};
constexpr Var<float>         kHumidity    {.address=0x02u, .access=Access::ReadOnly,  .name="humidity"};
constexpr Var<float>         kPressure    {.address=0x04u, .access=Access::ReadOnly,  .name="pressure"};
constexpr Var<float>         kAltitude    {.address=0x06u, .access=Access::ReadOnly,  .name="altitude"};

// Power (0x08–0x0F)
constexpr Var<float>         kVoltage     {.address=0x08u, .access=Access::ReadOnly,  .name="voltage"};
constexpr Var<float>         kCurrent     {.address=0x0Au, .access=Access::ReadOnly,  .name="current"};
constexpr Var<float>         kPower       {.address=0x0Cu, .access=Access::ReadOnly,  .name="power"};
constexpr Var<float>         kEnergy      {.address=0x0Eu, .access=Access::ReadOnly,  .name="energy"};

// Control (0x10–0x1B)
constexpr Var<float>         kSetpointTemp{.address=0x10u, .access=Access::ReadWrite, .name="setpoint_temp"};
constexpr Var<float>         kSetpointHum {.address=0x12u, .access=Access::ReadWrite, .name="setpoint_hum"};
constexpr Var<float>         kKp          {.address=0x14u, .access=Access::ReadWrite, .name="pid_kp"};
constexpr Var<float>         kKi          {.address=0x16u, .access=Access::ReadWrite, .name="pid_ki"};
constexpr Var<float>         kKd          {.address=0x18u, .access=Access::ReadWrite, .name="pid_kd"};
constexpr Var<bool>          kEnable      {.address=0x1Au, .access=Access::ReadWrite, .name="enable"};
constexpr Var<bool>          kFanEnable   {.address=0x1Bu, .access=Access::ReadWrite, .name="fan_enable"};

// Diagnostics (0x1C–0x27)
constexpr Var<std::uint32_t> kUptime      {.address=0x1Cu, .access=Access::ReadOnly,  .name="uptime_s"};
constexpr Var<std::uint16_t> kErrorFlags  {.address=0x1Eu, .access=Access::ReadOnly,  .name="error_flags"};
constexpr Var<std::uint16_t> kWarnFlags   {.address=0x1Fu, .access=Access::ReadOnly,  .name="warn_flags"};
constexpr Var<std::uint32_t> kResetCount  {.address=0x20u, .access=Access::ReadOnly,  .name="reset_count"};
constexpr Var<std::uint16_t> kWdgReload   {.address=0x22u, .access=Access::ReadWrite, .name="wdg_reload_ms"};
constexpr Var<std::int16_t>  kBoardTemp   {.address=0x23u, .access=Access::ReadOnly,  .name="board_temp_raw"};
constexpr Var<std::uint16_t> kFwMajor     {.address=0x24u, .access=Access::ReadOnly,  .name="fw_major"};
constexpr Var<std::uint16_t> kFwMinor     {.address=0x25u, .access=Access::ReadOnly,  .name="fw_minor"};
constexpr Var<std::uint16_t> kFwPatch     {.address=0x26u, .access=Access::ReadOnly,  .name="fw_patch"};
constexpr Var<std::uint16_t> kDeviceId    {.address=0x27u, .access=Access::ReadOnly,  .name="device_id"};

// ============================================================================
// Backing storage
// ============================================================================

float         g_temp{25.0f},     g_humidity{50.0f};
float         g_pressure{1013.25f}, g_altitude{0.0f};
float         g_voltage{12.0f},  g_current{0.0f};
float         g_power{0.0f},     g_energy{0.0f};
float         g_sp_temp{22.0f},  g_sp_hum{45.0f};
float         g_kp{1.0f}, g_ki{0.1f}, g_kd{0.01f};
bool          g_enable{false},   g_fan{false};
std::uint32_t g_uptime{0u},      g_resets{0u};
std::uint16_t g_errors{0u},      g_warns{0u};
std::uint16_t g_wdg{1000u};
std::int16_t  g_board_temp{0};
std::uint16_t g_fw_major{0u}, g_fw_minor{1u}, g_fw_patch{0u};
std::uint16_t g_device_id{0xAB01u};

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    if (uart.configure().is_err()) {
        while (true) { board::led::toggle(); }
    }

    alloy::modbus::UartStream stream{uart};

    auto registry = alloy::modbus::Registry<30u>{std::array<alloy::modbus::VarDescriptor, 30u>{
        // Environmental
        bind(kTemp,         g_temp,      &kMetaTemp),
        bind(kHumidity,     g_humidity,  &kMetaHumid),
        bind(kPressure,     g_pressure,  &kMetaPressure),
        bind(kAltitude,     g_altitude,  &kMetaAltitude),
        // Power
        bind(kVoltage,      g_voltage,   &kMetaVoltage),
        bind(kCurrent,      g_current,   &kMetaCurrent),
        bind(kPower,        g_power,     &kMetaPower),
        bind(kEnergy,       g_energy,    &kMetaEnergy),
        // Control
        bind(kSetpointTemp, g_sp_temp),
        bind(kSetpointHum,  g_sp_hum),
        bind(kKp,           g_kp,        &kMetaKp),
        bind(kKi,           g_ki,        &kMetaKi),
        bind(kKd,           g_kd,        &kMetaKd),
        bind(kEnable,       g_enable),
        bind(kFanEnable,    g_fan),
        // Diagnostics
        bind(kUptime,       g_uptime),
        bind(kErrorFlags,   g_errors),
        bind(kWarnFlags,    g_warns),
        bind(kResetCount,   g_resets),
        bind(kWdgReload,    g_wdg),
        bind(kBoardTemp,    g_board_temp),
        bind(kFwMajor,      g_fw_major),
        bind(kFwMinor,      g_fw_minor),
        bind(kFwPatch,      g_fw_patch),
        bind(kDeviceId,     g_device_id),
        // (5 spare slots to reach 30)
        bind(Var<std::uint16_t>{.address=0x28u,.access=Access::ReadWrite,.name="spare0"}, g_errors),
        bind(Var<std::uint16_t>{.address=0x29u,.access=Access::ReadWrite,.name="spare1"}, g_errors),
        bind(Var<std::uint16_t>{.address=0x2Au,.access=Access::ReadWrite,.name="spare2"}, g_errors),
        bind(Var<std::uint16_t>{.address=0x2Bu,.access=Access::ReadWrite,.name="spare3"}, g_errors),
        bind(Var<std::uint16_t>{.address=0x2Cu,.access=Access::ReadWrite,.name="spare4"}, g_errors),
    }};

    // Discovery FC 0x65 is enabled by default.
    alloy::modbus::Slave slave{stream, static_cast<std::uint8_t>(SLAVE_ID), registry};

    while (true) {
        // Update read-only sensor values (board-specific).
        g_temp     = board::sensor::read_temperature();
        g_humidity = board::sensor::read_humidity();
        g_voltage  = board::adc::read_voltage();
        ++g_uptime;

        (void)slave.poll(10'000u);
    }
}
