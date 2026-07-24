// Unit tests for the PI controller — the proportional+integral math and, the
// part the original lacked, back-calculation anti-windup.

#include "alloy/control/pi.hpp"
#include "alloy_test.hpp"

using namespace alloy;

ALLOY_TEST(pi_proportional_plus_integral) {
    // kp=2, ki=10, dt=0.1 -> ki*dt = 1.0 per step. Wide clamp (no saturation).
    control::pi_controller<float> pi{2.0f, 10.0f, 0.1f, -100.0f, 100.0f};

    const float o1 = pi.update(1.0f, 0.0f);  // e=1: integral=1, out = 2 + 1
    ALLOY_CHECK(o1 > 2.9f && o1 < 3.1f);

    const float o2 = pi.update(1.0f, 0.0f);  // integral=2, out = 2 + 2
    ALLOY_CHECK(o2 > 3.9f && o2 < 4.1f);

    const float o3 = pi.update(0.0f, 0.0f);  // e=0: integral held at 2, out = 0 + 2
    ALLOY_CHECK(o3 > 1.9f && o3 < 2.1f);
}

ALLOY_TEST(pi_anti_windup_bounds_integral) {
    // Pure integral, output clamped to [-1, 1]. A large persistent error would
    // wind the integrator up unboundedly WITHOUT anti-windup; with it, the
    // integrator is held right at the clamp.
    control::pi_controller<float> pi{0.0f, 5.0f, 0.1f, -1.0f, 1.0f};
    for (int i = 0; i < 1000; ++i) {
        (void)pi.update(100.0f, 0.0f);  // saturated high the whole time
    }
    ALLOY_CHECK(pi.integrator() <= 1.001f);
    ALLOY_CHECK(pi.integrator() >= 0.999f);

    // Because the integrator never ran away, reversing the error recovers on
    // the very next step (no long unwinding delay).
    const float out = pi.update(-100.0f, 0.0f);
    ALLOY_CHECK(out < 0.0f);
}

ALLOY_TEST(pi_reset_clears_integrator) {
    control::pi_controller<float> pi{1.0f, 1.0f, 1.0f, -10.0f, 10.0f};
    (void)pi.update(5.0f, 0.0f);
    ALLOY_CHECK(pi.integrator() != 0.0f);
    pi.reset();
    ALLOY_CHECK(pi.integrator() == 0.0f);
}
