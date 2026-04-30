#pragma once

namespace alloy::dev {

#if ALLOY_DEVICE_RUNTIME_AVAILABLE

namespace pin {
using enum device::PinId;
}

namespace periph {
using enum device::PeripheralId;
}

namespace sig {
using enum device::SignalId;
}

#endif

}  // namespace alloy::dev
