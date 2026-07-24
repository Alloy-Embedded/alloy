// Unit tests for the app-services layer against hardware test-doubles: the
// compile-time logger (over a fake ByteSink) and the cooperative scheduler
// (periodic firing + ISR-style event delivery, driven by an injected clock).

#include <chrono>
#include <cstdint>

#include "alloy/log.hpp"
#include "alloy/sched.hpp"
#include "alloy_test.hpp"
#include "doubles.hpp"

using namespace alloy;

ALLOY_TEST(logger_gates_below_min_level) {
    test::fake_uart<> u;
    auto lg = log::make<log::level::info>(u);
    lg.debug("secret");  // below info -> compiled out, emits nothing
    lg.info("visible");
    ALLOY_CHECK(!u.tx_contains("secret"));
    ALLOY_CHECK(u.tx_contains("visible"));
    ALLOY_CHECK(u.tx_contains("[I] "));
}

ALLOY_TEST(logger_off_is_silent) {
    test::fake_uart<> u;
    auto lg = log::make<log::level::off>(u);
    lg.error("boom");
    ALLOY_CHECK_EQ(u.tx_len, 0u);
}

ALLOY_TEST(logger_formats_hex_value) {
    test::fake_uart<> u;
    auto lg = log::make<log::level::trace>(u);
    lg.info("phy id", 0x156au);
    ALLOY_CHECK(u.tx_contains("0x0000156a"));
}

namespace {
int g_hb_count = 0;
std::uint32_t g_last_events = 0;
void hb_task(void*, std::uint32_t) { ++g_hb_count; }
void ev_task(void*, std::uint32_t events) { g_last_events = events; }
}  // namespace

ALLOY_TEST(scheduler_fires_periodic_on_cadence) {
    g_hb_count = 0;
    scheduler<4> s;
    const std::size_t i = s.add(&hb_task, nullptr, std::chrono::milliseconds{100});
    ALLOY_CHECK_EQ(i, 0u);

    s.run_once(0u);
    ALLOY_CHECK_EQ(g_hb_count, 0);  // not yet due
    s.run_once(100u);
    ALLOY_CHECK_EQ(g_hb_count, 1);  // due at +100
    s.run_once(150u);
    ALLOY_CHECK_EQ(g_hb_count, 1);  // 50 < 100
    s.run_once(205u);
    ALLOY_CHECK_EQ(g_hb_count, 2);  // due again (phase-preserving)
}

ALLOY_TEST(scheduler_delivers_and_clears_events) {
    g_last_events = 0;
    scheduler<4> s;
    const std::size_t i = s.add(&ev_task, nullptr, std::chrono::milliseconds{0});  // event-only

    s.run_once(0u);
    ALLOY_CHECK_EQ(g_last_events, 0u);  // no period, no event -> not run

    s.signal(i, 0x5u);  // ISR-style
    s.run_once(0u);
    ALLOY_CHECK_EQ(g_last_events, 0x5u);  // delivered

    g_last_events = 0;
    s.run_once(0u);
    ALLOY_CHECK_EQ(g_last_events, 0u);  // cleared after delivery
}
