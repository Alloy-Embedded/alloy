// Grid synchronisation — a SOGI-PLL that locks to a single-phase AC voltage.
//
// Anything that pushes current into a grid has to know where the grid's voltage
// is, right now, to a fraction of a degree: phase, amplitude and frequency, from
// one noisy scalar measurement. That is what this does.
//
// HOW IT WORKS, in the order the code runs.
//
//   A SECOND-ORDER GENERALISED INTEGRATOR takes the measured voltage and
//   produces two outputs: `alpha`, a filtered copy of the input, and `beta`, the
//   same signal shifted ninety degrees. One real measurement becomes the
//   orthogonal pair a phase detector needs, and the filtering is a resonant
//   band-pass centred on the grid frequency — so harmonics and noise are
//   attenuated for free rather than by a separate filter.
//
//   A PARK TRANSFORM rotates (alpha, beta) by the PLL's current phase estimate,
//   giving `d` and `q`. When the estimate is correct, `q` is zero and `d` is the
//   grid amplitude. So `q` IS the phase error, already in the right units.
//
//   A PI CONTROLLER drives `q` to zero by adjusting the estimated frequency,
//   which integrates into the phase. That is the loop.
//
// WHAT WAS KEPT AND WHAT WAS DISCARDED. The algorithm and one genuinely
// non-obvious trick come from a grid-tied inverter's PLL. The trick is
// `phase_advanced()`: a controller usually needs the phase one sample AHEAD, to
// cancel its own computation delay, and the obvious way to get it costs a third
// sine and cosine. Rotating the existing pair by a CONSTANT angle costs two
// multiplies and two adds instead — the constant is known at compile time
// because the sample period is. That file recorded the ISR cost before and
// after the change, in a comment, and that habit is worth more than the trick.
//
// Discarded: it ran in double precision by accident (`pow(x, 2)`, unqualified
// libm `sin`/`cos`, a bare `1.4142` literal) on a part whose FPU does single
// precision in one cycle and double in software. It hardcoded the grid peak as
// `311`, which is 220 V RMS and nothing else. And its constants were computed
// with a namespace-scope `constexpr float k = cos(x)` — which is not standard
// C++ at all, only a GCC builtin fold, so it would fail to compile on another
// toolchain. Here `T` is the caller's float type, the grid peak is a parameter,
// and the rotation constants are computed in the constructor.
//
// No allocation, no exceptions, one `std::sin`/`std::cos` pair per update.
#pragma once

#include <cmath>
#include <cstdint>

namespace alloy::lib::pll {

/// Everything about the grid and the loop, in physical units.
template <class T = float>
struct config {
    /// Nominal grid frequency in hertz — 50 or 60. The SOGI resonates here and
    /// the loop starts here, so a wrong value costs lock time, not accuracy.
    T nominal_hz = T(60);
    /// Control period in seconds: 1 / sample rate.
    T ts = T(1) / T(10'000);
    /// SOGI damping. sqrt(2) is the standard flat-response choice; lower is
    /// more selective and slower, higher is faster and lets more through.
    T sogi_k = T(1.4142135623730951);
    /// PI gains on the phase error. `ki` is what removes steady-state phase
    /// error; `kp` sets how fast it converges.
    T kp = T(180);
    T ki = T(3200);
    /// How far the estimate may wander from nominal, in hertz. A PLL with no
    /// limit will happily lock to a harmonic, or run away on a disconnected
    /// input where there is nothing to lock to.
    T freq_limit_hz = T(10);
    /// Nominal grid PEAK voltage, in whatever unit the samples are in. Only
    /// used to normalise the phase error so the gains mean the same thing at
    /// any grid voltage — NOT hardcoded to one market's mains.
    T nominal_peak = T(311);

    [[nodiscard]] constexpr bool valid() const {
        return nominal_hz > T(0) && ts > T(0) && sogi_k > T(0) &&
               nominal_peak > T(0) && freq_limit_hz >= T(0) &&
               // Nyquist, with room: a PLL sampled near twice the grid
               // frequency is not a PLL.
               ts * nominal_hz < T(0.1);
    }
};

/// Locks to a single-phase grid voltage.
///
/// `update(v)` once per control period, then read `phase()`, `frequency_hz()`
/// and `amplitude()`. `locked()` says whether to believe them yet.
template <class T = float>
class sogi_pll {
public:
    static constexpr T two_pi = T(6.283185307179586);

