#include "device/system_clock.hpp"

static_assert(alloy::device::SelectedSystemClockProfiles::available,
              "Selected device must publish system clock profiles.");

#if ALLOY_DEVICE_SYSTEM_CLOCK_AVAILABLE
static_assert(!alloy::device::system_clock::profiles.empty());
#endif

int main() {
    return 0;
}
