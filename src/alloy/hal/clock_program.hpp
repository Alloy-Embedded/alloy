// Interpreter for data-driven clock programs.
//
// The program (an array of clock_step) is a generated FACT; this interpreter
// is the hand-written BEHAVIOR. Poll timeouts run before any timebase exists,
// so they are approximated in busy-loop iterations — coarse by design, they
// only bound catastrophic hangs (crystal absent, PLL never locking).

#pragma once

#include <cstddef>
#include <span>

#include "alloy/core/types.hpp"

namespace alloy::hal {

// Returns true if every step succeeded, false on a poll timeout.
// On timeout the program stops where it is; the caller decides policy
// (the generated board::init() falls back to the reset clock and continues,
// so a board without its crystal still boots and blinks).
inline bool run_clock_program(std::span<const clock_step> program) {
    for (const auto& step : program) {
        switch (step.kind) {
            case clock_step::op::write:
                *reinterpret_cast<rw32*>(step.addr) = step.value;
                break;
            case clock_step::op::rmw: {
                auto& reg = *reinterpret_cast<rw32*>(step.addr);
                reg = (reg & ~step.mask) | step.value;
                break;
            }
            case clock_step::op::poll: {
                auto& reg = *reinterpret_cast<rw32*>(step.addr);
                // ~16 iterations/us at 16 MHz boot clock; deliberately coarse.
                std::uint32_t budget = step.timeout_us * 16u;
                while ((reg & step.mask) != step.value) {
                    if (budget-- == 0u) {
                        return false;
                    }
                }
                break;
            }
            case clock_step::op::delay:
                for (std::uint32_t i = 0; i < step.value * 16u; ++i) {
                    __asm volatile("" ::: "memory");  // keep the loop
                }
                break;
        }
    }
    return true;
}

}  // namespace alloy::hal
