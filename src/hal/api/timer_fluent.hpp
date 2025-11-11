/**
 * @file timer_fluent.hpp
 * @brief Level 2 Fluent API for Timer
 * @note Part of Phase 6.5: Timer Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/interface/timer.hpp"
#include "hal/timer_simple.hpp"

namespace alloy::hal {

using namespace alloy::core;
using namespace alloy::hal::signals;

struct TimerBuilderState {
    bool has_mode = false;
    bool has_period = false;
    
    constexpr bool is_valid() const { 
        return has_mode; 
    }
};

struct FluentTimerConfig {
    PeripheralId peripheral;
    TimerConfig config;
    
    Result<void, ErrorCode> apply() const { 
        return Ok(); 
    }
};

template <PeripheralId PeriphId>
class TimerBuilder {
public:
    constexpr TimerBuilder()
        : mode_(TimerDefaults::mode),
          period_us_(TimerDefaults::period_us),
          prescaler_(TimerDefaults::prescaler),
          capture_edge_(CaptureEdge::Rising),
          compare_value_(0),
          state_() {}

    constexpr TimerBuilder& periodic() {
        mode_ = TimerMode::Periodic;
        state_.has_mode = true;
        return *this;
    }

    constexpr TimerBuilder& one_shot() {
        mode_ = TimerMode::OneShot;
        state_.has_mode = true;
        return *this;
    }

    constexpr TimerBuilder& counter() {
        mode_ = TimerMode::Counter;
        state_.has_mode = true;
        return *this;
    }

    constexpr TimerBuilder& input_capture() {
        mode_ = TimerMode::InputCapture;
        state_.has_mode = true;
        return *this;
    }

    constexpr TimerBuilder& output_compare() {
        mode_ = TimerMode::OutputCompare;
        state_.has_mode = true;
        return *this;
    }

    constexpr TimerBuilder& period(u32 period_us) {
        period_us_ = period_us;
        state_.has_period = true;
        return *this;
    }

    constexpr TimerBuilder& ms(u32 milliseconds) {
        period_us_ = milliseconds * 1000;
        state_.has_period = true;
        return *this;
    }

    constexpr TimerBuilder& us(u32 microseconds) {
        period_us_ = microseconds;
        state_.has_period = true;
        return *this;
    }

    constexpr TimerBuilder& prescaler(u32 presc) {
        prescaler_ = presc;
        return *this;
    }

    constexpr TimerBuilder& capture_rising() {
        capture_edge_ = CaptureEdge::Rising;
        return *this;
    }

    constexpr TimerBuilder& capture_falling() {
        capture_edge_ = CaptureEdge::Falling;
        return *this;
    }

    constexpr TimerBuilder& capture_both() {
        capture_edge_ = CaptureEdge::Both;
        return *this;
    }

    constexpr TimerBuilder& compare(u32 value) {
        compare_value_ = value;
        return *this;
    }

    Result<FluentTimerConfig, ErrorCode> initialize() const {
        if (!state_.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        FluentTimerConfig config{
            PeriphId,
            TimerConfig{mode_, period_us_, prescaler_, capture_edge_, compare_value_}
        };

        return Ok(std::move(config));
    }

private:
    TimerMode mode_;
    u32 period_us_;
    u32 prescaler_;
    CaptureEdge capture_edge_;
    u32 compare_value_;
    TimerBuilderState state_;
};

}  // namespace alloy::hal
