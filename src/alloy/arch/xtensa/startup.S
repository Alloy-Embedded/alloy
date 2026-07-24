/* ESP32 (Xtensa LX6) PRO_CPU startup — hand-written BEHAVIOR, adapted from
 * the old-alloy hardware-validated startup (commit 0f6a41cb).
 *
 * Entered by the ESP-IDF 2nd-stage bootloader with the MMU already mapping
 * IROM/DROM, so running from flash cache is safe. Single-core: APP_CPU is
 * left parked (the old repo documents its IRAM dispatch never working).
 *
 * l32r needs literals at negative offsets: .literal_position before the
 * function + the linker placing .literal* before .text* guarantee that.
 */

    .section .text, "ax"

    .literal_position

    .global  call_start_cpu0
    .type    call_start_cpu0, @function
    .align   4

call_start_cpu0:
    /* Reset register windows: WINDOWBASE=0, WINDOWSTART=1 */
    movi    a0, 0
    wsr     a0, WINDOWBASE
    rsync
    movi    a0, 1
    wsr     a0, WINDOWSTART
    rsync

    /* Enable coprocessor 0 (the FPU) so portable app code that uses `float`
     * runs on ESP32 as it does on the soft-float Cortex-M targets, instead of
     * taking a Coprocessor0Disabled exception (EXCCAUSE=32) into the panic
     * loop. CPENABLE resets to 0 and the boot ROM never sets it. Safe for the
     * single-app superloop: there is no per-task FP context to manage, and the
     * ISR contract already forbids float in handlers (arch/xtensa/vectors.S),
     * so app FP state is untouched across an interrupt. */
    movi    a0, 1              /* bit 0 = CP0 (the LX6 FPU) */
    wsr     a0, CPENABLE
    rsync

    /* Stack pointer from the generated linker script, minus the reset
     * pseudo-frame's 16-byte base save area. */
    movi    a1, _stack_top
    addi    a1, a1, -16

    /* Enter C via CALLX4 — NOT a plain jump. The windowed spill protocol
     * self-maintains only for frames that HAVE a caller: each spill of a
     * caller writes the [a1-12] "caller SP" word its callee's own spill
     * later chases. A jump-entered base frame has no caller, so its
     * (call8-sized) spill chased uninitialized memory above the stack —
     * wild stores, double exception on real silicon. With callx4 the
     * reset pseudo-frame spills chase-free (overflow4) and writes the
     * chase word for everything above it. a0=0 terminates the chain. */
    movi    a0, 0
    movi    a4, _alloy_startup
    callx4  a4
1:  j       1b

    .size call_start_cpu0, . - call_start_cpu0