    constexpr explicit sogi_pll(config<T> cfg)
        : cfg_(cfg),
          omega_(two_pi * cfg.nominal_hz),
          mean_omega_(two_pi * cfg.nominal_hz),
          // The advance rotation is by exactly one sample of nominal phase.
          // Computed here, in a constructor, rather than as a namespace-scope
          // constexpr call to std::cos — which is not constexpr in any
          // published standard and compiles only where the compiler folds it
          // as a builtin.
          cos_step_(std::cos(two_pi * cfg.nominal_hz * cfg.ts)),
          sin_step_(std::sin(two_pi * cfg.nominal_hz * cfg.ts)) {}

    /// One sample of grid voltage.
    void update(T v) {
        // ── SOGI: one measurement in, an orthogonal pair out.
        const T error = v - alpha_;
        const T d_alpha = (cfg_.sogi_k * error - beta_) * omega_;
        const T d_beta = alpha_ * omega_;
        alpha_ += d_alpha * cfg_.ts;
        beta_ += d_beta * cfg_.ts;

        // ── Park: rotate by the current estimate. q is the phase error.
        const T s = std::sin(phase_);
        const T c = std::cos(phase_);
        d_ = alpha_ * c + beta_ * s;
        q_ = -alpha_ * s + beta_ * c;

        // Normalise by the nominal peak so kp/ki mean the same thing on a
        // 110 V grid and a 400 V one. x*x, not pow(x, 2): the latter is a
        // library call that also promotes to double.
        const T phase_error = q_ / cfg_.nominal_peak;

        // ── PI on the phase error, integrating into frequency.
        integral_ += phase_error * cfg_.ki * cfg_.ts;
        const T limit = two_pi * cfg_.freq_limit_hz;
        if (integral_ > limit) {
            integral_ = limit;
        } else if (integral_ < -limit) {
            integral_ = -limit;
        }
        T omega = two_pi * cfg_.nominal_hz + cfg_.kp * phase_error + integral_;
        // Clamp the FREQUENCY too, not just the integrator: a large transient
        // reaches the same runaway through the proportional term alone.
        const T omega_max = two_pi * (cfg_.nominal_hz + cfg_.freq_limit_hz);
        const T omega_min = two_pi * (cfg_.nominal_hz - cfg_.freq_limit_hz);
        omega_ = omega > omega_max ? omega_max : (omega < omega_min ? omega_min : omega);

        // ── integrate frequency into phase, wrapped to [0, 2pi).
        phase_ += omega_ * cfg_.ts;
        while (phase_ >= two_pi) {
            phase_ -= two_pi;
        }
        while (phase_ < T(0)) {
            phase_ += two_pi;
        }

        // THE SECOND-HARMONIC RIPPLE, and why frequency is filtered.
        //
        // A single-phase PLL has no second real measurement, so the orthogonal
        // pair the SOGI synthesises carries a component at twice the grid
        // frequency. It lands on q, hence on the frequency estimate: the
        // instantaneous value oscillates by a few tenths of a hertz around the
        // truth even when the loop is tracking perfectly. That is inherent to
        // the topology, not a tuning fault.
        //
        // So the reported frequency is filtered over about a cycle. A caller
        // deciding a grid-code trip needs the average, not a sample of the
        // ripple; `frequency_instant_hz()` is still there for a controller that
        // wants the raw loop output.
        mean_omega_ += (omega_ - mean_omega_) * cfg_.ts * cfg_.nominal_hz;

        // Amplitude is the d axis once locked. Slew it so a transient does not
        // make the reported amplitude jump before the loop has caught up.
        amplitude_ += (d_ - amplitude_) * cfg_.ts * cfg_.nominal_hz;

        // Lock is decided on a FILTERED phase error, not an instantaneous one
        // and not a run of consecutive good samples.
        //
        // Both of the obvious alternatives are wrong. A single sample is
        // meaningless — q crosses zero twice a cycle whether locked or not. A
        // run counter is worse in the case that matters: one noisy sample
        // resets it to zero, so a PLL that is tracking a real grid perfectly
        // well reports unlocked whenever the grid is dirty, which is exactly
        // when you need the answer. The first version of this file used a run
        // counter and failed its own noise test.
        //
        // A one-pole average over roughly a cycle, with separate thresholds so
        // the flag does not chatter on the boundary.
        const T alpha = cfg_.ts * cfg_.nominal_hz;  // tau ~ one grid cycle
        mean_error_ += (std::fabs(phase_error) - mean_error_) * alpha;
        if (locked_) {
            locked_ = mean_error_ < kUnlockError;
        } else {
            locked_ = mean_error_ < kLockError;
        }
    }

