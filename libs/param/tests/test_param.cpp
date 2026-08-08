// Parameter registry + bank: table discipline, the strict-vs-clamped write
// split (the wire refuses, the preset lands), O(1) tick reads, and the
// persistence properties that matter on a power-cycled product — round-trip,
// newest-region-wins, a torn write costing one generation (never the data),
// re-clamp on load after a schema tightens, and the deferred-commit window
// batching a write burst into one erase cycle.

#include "param.hpp"

#include <cstdint>
#include <cstring>

#include "alloy_test.hpp"

namespace {
using namespace alloy::lib::param;

constexpr descriptor kTable[] = {
    {.id = 10, .def = 40, .min = -100, .max = 100},
    {.id = 11, .def = 0, .min = 0, .max = 1},
    {.id = 20, .def = -50, .min = -1000, .max = 1000},
    {.id = 21, .def = 25, .min = 25, .max = 25, .read_only = true},
    {.id = 60, .def = 7, .min = 0, .max = 15, .persist = false},
};
constexpr std::size_t kN = sizeof kTable / sizeof kTable[0];

struct fake_flash {
    alignas(std::uint64_t) static inline std::uint8_t mem[512];
    static inline bool fail_program = false;
    static inline int programs_until_fail = -1;  // -1 = never

    [[nodiscard]] bool erase_page(std::uintptr_t addr) const {
        std::memset(reinterpret_cast<void*>(addr), 0xFF, 128);
        return true;
    }
    [[nodiscard]] bool program(std::uintptr_t addr, const std::uint64_t* data,
                               std::size_t n) const {
        if (fail_program) {
            return false;
        }
        if (programs_until_fail == 0) {
            return false;
        }
        if (programs_until_fail > 0) {
            --programs_until_fail;
        }
        std::memcpy(reinterpret_cast<void*>(addr), data, n * 8u);
        return true;
    }
};

std::uintptr_t region_a() { return reinterpret_cast<std::uintptr_t>(fake_flash::mem); }
std::uintptr_t region_b() { return region_a() + 256u; }
void fresh() {
    std::memset(fake_flash::mem, 0xFF, sizeof fake_flash::mem);
    fake_flash::fail_program = false;
    fake_flash::programs_until_fail = -1;
}
bank<fake_flash>::config cfg() {
    return {.region_a = region_a(), .region_b = region_b(),
            .region_bytes = 128, .page_size = 128, .quiet_ms = 500};
}
}  // namespace

ALLOY_TEST(param_defaults_populate_and_tick_read_is_an_index) {
    registry<kN> reg{kTable};
    ALLOY_CHECK_EQ(reg.get_at(0), 40);
    ALLOY_CHECK_EQ(reg.get_at(2), -50);
    const auto idx = reg.index_of(20);
    ALLOY_CHECK(idx.has_value());
    ALLOY_CHECK_EQ(reg.get_at(*idx), -50);
}

ALLOY_TEST(param_strict_set_refuses_with_the_reason_the_wire_needs) {
    registry<kN> reg{kTable};
    ALLOY_CHECK(reg.set(10, 99).has_value());
    ALLOY_CHECK_EQ(reg.get(10).value(), 99);

    const auto range = reg.set(10, 101);  // beyond max
    ALLOY_CHECK(!range);
    ALLOY_CHECK(range.error() == set_error::out_of_range);  // -> exception 0x03
    ALLOY_CHECK_EQ(reg.get(10).value(), 99);                // untouched

    const auto ro = reg.set(21, 30);
    ALLOY_CHECK(!ro);
    ALLOY_CHECK(ro.error() == set_error::read_only);

    const auto unknown = reg.set(999, 1);
    ALLOY_CHECK(!unknown);
    ALLOY_CHECK(unknown.error() == set_error::unknown_id);  // -> exception 0x02
}

ALLOY_TEST(param_clamped_set_lands_presets_on_the_nearest_legal_value) {
    registry<kN> reg{kTable};
    ALLOY_CHECK(reg.set_clamped(10, 5000).has_value());
    ALLOY_CHECK_EQ(reg.get(10).value(), 100);  // clamped to max, not refused
    ALLOY_CHECK(reg.set_clamped(20, -5000).has_value());
    ALLOY_CHECK_EQ(reg.get(20).value(), -1000);
}

