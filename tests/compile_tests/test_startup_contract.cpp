#include "device/startup.hpp"

static_assert(alloy::device::SelectedStartupDescriptors::available,
              "Selected device must publish startup descriptors.");

#if ALLOY_DEVICE_STARTUP_AVAILABLE
static_assert(!alloy::device::startup::vector_slots.empty());
static_assert(!alloy::device::startup::descriptors.empty());
#endif

int main() { return 0; }