    /// Estimated grid phase in radians, [0, 2pi).
    [[nodiscard]] constexpr T phase() const { return phase_; }

    /// The phase ONE SAMPLE AHEAD, for a controller that must cancel its own
    /// computation delay.
    ///
    /// A rotation by a constant angle: two multiplies and two adds, versus a
    /// third sin/cos pair. The angle is one nominal sample, so this is exact at
    /// nominal frequency and off by the frequency deviation elsewhere — which
    /// at a tenth of a hertz on sixty is parts per thousand of a sample.
    [[nodiscard]] T phase_advanced() const {
        const T s = std::sin(phase_) * cos_step_ + std::cos(phase_) * sin_step_;
        const T c = std::cos(phase_) * cos_step_ - std::sin(phase_) * sin_step_;
        T advanced = std::atan2(s, c);
        if (advanced < T(0)) {
            advanced += two_pi;
        }
        return advanced;
    }

    /// The orthogonal pair the SOGI produces: the filtered input and its
    /// quadrature. Useful on their own for a current controller.
    [[nodiscard]] constexpr T alpha() const { return alpha_; }
    [[nodiscard]] constexpr T beta() const { return beta_; }
    [[nodiscard]] constexpr T d() const { return d_; }
    [[nodiscard]] constexpr T q() const { return q_; }

    /// Grid frequency in hertz, averaged over about a cycle to remove the
    /// second-harmonic ripple inherent to a single-phase PLL. This is the
    /// number to compare against a grid-code window.
    [[nodiscard]] constexpr T frequency_hz() const { return mean_omega_ / two_pi; }

    /// The loop's raw frequency, ripple and all — for a controller that wants
    /// the instantaneous value rather than a decision-grade one.
    [[nodiscard]] constexpr T frequency_instant_hz() const { return omega_ / two_pi; }
    [[nodiscard]] constexpr T amplitude() const { return amplitude_; }
    [[nodiscard]] constexpr bool locked() const { return locked_; }
    /// The filtered phase error the lock decision is made on, in radians —
    /// exposed because "how well locked" is a more useful number than a bool
    /// when something is going wrong.
    [[nodiscard]] constexpr T mean_phase_error() const { return mean_error_; }

    /// Back to nominal, unlocked, integrator cleared.
    constexpr void reset() {
        alpha_ = beta_ = d_ = q_ = integral_ = amplitude_ = T(0);
        phase_ = T(0);
        omega_ = mean_omega_ = two_pi * cfg_.nominal_hz;
        mean_error_ = T(1);
        locked_ = false;
    }

    [[nodiscard]] constexpr const config<T>& settings() const { return cfg_; }

private:
    /// Filtered phase error to declare lock, and the wider one to give it up.
    /// Separate values so the flag does not chatter at the boundary — about
    /// half a degree in, one and a half degrees out.
    static constexpr T kLockError = T(0.01);
    static constexpr T kUnlockError = T(0.03);

    config<T> cfg_;
    T omega_;
    T mean_omega_;
    T cos_step_, sin_step_;
    T alpha_{0}, beta_{0};
    T d_{0}, q_{0};
    T phase_{0}, integral_{0}, amplitude_{0};
    /// Starts at 1 rad — far above any lock threshold — so a freshly built PLL
    /// reports unlocked until it has actually seen a grid.
    T mean_error_{1};
    bool locked_{false};
};

}  // namespace alloy::lib::pll