ALLOY_TEST(param_persist_round_trip_and_newest_region_wins) {
    fresh();
    fake_flash f;
    bank<fake_flash> bk{f, cfg()};
    registry<kN> reg{kTable};

    ALLOY_CHECK(reg.set(10, 77).has_value());
    ALLOY_CHECK(bk.commit(reg));       // generation 1 -> region A
    ALLOY_CHECK(reg.set(10, 88).has_value());
    ALLOY_CHECK(bk.commit(reg));       // generation 2 -> region B

    bank<fake_flash> bk2{f, cfg()};    // a fresh boot
    registry<kN> reg2{kTable};
    ALLOY_CHECK(bk2.load(reg2));
    ALLOY_CHECK_EQ(reg2.get(10).value(), 88);  // the NEWEST generation
    ALLOY_CHECK(!reg2.dirty());
}

ALLOY_TEST(param_torn_commit_costs_one_generation_never_the_data) {
    fresh();
    fake_flash f;
    bank<fake_flash> bk{f, cfg()};
    registry<kN> reg{kTable};

    ALLOY_CHECK(reg.set(10, 77).has_value());
    ALLOY_CHECK(bk.commit(reg));

    // Power dies mid-program of the NEXT commit: the trailer never lands.
    ALLOY_CHECK(reg.set(10, 88).has_value());
    fake_flash::programs_until_fail = 2;
    ALLOY_CHECK(!bk.commit(reg));

    bank<fake_flash> bk2{f, cfg()};
    registry<kN> reg2{kTable};
    ALLOY_CHECK(bk2.load(reg2));
    ALLOY_CHECK_EQ(reg2.get(10).value(), 77);  // previous generation intact
}

ALLOY_TEST(param_load_reclamps_when_a_schema_tightened_the_range) {
    fresh();
    fake_flash f;
    bank<fake_flash> bk{f, cfg()};
    registry<kN> reg{kTable};
    ALLOY_CHECK(reg.set(20, 900).has_value());
    ALLOY_CHECK(bk.commit(reg));

    // The same table, but id 20's range tightened to [-100, 100] in a newer
    // firmware: the stored 900 must HEAL to the boundary, not factory-reset.
    constexpr descriptor tight[] = {
        {.id = 10, .def = 40, .min = -100, .max = 100},
        {.id = 11, .def = 0, .min = 0, .max = 1},
        {.id = 20, .def = -50, .min = -100, .max = 100},
        {.id = 21, .def = 25, .min = 25, .max = 25, .read_only = true},
        {.id = 60, .def = 7, .min = 0, .max = 15, .persist = false},
    };
    registry<kN> reg2{std::span<const descriptor, kN>{tight}};
    bank<fake_flash> bk2{f, cfg()};
    ALLOY_CHECK(bk2.load(reg2));
    ALLOY_CHECK_EQ(reg2.get(20).value(), 100);
}

ALLOY_TEST(param_deferred_commit_batches_a_burst_into_one_erase) {
    fresh();
    fake_flash f;
    bank<fake_flash> bk{f, cfg()};
    registry<kN> reg{kTable};

    ALLOY_CHECK(reg.set(10, 1).has_value());
    ALLOY_CHECK(!bk.poll(reg, 1000));  // dirty observed: window starts
    ALLOY_CHECK(reg.set(10, 2).has_value());
    ALLOY_CHECK(!bk.poll(reg, 1300));  // burst continues: window restarts
    ALLOY_CHECK(!bk.poll(reg, 1700));  // quiet 400 ms < 500: not yet
    ALLOY_CHECK(bk.poll(reg, 1801));   // quiet >= 500: ONE commit fires

    bank<fake_flash> bk2{f, cfg()};
    registry<kN> reg2{kTable};
    ALLOY_CHECK(bk2.load(reg2));
    ALLOY_CHECK_EQ(reg2.get(10).value(), 2);  // the last of the burst
}

