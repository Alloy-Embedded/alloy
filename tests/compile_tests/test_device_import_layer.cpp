#include "arch/cortex_m/startup_contract.hpp"
#include "device/descriptors.hpp"
#include "device/import.hpp"
#include "device/traits.hpp"

#include <iterator>

static_assert(alloy::device::SelectedDeviceTraits::available,
              "Selected device contract must be available for the smoke target.");
static_assert(!alloy::device::SelectedDeviceTraits::vendor.empty());
static_assert(!alloy::device::SelectedDeviceTraits::family.empty());
static_assert(!alloy::device::SelectedDeviceTraits::name.empty());

int main() {
    using namespace alloy::device;

    static_assert(imported::available);
    static_assert(SelectedDescriptors::available);
    static_assert(alloy::arch::cortex_m::SelectedStartupContract::available);
    static_assert(std::size(alloy::arch::cortex_m::kVectorSlots) > 0);
    static_assert(std::size(alloy::arch::cortex_m::kStartupDescriptors) > 0);

    return 0;
}
