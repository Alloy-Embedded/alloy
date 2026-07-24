// Unit tests for the gpio::bus bit-mapping logic (value bits <-> port pins).
// The atomic BSRR store itself is exercised on silicon (the gpio_bus example
// reads its own outputs back); here we pin the pure mask math that decides which
// port bits get set/cleared and how a read is gathered back into a value.

#include <array>
#include <cstdint>

#include "alloy/gpio.hpp"
#include "alloy_test.hpp"

using namespace alloy::gpio::detail;

ALLOY_TEST(bus_masks_maps_value_bits_to_pins) {
    const std::array<std::uint8_t, 3> bits{3, 4, 5};
    const auto m = bus_masks(0b101u, bits);
    ALLOY_CHECK_EQ(m.set, (1u << 3) | (1u << 5));
    ALLOY_CHECK_EQ(m.clear, (1u << 4));
}

ALLOY_TEST(bus_masks_all_zero_and_all_one) {
    const std::array<std::uint8_t, 3> bits{3, 4, 5};
    const auto z = bus_masks(0u, bits);
    ALLOY_CHECK_EQ(z.set, 0u);
    ALLOY_CHECK_EQ(z.clear, (1u << 3) | (1u << 4) | (1u << 5));
    const auto o = bus_masks(0b111u, bits);
    ALLOY_CHECK_EQ(o.set, (1u << 3) | (1u << 4) | (1u << 5));
    ALLOY_CHECK_EQ(o.clear, 0u);
}

ALLOY_TEST(bus_masks_ignores_value_bits_beyond_width) {
    const std::array<std::uint8_t, 3> bits{3, 4, 5};
    const auto m = bus_masks(0b1101u, bits);  // bit 3 has no pin -> dropped
    ALLOY_CHECK_EQ(m.set, (1u << 3) | (1u << 5));
    ALLOY_CHECK_EQ(m.clear, (1u << 4));
}

ALLOY_TEST(bus_gather_inverts_and_isolates) {
    const std::array<std::uint8_t, 3> bits{3, 4, 5};
    ALLOY_CHECK_EQ(bus_gather((1u << 3) | (1u << 5), bits), 0b101u);
    ALLOY_CHECK_EQ(bus_gather(0u, bits), 0u);
    // unrelated port pins must not leak into the value
    ALLOY_CHECK_EQ(bus_gather((1u << 7) | (1u << 4), bits), 0b010u);
}

ALLOY_TEST(bus_roundtrip_scattered_pins) {
    const std::array<std::uint8_t, 4> bits{0, 2, 7, 15};  // non-contiguous
    for (std::uint32_t v = 0; v < 16u; ++v) {
        const auto m = bus_masks(v, bits);
        // a push-pull output reflects its driven level in the input register:
        // the "port" reads back exactly the bits we set. read must recover v.
        ALLOY_CHECK_EQ(bus_gather(m.set, bits), v);
    }
}
