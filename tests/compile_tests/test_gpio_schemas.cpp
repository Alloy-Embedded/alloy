// test_gpio_schemas.cpp — Tasks 3.3 + 3.4 + 4.1–4.3 (gpio-schema-open-traits)
//
// Compile-time verification that all schema types satisfy GpioSchemaImpl.
// Host-only compile test — no hardware required.

#include "hal/detail/gpio_schema_concept.hpp"
#include "hal/detail/gpio/st_gpio_schema.hpp"
#include "hal/detail/gpio/microchip_pio_schema.hpp"
#include "hal/detail/gpio/nxp_imxrt_gpio_schema.hpp"
#include "hal/detail/gpio/unknown_gpio_schema.hpp"
// New vendor schemas (tasks 4.1–4.3)
#include "hal/detail/gpio/nordic_gpiote_schema.hpp"
#include "hal/detail/gpio/rp2040_gpio_schema.hpp"
#include "hal/detail/gpio/espressif_gpio_schema.hpp"

using namespace alloy::hal::gpio::detail;

// --- concept satisfaction — original schemas (tasks 1.2–1.5) ---

static_assert(GpioSchemaImpl<StGpioSchema>);
static_assert(GpioSchemaImpl<MicrochipPioSchema>);
static_assert(GpioSchemaImpl<NxpImxrtGpioSchema>);
static_assert(GpioSchemaImpl<UnknownGpioSchema>);

// --- concept satisfaction — new vendor schemas (tasks 4.1–4.3) ---

static_assert(GpioSchemaImpl<NordicGpioteSchema>);
static_assert(GpioSchemaImpl<Rp2040GpioSchema>);
static_assert(GpioSchemaImpl<EspressifGpioSchema>);

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
static_assert(NordicGpioteSchema::kSchemaName != nullptr);
static_assert(Rp2040GpioSchema::kSchemaName != nullptr);
static_assert(EspressifGpioSchema::kSchemaName != nullptr);

// --- field correctness — new vendor schemas ---

// nRF52840 P0 base = 0x50000000, pin 5
static constexpr std::uintptr_t kNrfP0Base = 0x50000000u;
static constexpr auto kNrfMode  = NordicGpioteSchema::mode_field(kNrfP0Base, 5);
static constexpr auto kNrfIn    = NordicGpioteSchema::input_data_field(kNrfP0Base, 5);
static constexpr auto kNrfSet   = NordicGpioteSchema::output_set_field(kNrfP0Base, 5);
static constexpr auto kNrfClear = NordicGpioteSchema::output_clear_field(kNrfP0Base, 5);
static constexpr auto kNrfPull  = NordicGpioteSchema::pull_field(kNrfP0Base, 5);
static_assert(kNrfMode.valid);   // PIN_CNF[5] bit 0
static_assert(kNrfMode.bit_offset == 0u);
static_assert(kNrfIn.valid);     // IN register bit 5
static_assert(kNrfIn.bit_offset == 5u);
static_assert(kNrfSet.valid);    // OUTSET bit 5
static_assert(kNrfClear.valid);  // OUTCLR bit 5
static_assert(kNrfPull.valid);   // PIN_CNF[5] bits [5:4]
static_assert(kNrfPull.bit_offset == 4u);
static_assert(kNrfPull.bit_width == 2u);

// RP2040 SIO base = 0xD0000000, pin 3
static constexpr std::uintptr_t kSioBase = 0xD0000000u;
static constexpr auto kRpMode  = Rp2040GpioSchema::mode_field(kSioBase, 3);
static constexpr auto kRpSet   = Rp2040GpioSchema::output_set_field(kSioBase, 3);
static constexpr auto kRpClear = Rp2040GpioSchema::output_clear_field(kSioBase, 3);
static constexpr auto kRpIn    = Rp2040GpioSchema::input_data_field(kSioBase, 3);
static constexpr auto kRpPull  = Rp2040GpioSchema::pull_field(kSioBase, 3);
static_assert(kRpMode.valid);
static_assert(kRpMode.bit_offset == 3u);    // GPIO_OE_SET bit 3
static_assert(kRpSet.valid);
static_assert(kRpSet.bit_offset == 3u);     // GPIO_OUT_SET bit 3
static_assert(kRpClear.valid);
static_assert(kRpIn.valid);
static_assert(!kRpPull.valid);              // pulls in PADS block

// ESP32 GPIO base = 0x3FF44000, pin 7
static constexpr std::uintptr_t kEspBase = 0x3FF44000u;
static constexpr auto kEspMode  = EspressifGpioSchema::mode_field(kEspBase, 7);
static constexpr auto kEspSet   = EspressifGpioSchema::output_set_field(kEspBase, 7);
static constexpr auto kEspClear = EspressifGpioSchema::output_clear_field(kEspBase, 7);
static constexpr auto kEspIn    = EspressifGpioSchema::input_data_field(kEspBase, 7);
static constexpr auto kEspPull  = EspressifGpioSchema::pull_field(kEspBase, 7);
static_assert(kEspMode.valid);
static_assert(kEspMode.bit_offset == 7u);   // GPIO_ENABLE_W1TS bit 7
static_assert(kEspSet.valid);
static_assert(kEspSet.bit_offset == 7u);    // GPIO_OUT_W1TS bit 7
static_assert(kEspClear.valid);
static_assert(kEspIn.valid);
static_assert(!kEspPull.valid);             // pulls in IO_MUX
