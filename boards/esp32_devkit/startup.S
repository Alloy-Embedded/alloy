/* ESP32 (Xtensa LX6) startup — called by ESP-IDF 2nd-stage bootloader.
 *
 * Placed in .text so it lands in the IROM flash-cached region.
 * The bootloader sets up the MMU (IROM + DROM mappings) BEFORE jumping
 * to call_start_cpu0, so running from flash cache is safe here.
 *
 * l32r requires the literal pool to be BEFORE (negative offset from) the
 * instruction. We emit .literal_position before the function body so the
 * assembler places the pool there. The linker script puts all .literal*
 * before .text* within the IROM section, guaranteeing negative offsets.
 */

    .section .text, "ax"

    /* ------------------------------------------------------------------ *
     * Literal pool — must be BEFORE the l32r instructions that use it    *
     * ------------------------------------------------------------------ */
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

    /* Set stack pointer.
     * 'movi a1, symbol' is assembled as l32r when the value > 12-bit range.
     * The assembler emits the literal at the nearest .literal_position above. */
    movi    a1, _stack_top

    /* Clear return address and jump to C startup (j avoids window rotation) */
    movi    a0, 0
    j       _alloy_startup

    .size call_start_cpu0, . - call_start_cpu0
