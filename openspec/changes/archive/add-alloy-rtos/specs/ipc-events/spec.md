# Spec: Event Flags

## ADDED Requirements

### Requirement: 32-bit Event Flags
**ID**: RTOS-EVENT-001
**Priority**: P0 (Critical)

The system SHALL provide 32-bit event flags for lightweight task notification.

#### Scenario: Wait for multiple events
```cpp
EventFlags events;

constexpr uint32_t BUTTON = (1 << 0);
constexpr uint32_t TIMER  = (1 << 1);
constexpr uint32_t DATA   = (1 << 2);

void event_task() {
    while(1) {
        // Wait for any event
        uint32_t flags = events.wait_any(BUTTON | TIMER | DATA);

        if (flags & BUTTON) handle_button();
        if (flags & TIMER) handle_timer();
        if (flags & DATA) handle_data();
    }
}

void button_isr() {
    events.set(BUTTON);  // Set from ISR
}
```

---

### Requirement: Wait ANY or ALL
**ID**: RTOS-EVENT-002
**Priority**: P1 (High)

The system SHALL support waiting for ANY or ALL specified event flags.

#### Scenario: Wait for all conditions
```cpp
EventFlags startup;

constexpr uint32_t SENSOR_READY = (1 << 0);
constexpr uint32_t WIFI_READY   = (1 << 1);
constexpr uint32_t TIME_SYNCED  = (1 << 2);

void main_task() {
    // Wait for all startup conditions
    bool ready = startup.wait_all(
        SENSOR_READY | WIFI_READY | TIME_SYNCED,
        5000  // 5 second timeout
    );

    if (ready) {
        start_application();
    }
}
```

## MODIFIED Requirements
None.

## REMOVED Requirements
None.
