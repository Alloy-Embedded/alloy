// AC metering — RMS, real power, power factor and energy, from raw samples.
//
// Feed it a voltage and a current sample per tick. At the end of a window it
// gives you Vrms, Irms, real power, apparent power, power factor and the energy
// accumulated so far. Single pass, integer accumulators, NO BUFFERS.
//
// THE ONE THING WORTH KNOWING. Real power is the MEAN OF THE INSTANTANEOUS
// PRODUCT — sum(v[i] * i[i]) / n — and not Vrms * Irms. The two agree only for
// a purely resistive load at unity power factor. Every reactive load, every
// phase-controlled dimmer, every switch-mode supply makes them differ, and the
// version that multiplies the RMS values is the one most implementations ship.
// Apparent power IS Vrms * Irms; the ratio of the two is the power factor,
// which is the whole reason both are computed.
//
// PROVENANCE, and why this is a rewrite rather than a port. The outline comes
// from a grid-tied inverter's metering module, which had the algorithm right
// and four defects that made it unusable:
//
//   * two `int[512]` scratch arrays on the STACK — 4 KB, on a part with a few
//     tens of KB of RAM, to compute sums that need no storage at all;
//   * a divide by an apparent power that had already been floored to zero, so
//     a no-load reading produced NaN every cycle;
//   * the ISR-called write cursor held in a template-member static, hence
//     SHARED between instances — a second meter silently corrupted the first;
//   * the power sum truncated to int32 before integrating, so any load under
//     about a watt accumulated to exactly zero forever.
//
// So: accumulators are int64 and there are no arrays; apparent power at or near
// zero yields an EMPTY reading rather than a NaN; all state is per-object; and
// energy accumulates in int64 microjoules, which at a kilowatt overflows after
// roughly three hundred thousand years.
//
// Fixed-point in, float out. The samples are whatever your ADC gives you and
// the sums stay integer, so the hot path has no floating point; the conversion
// to volts and amps happens once per window, in the report.
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace alloy::lib::meter {

/// How raw counts become volts and amps.
///
/// `offset` is the count that represents zero — the mid-scale bias a
/// single-supply front end uses to measure a bipolar signal. Getting it wrong
/// is the classic metering bug: the DC term it leaves behind inflates Vrms and
/// makes real power drift with no load at all.
struct scaling {
    double volts_per_count = 1.0;
    double amps_per_count = 1.0;
    std::int32_t voltage_offset = 0;
    std::int32_t current_offset = 0;

    [[nodiscard]] constexpr bool valid() const {
        return volts_per_count > 0.0 && amps_per_count > 0.0;
    }
};

/// One window's worth of measurement.
struct reading {
    double vrms = 0.0;
    double irms = 0.0;
    /// Mean of v*i over the window. Signed: negative means the load is
    /// EXPORTING, which for an inverter is the normal direction and not an
    /// error to be abs()'d away.
    double real_power_w = 0.0;
    double apparent_power_va = 0.0;
    /// real / apparent, in [-1, 1]. Empty when apparent power is too small to
    /// divide by — see `accumulator::minimum_apparent_va`.
    std::optional<double> power_factor{};
    /// Mean voltage and current over the window. A healthy AC signal has both
    /// near zero; a persistent offset means the bias is wrong, so they are
    /// reported rather than silently subtracted.
    double dc_voltage = 0.0;
    double dc_current = 0.0;
    std::size_t samples = 0;
};

/// Single-pass accumulators over a measurement window.
///
/// `update` is what an ISR calls. It does five multiply-accumulates into int64
/// and nothing else: no division, no floating point, no branch on sample value,
/// no memory beyond the object.
class accumulator {
public:
    constexpr explicit accumulator(scaling scale) : scale_(scale) {}

    /// Below this apparent power a reading reports no power factor rather than
    /// a number produced by dividing by nearly nothing. The original divided
    /// unconditionally by a value it had floored to zero, so an unloaded meter
    /// produced NaN on every cycle.
    static constexpr double minimum_apparent_va = 1e-9;

    /// One sample pair, in raw counts.
    constexpr void update(std::int32_t voltage_count, std::int32_t current_count) {
        const std::int64_t v = voltage_count - scale_.voltage_offset;
        const std::int64_t i = current_count - scale_.current_offset;
        sum_v_ += v;
        sum_i_ += i;
        sum_vv_ += v * v;
        sum_ii_ += i * i;
        sum_vi_ += v * i;
        ++samples_;
    }

    [[nodiscard]] constexpr std::size_t samples() const { return samples_; }

    /// The window's result. Does NOT reset — a caller may want to read a
    /// running window without ending it, and a report that silently cleared
    /// the state would make that impossible to express.
    [[nodiscard]] reading report() const {
        reading out{};
        if (samples_ == 0) {
            return out;
        }
        const auto n = static_cast<double>(samples_);
        const double vk = scale_.volts_per_count;
        const double ik = scale_.amps_per_count;

        out.samples = samples_;
        out.dc_voltage = (static_cast<double>(sum_v_) / n) * vk;
        out.dc_current = (static_cast<double>(sum_i_) / n) * ik;
        out.vrms = std::sqrt(static_cast<double>(sum_vv_) / n) * vk;
        out.irms = std::sqrt(static_cast<double>(sum_ii_) / n) * ik;
        // THE POINT: mean of the product, not product of the means.
        out.real_power_w = (static_cast<double>(sum_vi_) / n) * vk * ik;
        out.apparent_power_va = out.vrms * out.irms;
        if (out.apparent_power_va > minimum_apparent_va) {
            out.power_factor = out.real_power_w / out.apparent_power_va;
        }
        return out;
    }

    /// Fold this window into the energy total and start the next one.
    ///
    /// `window_seconds` is how long the window covered. Energy accumulates in
    /// int64 MICROJOULES: a watt-second is a million of them, so a kilowatt
    /// runs for about three hundred thousand years before overflow. The
    /// original truncated its power sum to int32 first, which made every load
    /// under roughly a watt integrate to exactly zero — forever.
    void close_window(double window_seconds) {
        if (samples_ != 0 && window_seconds > 0.0) {
            const double joules = report().real_power_w * window_seconds;
            energy_uj_ += static_cast<std::int64_t>(joules * 1e6);
        }
        reset();
    }

    /// Energy since construction, in microjoules. Signed, so an exporting
    /// source drives it negative rather than wrapping.
    [[nodiscard]] constexpr std::int64_t energy_microjoules() const { return energy_uj_; }
    [[nodiscard]] double energy_wh() const {
        return static_cast<double>(energy_uj_) / 1e6 / 3600.0;
    }

    /// Clear the window. Leaves the energy total alone — that is the whole
    /// distinction between a window and a total.
    constexpr void reset() {
        sum_v_ = sum_i_ = sum_vv_ = sum_ii_ = sum_vi_ = 0;
        samples_ = 0;
    }

    /// Clear everything, energy included.
    constexpr void reset_all() {
        reset();
        energy_uj_ = 0;
    }

    /// Start with a known energy — restoring a total from non-volatile memory
    /// after a power cycle, which is the normal thing a meter has to do.
    constexpr void seed_energy(std::int64_t microjoules) { energy_uj_ = microjoules; }

    [[nodiscard]] constexpr const scaling& scale() const { return scale_; }

private:
    scaling scale_;
    std::int64_t sum_v_{0}, sum_i_{0};
    std::int64_t sum_vv_{0}, sum_ii_{0}, sum_vi_{0};
    std::int64_t energy_uj_{0};
    std::size_t samples_{0};
};

}  // namespace alloy::lib::meter