ALLOY_TEST(param_blank_flash_is_factory_state_not_an_error_path) {
    fresh();
    fake_flash f;
    bank<fake_flash> bk{f, cfg()};
    registry<kN> reg{kTable};
    ALLOY_CHECK(!bk.load(reg));               // nothing stored
    ALLOY_CHECK_EQ(reg.get(10).value(), 40);  // defaults stand
}

// ── the bridge: a registry-backed Modbus DataModel ──────────────────────────
// ~20 lines of app code close the chain wire -> registry -> flash: set_error
// maps onto the rich write verdict (unknown id -> 0x02, out-of-range -> 0x03)
// so the server answers with the registry's own reason. Lives here as a test,
// not in either library — neither needs to know the other exists.

#include "modbus/crc.hpp"
#include "modbus/server.hpp"
#include "testkit/mock_wire.hpp"

namespace {
namespace mb = alloy::lib::modbus;

struct param_model {
    registry<kN>& reg;

    [[nodiscard]] bool read_coil(std::uint16_t, bool&) const { return false; }
    [[nodiscard]] bool write_coil(std::uint16_t, bool) { return false; }
    [[nodiscard]] bool read_discrete_input(std::uint16_t, bool&) const { return false; }
    [[nodiscard]] bool read_input(std::uint16_t, std::uint16_t&) const { return false; }
    [[nodiscard]] bool read_holding(std::uint16_t id, std::uint16_t& v) const {
        const auto r = reg.get(id);
        if (!r) {
            return false;
        }
        v = static_cast<std::uint16_t>(*r);
        return true;
    }
    [[nodiscard]] alloy::Result<void, mb::modbus_error> write_holding(
        std::uint16_t id, std::uint16_t v) {
        const auto r = reg.set(id, static_cast<std::int16_t>(v));
        if (r) {
            return alloy::ok<mb::modbus_error>();
        }
        return r.error() == set_error::out_of_range
                   ? mb::modbus_error::exception_illegal_data_value
                   : mb::modbus_error::exception_illegal_data_address;
    }
};
static_assert(mb::DataModel<param_model>);

struct stepping_clock {
    alloy::testkit::virtual_clock* clk;
    [[nodiscard]] std::uint32_t now_us() const {
        clk->advance_us(100);
        return clk->now_us;
    }
};
}  // namespace

ALLOY_TEST(param_registry_answers_modbus_with_its_own_reasons) {
    registry<kN> reg{kTable};
    param_model model{reg};
    alloy::testkit::mock_serial wire;
    alloy::testkit::virtual_clock vc;
    mb::rtu_server<alloy::testkit::mock_serial, param_model, 8, stepping_clock>
        srv{wire, model, {.unit = 9, .baud = 19'200}, {&vc}};

    // FC06 id 10 := 99 (legal): echo, and the registry holds it.
    std::uint8_t w1[8] = {0x09, 0x06, 0x00, 0x0A, 0x00, 0x63, 0, 0};
    const std::size_t n1 = mb::append_crc(w1, 6);
    wire.queue_rx({w1, n1});
    ALLOY_CHECK(srv.poll());
    ALLOY_CHECK_EQ(reg.get(10).value(), 99);

    // FC06 id 10 := 101 (beyond max): exception 0x03 FROM THE REGISTRY.
    wire.reset();
    std::uint8_t w2[8] = {0x09, 0x06, 0x00, 0x0A, 0x00, 0x65, 0, 0};
    const std::size_t n2 = mb::append_crc(w2, 6);
    wire.queue_rx({w2, n2});
    ALLOY_CHECK(srv.poll());
    ALLOY_CHECK_EQ(wire.tx[1], 0x86u);
    ALLOY_CHECK_EQ(wire.tx[2], 0x03u);
    ALLOY_CHECK_EQ(reg.get(10).value(), 99);  // refused, value stands

    // FC06 unknown id: exception 0x02.
    wire.reset();
    std::uint8_t w3[8] = {0x09, 0x06, 0x03, 0xE7, 0x00, 0x01, 0, 0};
    const std::size_t n3 = mb::append_crc(w3, 6);
    wire.queue_rx({w3, n3});
    ALLOY_CHECK(srv.poll());
    ALLOY_CHECK_EQ(wire.tx[1], 0x86u);
    ALLOY_CHECK_EQ(wire.tx[2], 0x02u);
}
