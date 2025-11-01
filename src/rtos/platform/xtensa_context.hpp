/// Xtensa (ESP32) Context Switching
///
/// Implements context switching for Xtensa architecture (ESP32).
///
/// Theory:
/// - Xtensa has windowed registers (a0-a15) with register windows
/// - Need to save/restore all registers including PS, PC, SAR
/// - Uses software interrupt (level-1) for context switching
/// - More complex than ARM due to register windows
///
/// Context switch time: ~10-20µs @ 240MHz
///
/// Stack layout (Xtensa exception frame):
/// ```
/// High memory
/// ┌─────────────┐
/// │     PC      │ ← Exception frame
/// │     PS      │
/// │    SAR      │
/// │    a0-a15   │ ← Window overflow area
/// ├─────────────┤
/// │  EXCCAUSE   │
/// │  EXCVADDR   │
/// └─────────────┘
/// Low memory
/// ```
///
/// Note: Xtensa register windows make this more complex than ARM.
/// We need to handle window overflow/underflow.

#ifndef ALLOY_RTOS_XTENSA_CONTEXT_HPP
#define ALLOY_RTOS_XTENSA_CONTEXT_HPP

#include "rtos/rtos.hpp"
#include "core/types.hpp"

#ifdef ESP32

namespace alloy::rtos {

/// Initialize task stack for Xtensa
///
/// Sets up initial stack frame as if the task was interrupted.
///
/// @param tcb Task control block
/// @param func Task entry point
void init_task_stack(TaskControlBlock* tcb, void (*func)());

/// Start first task (never returns)
///
/// Loads context of first task and starts executing it.
[[noreturn]] void start_first_task();

/// Trigger a context switch
///
/// Sets software interrupt to request a context switch.
void trigger_context_switch();

/// Context switch interrupt handler
///
/// Called when software interrupt is triggered.
/// Saves current context, loads next context.
extern "C" void context_switch_handler(void* arg);

/// Timer interrupt handler
///
/// Called every 1ms from ESP32 timer.
/// Calls RTOS scheduler tick.
extern "C" void IRAM_ATTR timer_isr_handler(void* arg);

// Xtensa-specific definitions
namespace xtensa {

/// Xtensa exception frame structure
///
/// This is the format of the stack after an exception/interrupt.
/// The hardware automatically saves this.
struct XtExceptionFrame {
    core::u32 pc;          ///< Program counter
    core::u32 ps;          ///< Processor status
    core::u32 a0;          ///< Return address
    core::u32 a1;          ///< Stack pointer
    core::u32 a2;          ///< First argument
    core::u32 a3;          ///< Second argument
    core::u32 a4;          ///< Register a4
    core::u32 a5;          ///< Register a5
    core::u32 a6;          ///< Register a6
    core::u32 a7;          ///< Register a7
    core::u32 a8;          ///< Register a8
    core::u32 a9;          ///< Register a9
    core::u32 a10;         ///< Register a10
    core::u32 a11;         ///< Register a11
    core::u32 a12;         ///< Register a12
    core::u32 a13;         ///< Register a13
    core::u32 a14;         ///< Register a14
    core::u32 a15;         ///< Register a15
    core::u32 sar;         ///< Shift amount register
    core::u32 exccause;    ///< Exception cause
    core::u32 excvaddr;    ///< Exception virtual address
    core::u32 lbeg;        ///< Loop begin
    core::u32 lend;        ///< Loop end
    core::u32 lcount;      ///< Loop count
};

/// Context saved by software (in addition to exception frame)
struct XtSolFrame {
    core::u32 exit;        ///< Exit routine address
    core::u32 pc;          ///< Program counter
    core::u32 ps;          ///< Processor status (with interrupt enable)
    // Additional callee-saved registers if needed
};

/// Initialize Xtensa-specific hardware for RTOS
void init_xtensa_rtos();

/// Setup timer interrupt for RTOS tick
void setup_timer_interrupt();

} // namespace xtensa

} // namespace alloy::rtos

#endif // ESP32

#endif // ALLOY_RTOS_XTENSA_CONTEXT_HPP
