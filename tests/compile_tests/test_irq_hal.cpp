// test_irq_hal.cpp — Tasks 2.6 + 3.4 (add-irq-vector-hal)
//
// Compile-time and link-time verification of the IRQ HAL API.
// Host-only — uses NoopImpl (no NVIC, no hardware).

#include "hal/irq/irq_id.hpp"
#include "hal/irq/irq_vector_traits.hpp"
#include "hal/irq/irq_handle.hpp"
#include "hal/irq/irq_table.hpp"
#include "device/runtime.hpp"
#include "core/result.hpp"

using namespace alloy;
using namespace alloy::hal::irq;
using alloy::device::runtime::PeripheralId;

// --- IrqId ---

static_assert(IrqId{5}.is_valid());
static_assert(!kInvalidIrqId.is_valid());
static_assert(make_irq_id(28).value == 28u);
static_assert(!make_irq_id(-1).is_valid());

// --- IrqVectorTraits primary (no IRQ for PeripheralId::none) ---

static_assert(!IrqVectorTraits<PeripheralId::none>::kPresent);
static_assert(!IrqVectorTraits<PeripheralId::none>::kIrqId.is_valid());
static_assert(!kHasIrq<PeripheralId::none>);

// --- irq::enable<P> / disable<P> on peripheral without IRQ → NotSupported ---

void test_enable_no_irq() {
    auto r = enable<PeripheralId::none>(4u);
    (void)r;
    static_assert(
        std::is_same_v<decltype(enable<PeripheralId::none>(4u)),
                       core::Result<void, core::ErrorCode>>
    );
}

void test_disable_no_irq() {
    auto r = disable<PeripheralId::none>();
    (void)r;
}

void test_set_priority_no_irq() {
    auto r = set_priority<PeripheralId::none>(5u);
    (void)r;
}

// --- irq::set_handler / clear_handler (peripheral without IRQ → NotSupported) ---

void test_set_handler_no_irq() {
    auto r = set_handler<PeripheralId::none>([]() {});
    (void)r;
}

// --- dispatch_irq ---

void test_dispatch() {
    // Installing a handler at index 0 and dispatching should call it
    detail::irq_table()[0] = []() { /* handler */ };
    dispatch_irq(0u);
    detail::irq_table()[0] = nullptr;
}

// --- Return types ---

static_assert(std::is_same_v<decltype(set_handler<PeripheralId::none>(nullptr)),
                              core::Result<void, core::ErrorCode>>);
