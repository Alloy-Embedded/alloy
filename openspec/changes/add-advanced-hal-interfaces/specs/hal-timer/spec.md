# Spec Delta: hal-timer

**Capability**: `hal-timer`
**Status**: NEW

## ADDED Requirements

### Requirement: Periodic Timer

The system SHALL support periodic timers with configurable interval.

#### Scenario: 1ms periodic callback
```cpp
TimerConfig config{
    .mode = TimerMode::Periodic,
    .period_us = 1000  // 1ms
};
timer.configure(config);
timer.set_callback([]() {
    // Called every 1ms
});
timer.start();
```

### Requirement: One-Shot Timer

The system SHALL support one-shot timers that fire once.

#### Scenario: Delayed action after 5 seconds
```cpp
TimerConfig config{
    .mode = TimerMode::OneShot,
    .period_us = 5000000  // 5s
};
timer.configure(config);
timer.set_callback([]() {
    // Called once after 5s
});
timer.start();
```

### Requirement: Counter Access

The system SHALL provide access to timer counter value.

#### Scenario: Measure elapsed time
```cpp
timer.start();
uint32_t start = timer.get_counter();
// ... do work ...
uint32_t end = timer.get_counter();
uint32_t elapsed_us = end - start;
```

### Requirement: Input Capture

The system SHALL support input capture for frequency/pulse width measurement.

#### Scenario: Measure PWM frequency
```cpp
TimerConfig config{
    .mode = TimerMode::InputCapture,
    .capture_edge = CaptureEdge::Rising
};
timer.configure(config);
uint32_t period = timer.get_captured_value();
```

### Requirement: Output Compare

The system SHALL support output compare for precise timing events.

#### Scenario: Toggle pin at exact time
```cpp
TimerConfig config{
    .mode = TimerMode::OutputCompare,
    .compare_value = 1000
};
timer.configure(config);
timer.start();
```

## Non-Functional Requirements

### Perf-TIMER-001: Callback Latency
Timer callbacks SHOULD execute within 10 CPU cycles of timer event.

### Safe-TIMER-001: ISR Context
Timer callbacks MUST execute in interrupt context and be documented as such.
