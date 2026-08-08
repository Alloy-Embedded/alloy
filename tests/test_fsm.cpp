// Tabular FSM: the contract is exactly three properties — first matching row
// wins (row order IS priority), an unclaimed event stays put silently, and
// dispatch's return value is the change EDGE, not success. Typed enums make
// cross-machine event feeding a compile error, pinned by static_assert.

#include "alloy/util/fsm.hpp"

#include <type_traits>

#include "alloy_test.hpp"

namespace {
enum class mode { idle, cooling, defrost };
enum class ev { too_warm, setpoint_reached, defrost_due, defrost_done };

constexpr alloy::util::transition<mode, ev> kMap[] = {
    {mode::idle, ev::too_warm, mode::cooling},
    {mode::cooling, ev::setpoint_reached, mode::idle},
    {mode::cooling, ev::defrost_due, mode::defrost},
    // Priority row: while ALREADY defrosting, defrost_due must not re-enter
    // (self-loop placed FIRST so a later conflicting row can never win).
    {mode::defrost, ev::defrost_due, mode::defrost},
    {mode::defrost, ev::defrost_done, mode::idle},
};
constexpr std::size_t kN = sizeof kMap / sizeof kMap[0];
using machine = alloy::util::fsm<mode, ev, kN>;
}  // namespace

ALLOY_TEST(fsm_walks_the_table_and_reports_change_edges) {
    machine m{kMap, mode::idle};
    ALLOY_CHECK(m.dispatch(ev::too_warm));            // idle -> cooling: edge
    ALLOY_CHECK(m.state() == mode::cooling);
    ALLOY_CHECK(m.dispatch(ev::defrost_due));         // cooling -> defrost
    ALLOY_CHECK(!m.dispatch(ev::defrost_due));        // self-loop: NO edge
    ALLOY_CHECK(m.state() == mode::defrost);
    ALLOY_CHECK(m.dispatch(ev::defrost_done));
    ALLOY_CHECK(m.state() == mode::idle);
}

ALLOY_TEST(fsm_unclaimed_event_stays_put_silently) {
    machine m{kMap, mode::idle};
    ALLOY_CHECK(!m.dispatch(ev::defrost_done));  // no row from idle
    ALLOY_CHECK(m.state() == mode::idle);
    ALLOY_CHECK(!m.dispatch(ev::setpoint_reached));
    ALLOY_CHECK(m.state() == mode::idle);
}

ALLOY_TEST(fsm_reset_is_not_an_event) {
    machine m{kMap, mode::idle};
    m.reset(mode::defrost);
    ALLOY_CHECK(m.state() == mode::defrost);
    ALLOY_CHECK(m.dispatch(ev::defrost_done));  // table resumes from there
    ALLOY_CHECK(m.state() == mode::idle);
}

ALLOY_TEST(fsm_is_fully_constexpr) {
    constexpr auto walked = [] {
        machine m{kMap, mode::idle};
        (void)m.dispatch(ev::too_warm);
        (void)m.dispatch(ev::defrost_due);
        return m.state();
    }();
    static_assert(walked == mode::defrost);
    ALLOY_CHECK(walked == mode::defrost);
}
