/// ESP32/Xtensa SysTick Integration with RTOS (Bare-Metal)
///
/// Integrates ESP32 hardware timer with RTOS scheduler.
/// Uses Timer Group 0, Timer 0 for periodic tick generation.
///
/// Note: This is a bare-metal implementation without ESP-IDF dependencies.

#ifdef ESP32

#include "rtos/rtos.hpp"
#include "rtos/platform/xtensa_context.hpp"
#include "core/types.hpp"

namespace alloy::rtos {

namespace xtensa {

// ESP32 Timer Group 0 registers
constexpr core::u32 TIMG0_BASE = 0x3FF5F000;
constexpr core::u32 TIMG0_T0CONFIG_REG = TIMG0_BASE + 0x00;
constexpr core::u32 TIMG0_T0LO_REG = TIMG0_BASE + 0x04;
constexpr core::u32 TIMG0_T0HI_REG = TIMG0_BASE + 0x08;
constexpr core::u32 TIMG0_T0UPDATE_REG = TIMG0_BASE + 0x0C;
constexpr core::u32 TIMG0_T0ALARMLO_REG = TIMG0_BASE + 0x10;
constexpr core::u32 TIMG0_T0ALARMHI_REG = TIMG0_BASE + 0x14;
constexpr core::u32 TIMG0_T0LOADLO_REG = TIMG0_BASE + 0x18;
constexpr core::u32 TIMG0_T0LOADHI_REG = TIMG0_BASE + 0x1C;
constexpr core::u32 TIMG0_T0LOAD_REG = TIMG0_BASE + 0x20;
constexpr core::u32 TIMG0_INT_ENA_REG = TIMG0_BASE + 0x98;
constexpr core::u32 TIMG0_INT_CLR_REG = TIMG0_BASE + 0xA4;

// Timer config bits
constexpr core::u32 TIMG_T0_EN = (1 << 31);
constexpr core::u32 TIMG_T0_INCREASE = (1 << 30);
constexpr core::u32 TIMG_T0_AUTORELOAD = (1 << 29);
constexpr core::u32 TIMG_T0_DIVIDER_SHIFT = 13;
constexpr core::u32 TIMG_T0_EDGE_INT_EN = (1 << 12);
constexpr core::u32 TIMG_T0_ALARM_EN = (1 << 10);

// Interrupt enable bit
constexpr core::u32 TIMG_T0_INT_ENA = (1 << 0);

// Helper to write timer registers
inline void write_timer_reg(core::u32 addr, core::u32 value) {
    *reinterpret_cast<volatile core::u32*>(addr) = value;
}

inline core::u32 read_timer_reg(core::u32 addr) {
    return *reinterpret_cast<volatile core::u32*>(addr);
}

// Global timer state
static bool g_timer_initialized = false;

void init_rtos_timer() {
    if (g_timer_initialized) return;

    // ESP32 APB clock is typically 80MHz
    // We want 1ms tick, so we need to count to 80,000 cycles
    // Using divider of 80 gives us 1MHz clock, then count to 1000 for 1ms
    constexpr core::u32 TIMER_DIVIDER = 80;
    constexpr core::u32 TIMER_ALARM_VALUE = 1000; // 1ms at 1MHz

    // Disable timer first
    write_timer_reg(TIMG0_T0CONFIG_REG, 0);

    // Configure timer:
    // - Enable timer
    // - Count up
    // - Auto-reload on alarm
    // - Set divider to 80 (80MHz / 80 = 1MHz)
    // - Enable alarm
    // - Enable edge interrupt
    core::u32 config = TIMG_T0_EN | TIMG_T0_INCREASE | TIMG_T0_AUTORELOAD |
                       (TIMER_DIVIDER << TIMG_T0_DIVIDER_SHIFT) |
                       TIMG_T0_ALARM_EN | TIMG_T0_EDGE_INT_EN;

    write_timer_reg(TIMG0_T0CONFIG_REG, config);

    // Set alarm value (1ms)
    write_timer_reg(TIMG0_T0ALARMLO_REG, TIMER_ALARM_VALUE);
    write_timer_reg(TIMG0_T0ALARMHI_REG, 0);

    // Set initial counter value to 0
    write_timer_reg(TIMG0_T0LOADLO_REG, 0);
    write_timer_reg(TIMG0_T0LOADHI_REG, 0);
    write_timer_reg(TIMG0_T0LOAD_REG, 1); // Trigger reload

    // Enable timer interrupt
    write_timer_reg(TIMG0_INT_ENA_REG, TIMG_T0_INT_ENA);

    // TODO: Register interrupt handler with ESP32 interrupt controller
    // This would normally use xt_set_interrupt_handler() or similar
    // For now, this is a placeholder - full implementation would need
    // ESP32 interrupt controller setup

    g_timer_initialized = true;
}

void stop_rtos_timer() {
    if (g_timer_initialized) {
        // Disable timer interrupt
        write_timer_reg(TIMG0_INT_ENA_REG, 0);

        // Disable timer
        write_timer_reg(TIMG0_T0CONFIG_REG, 0);

        g_timer_initialized = false;
    }
}

} // namespace xtensa

} // namespace alloy::rtos

// Timer ISR handler - called every 1ms
extern "C" void timer_isr_handler(void* arg) {
    (void)arg; // Unused

    // Clear interrupt
    alloy::rtos::xtensa::write_timer_reg(
        alloy::rtos::xtensa::TIMG0_INT_CLR_REG,
        alloy::rtos::xtensa::TIMG_T0_INT_ENA
    );

    // Call RTOS scheduler tick
    alloy::rtos::RTOS::tick();

    // Trigger context switch if needed
    if (alloy::rtos::RTOS::need_context_switch()) {
        alloy::rtos::trigger_context_switch();
    }
}

#endif // ESP32
