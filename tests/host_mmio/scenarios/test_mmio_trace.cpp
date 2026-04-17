#include "host_mmio/framework/boot_probe.hpp"
#include "host_mmio/framework/mmio_space.hpp"
#include "host_mmio/framework/register_expect.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

#include <array>
#include <string>
#include <vector>

namespace alloy::test::mmio {
namespace {

constexpr auto kPmcPcer0 = std::uintptr_t{0x400e0610u};
constexpr auto kPioABcdsr = std::uintptr_t{0x400e0e70u};
constexpr auto kUsartMr = std::uintptr_t{0x40024004u};

}  // namespace

TEST_CASE("host mmio trace records ordered boot-side register programming", "[host-mmio]") {
    trace_log trace;
    mmio_space mmio{trace};

    mmio.preload(kUsartMr, 0xffff0000u);
    mmio.write32(kPmcPcer0, 0x00000001u);
    mmio.set_bits(kPioABcdsr, 0x00000020u);
    mmio.write_masked(kUsartMr, 0x0000ffffu, 0x00000008u);
    const auto usart_mode = mmio.read32(kUsartMr);

    REQUIRE(usart_mode == 0xffff0008u);

    const auto expected = std::array{
        access{.kind = access_kind::write, .address = kPmcPcer0, .value = 0x00000001u, .mask = 0u},
        access{.kind = access_kind::set_bits, .address = kPioABcdsr, .value = 0x00000020u, .mask = 0x00000020u},
        access{.kind = access_kind::write_masked, .address = kUsartMr, .value = 0xffff0008u, .mask = 0x0000ffffu},
        access{.kind = access_kind::read, .address = kUsartMr, .value = 0xffff0008u, .mask = 0u},
    };

    REQUIRE(trace.entries().size() == expected.size());
    for (auto index = std::size_t{0}; index < expected.size(); ++index) {
        INFO("trace entry " << index << '\n' << describe_trace(trace));
        REQUIRE(trace.entries()[index] == expected[index]);
    }
}

TEST_CASE("boot probe captures reset-handler to main lifecycle", "[host-mmio]") {
    boot_probe probe;

    probe.enter_reset_handler();
    probe.checkpoint("clock-ready");
    probe.checkpoint("uart-ready");
    probe.enter_main();

    REQUIRE(probe.reset_handler_entered());
    REQUIRE(probe.main_entered());
    REQUIRE(probe.checkpoints() == std::vector<std::string>{"reset-handler", "clock-ready", "uart-ready", "main"});
}

}  // namespace alloy::test::mmio
