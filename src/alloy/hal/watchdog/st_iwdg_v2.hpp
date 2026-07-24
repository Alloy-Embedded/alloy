// Watchdog driver for the STM32 IWDG v2 (G0/G4/F7-newer/L4 — has the window
// register). Runtime-configured; shares its logic with v1 via st_iwdg_detail.

#pragma once

#include <chrono>
#include <concepts>

#include "alloy/hal/watchdog/st_iwdg_detail.hpp"
#include "alloy/hal/watchdog/wdt_impl.hpp"
#include "alloy/ip/st/iwdg_v2.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::iwdg_v2>
struct wdt_impl<Inst> {
    using IP = typename Inst::ip;
    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    static void start(std::chrono::milliseconds timeout) { detail::iwdg_configure(r(), timeout); }
    static void feed() { detail::iwdg_kick(r()); }
};

}  // namespace alloy::hal
