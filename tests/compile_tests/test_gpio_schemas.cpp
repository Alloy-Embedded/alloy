// test_gpio_schemas.cpp — Tasks 3.3 + 3.4 (gpio-schema-open-traits)
//
// Compile-time verification that all schema types satisfy GpioSchemaImpl.
// Host-only compile test — no hardware required.

#include "hal/detail/gpio_schema_concept.hpp"
#include "hal/detail/gpio/st_gpio_schema.hpp"
#include "hal/detail/gpio/microchip_pio_schema.hpp"
#include "hal/detail/gpio/nxp_imxrt_gpio_schema.hpp"
#include "hal/detail/gpio/unknown_gpio_schema.hpp"

using namespace alloy::hal::gpio::detail;

// --- concept satisfaction ---

static_assert(GpioSchemaImpl<StGpioSchema>);
static_assert(GpioSchemaImpl<MicrochipPioSchema>);
static_assert(GpioSchemaImpl<NxpImxrtGpioSchema>);
static_assert(GpioSchemaImpl<UnknownGpioSchema>);

// --- optional field detection ---

static_assert(HasSpeedField<StGpioSchema>);
static_assert(HasAfField<StGpioSchema>);
static_assert(HasOpenDrainField<StGpioSchema>);

static_assert(!HasSpeedField<UnknownGpioSchema>);
static_assert(!HasAfField<UnknownGpioSchema>);

// --- field computations return non-zero valid refs for real schemas ---

// STM32 GPIOA at 0x48000000, pin 2
static constexpr std::uintptr_t kGpioABase = 0x48000000u;
static constexpr auto kModeField  = StGpioSchema::mode_field(kGpioABase, 2);
static constexpr auto kPullField  = StGpioSchema::pull_field(kGpioABase, 2);
static constexpr auto kAfField    = StGpioSchema::af_field(kGpioABase, 2);
static constexpr auto kSetField   = StGpioSchema::output_set_field(kGpioABase, 2);
static constexpr auto kClearField = StGpioSchema::output_clear_field(kGpioABase, 2);

static_assert(kModeField.valid);
static_assert(kModeField.bit_offset == 4u);   // pin 2 → MODER bits [5:4]
static_assert(kModeField.bit_width == 2u);
static_assert(kPullField.valid);
static_assert(kPullField.bit_offset == 4u);   // PUPDR bits [5:4]
static_assert(kSetField.valid);
static_assert(kSetField.bit_offset == 2u);    // BSRR bit 2
static_assert(kClearField.valid);
static_assert(kClearField.bit_offset == 18u); // BSRR bit 18 (2+16)
static_assert(kAfField.valid);
static_assert(kAfField.bit_offset == 8u);     // AFR[0] bits [11:8] for pin 2

// Unknown schema always returns invalid
static constexpr auto kUnknownMode = UnknownGpioSchema::mode_field(kGpioABase, 0);
static_assert(!kUnknownMode.valid);

// Microchip PIO PIOA at 0x400E0E00, pin 5
static constexpr std::uintptr_t kPioABase = 0x400E0E00u;
static constexpr auto kPioMode  = MicrochipPioSchema::mode_field(kPioABase, 5);
static constexpr auto kPioSodr  = MicrochipPioSchema::output_set_field(kPioABase, 5);
static_assert(kPioMode.valid);
static_assert(kPioMode.bit_offset == 5u);     // OER bit 5
static_assert(kPioSodr.valid);
static_assert(kPioSodr.bit_offset == 5u);     // SODR bit 5

// schema name strings are valid
static_assert(StGpioSchema::kSchemaName != nullptr);
static_assert(MicrochipPioSchema::kSchemaName != nullptr);
static_assert(NxpImxrtGpioSchema::kSchemaName != nullptr);
static_assert(UnknownGpioSchema::kSchemaName != nullptr);
