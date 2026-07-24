// PI controller with back-calculation anti-windup and an output clamp — the
// control building block that sits on top of ADC (measure) and PWM (actuate):
// motor current, thermal, power loops. Templated on the scalar so a fixed-point
// type can replace float on FPU-less parts (Cortex-M0+, where soft-float is
// costly under -Os); default float. The anti-windup — bleeding the saturated
// excess back out of the integrator so it cannot keep growing while the output
// is clamped — is exactly what the original startup-code PI lacked.

#pragma once

namespace alloy::control {

template <class T = float>
class pi_controller {
    T kp_{};
    T ki_dt_{};  // ki * dt, folded once at construction
    T out_min_{};
    T out_max_{};
    T integral_{};

public:
    constexpr pi_controller() = default;

    // dt is the fixed loop period (in the same time unit as ki). out_min/out_max
    // clamp the returned command.
    constexpr pi_controller(T kp, T ki, T dt, T out_min, T out_max)
        : kp_(kp), ki_dt_(ki * dt), out_min_(out_min), out_max_(out_max) {}

    // One control step: returns the clamped actuator command.
    T update(T setpoint, T measured) {
        const T e = setpoint - measured;
        integral_ += ki_dt_ * e;
        T out = kp_ * e + integral_;
        // Anti-windup: clamp, then remove the clamped excess from the
        // integrator so it does not wind up further while saturated.
        if (out > out_max_) {
            integral_ -= (out - out_max_);
            out = out_max_;
        } else if (out < out_min_) {
            integral_ += (out_min_ - out);
            out = out_min_;
        }
        return out;
    }

    void reset() { integral_ = T{}; }
    [[nodiscard]] T integrator() const { return integral_; }
};

}  // namespace alloy::control
