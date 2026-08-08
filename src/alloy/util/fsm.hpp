// Tabular finite state machine — the smallest FSM that is still a table.
//
// One rule, deliberately: dispatch scans the table top-down, the FIRST row
// matching (state, event) wins, and an event no row claims leaves the state
// unchanged. That first-match-wins order is part of the CONTRACT, not an
// implementation detail — tables written for engines with this rule use row
// order as priority, and porting such a table here must be a mechanical,
// diff-reviewable copy. No entry/exit hooks, no guards, no hierarchy: the
// machines this serves (thermostat/defrost/compressor-class control logic)
// keep their actions in the surrounding tick code, where they are testable
// as plain calls.
//
// Improvement over the classic uint8_t triple: State and Event are the
// caller's own enums, so feeding machine A's event to machine B is a type
// error instead of a silent wrong transition.

#pragma once

#include <cstddef>
#include <span>

namespace alloy::util {

template <class State, class Event>
struct transition {
    State from;
    Event event;
    State to;
};

template <class State, class Event, std::size_t N>
class fsm {
    static_assert(N >= 1, "a state machine with no transitions is a constant");

public:
    constexpr fsm(std::span<const transition<State, Event>, N> table, State initial)
        : table_(table), state_(initial) {}

    [[nodiscard]] constexpr State state() const { return state_; }

    // Feed one event. Returns true when the state CHANGED — the edge signal
    // tick code keys entry actions on (`if (m.dispatch(e)) { on_enter(); }`).
    constexpr bool dispatch(Event e) {
        for (const auto& t : table_) {
            if (t.from == state_ && t.event == e) {
                const bool changed = t.to != state_;
                state_ = t.to;
                return changed;
            }
        }
        return false;  // unclaimed event: stay, silently — the table's rule
    }

    // Force a state (init/reset paths). Not an event: no row is consulted.
    constexpr void reset(State s) { state_ = s; }

private:
    std::span<const transition<State, Event>, N> table_;
    State state_;
};

}  // namespace alloy::util
