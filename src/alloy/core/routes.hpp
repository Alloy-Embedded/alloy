// Pin-route contract.
//
// Codegen emits one `route` specialization per (pin, peripheral, signal)
// entry in the chip data. Hand-written code never defines routes; it only
// asks `routable` and reads the payload.
//
// NORTH_STAR guard #5: the STM32 alternate-function model is not universal,
// so every route carries a kind and a kind-specific payload:
//   af_fixed    -> ::af          (STM32/SAM style fixed mux table)
//   funcsel     -> ::funcsel     (RP2040)
//   full_matrix -> ::matrix_signal (ESP32 GPIO matrix; check is legality, not table)
//   psel        -> (no payload)  (nRF: any pin, config written to PSEL register)

#pragma once

#include <cstdint>

#include "alloy/core/types.hpp"

namespace alloy::routes {

enum class kind : std::uint8_t { af_fixed, funcsel, full_matrix, psel };

// Primary template intentionally undefined: no specialization = no route.
template <class Pin, class Periph, alloy::signal S>
struct route;

template <class Pin, class Periph, alloy::signal S>
concept routable = requires { route<Pin, Periph, S>::k; };

// Route payload -> mux value for the pin driver's make_af(). if-constexpr
// (not ?:) so only the payload the kind actually carries is instantiated.
// full_matrix routes hand their output-signal index to make_af (the ESP32
// pin driver writes it into FUNC_OUT_SEL); input-side matrix routing is
// the consuming driver's job.
template <class Route>
constexpr std::uint8_t mux_value() {
    static_assert(Route::k == kind::af_fixed || Route::k == kind::funcsel ||
                      Route::k == kind::full_matrix,
                  "route kind not implemented yet (psel pending)");
    if constexpr (Route::k == kind::af_fixed) {
        return Route::af;
    } else if constexpr (Route::k == kind::funcsel) {
        return Route::funcsel;
    } else {
        static_assert(Route::matrix_signal < 256, "matrix signal exceeds mux byte");
        return static_cast<std::uint8_t>(Route::matrix_signal);
    }
}

}  // namespace alloy::routes
