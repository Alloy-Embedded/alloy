#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/i2c.hpp"

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_SAME70_XPLD)
using I2cConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::TWIHS0,
    alloy::hal::connection::scl<alloy::device::PinId::PA4, alloy::device::SignalId::signal_twck0>,
    alloy::hal::connection::sda<alloy::device::PinId::PA3, alloy::device::SignalId::signal_twd0>>;
using I2cPort = decltype(alloy::hal::i2c::open<I2cConnector>());

static_assert(I2cPort::start_field.valid,   "TWIHS0 kStartField must be valid (task 8.1)");
static_assert(I2cPort::stop_field.valid,    "TWIHS0 kStopField must be valid (task 8.1)");
static_assert(I2cPort::txcomp_field.valid,  "TWIHS0 kTxcompField must be valid (task 8.1)");
static_assert(I2cPort::rxrdy_field.valid,   "TWIHS0 kRxrdyField must be valid (task 8.1)");
static_assert(I2cPort::txrdy_field.valid,   "TWIHS0 kTxrdyField must be valid (task 8.1)");
static_assert(I2cPort::nack_field.valid,    "TWIHS0 kNackField must be valid (task 8.1)");
static_assert(I2cPort::arblst_field.valid,  "TWIHS0 kArblstField must be valid (task 8.1)");
#endif
