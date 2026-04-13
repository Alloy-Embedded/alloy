#include <iterator>

#include "device/import.hpp"
#include "device/runtime.hpp"
#include "device/traits.hpp"

static_assert(alloy::device::SelectedDeviceTraits::available,
              "Selected device contract must be available for the smoke target.");
static_assert(!alloy::device::SelectedDeviceTraits::vendor.empty());
static_assert(!alloy::device::SelectedDeviceTraits::family.empty());
static_assert(!alloy::device::SelectedDeviceTraits::name.empty());

int main() {
    using namespace alloy::device;

    static_assert(imported::available);
    static_assert(SelectedRuntimeDescriptors::available);
    static_assert(!runtime::peripherals.empty());
    static_assert(!runtime::pins.empty());
    static_assert(!runtime::registers.empty());

    return 0;
}
