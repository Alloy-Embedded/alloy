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

    /* ------------------------------------------------------------------ *
     * APP_CPU (core 1) entry trampoline — placed in IRAM so APP_CPU does *
     * not depend on flash cache while booting.                            *
     *                                                                      *
     * OPEN ISSUE (2026-04-26): APP_CPU never reaches this function.       *
     * The UART probe below ('#1\n') has never appeared in output.         *
     *                                                                      *
     * Root-cause hypothesis: ets_set_appcpu_boot_addr (ROM 0x4000689C)    *
     * dispatches correctly when the target address is in IROM range       *
     * (0x400D0000+), but silently ignores IRAM addresses (0x40080000+).   *
     * ROM call_start_cpu1 may range-check the boot address.               *
     *                                                                      *
     * Proposed fix to try: place a tiny IROM relay stub that immediately  *
     * jumps here, and give that stub's address to                         *
     * ets_set_appcpu_boot_addr instead.  See board.cpp start_app_cpu for  *
     * complete analysis and alternative next steps.                        *
     * ------------------------------------------------------------------ */
    .section .iram1.text, "ax"
    .literal_position

    .global  _alloy_appcpu_entry
    .type    _alloy_appcpu_entry, @function
    .align   4

_alloy_appcpu_entry:
    /* DIAG: write '#1\n' to UART0 TX FIFO to confirm APP_CPU reached here.
     * UART0 base 0x3FF40000, offset 0 = TX FIFO push register.
     * TX FIFO is 128 bytes deep; safe to write without polling at boot. */
    movi    a3, 0x3FF40000
    movi    a4, '#'
    s32i    a4, a3, 0
    movi    a4, '1'
    s32i    a4, a3, 0
    movi    a4, 0x0A           /* '\n' */
    s32i    a4, a3, 0
    memw

    /* Disable interrupts at INTLEVEL=15 atomically. */
    rsil    a0, 15

    /* Reset register windows for APP_CPU */
    movi    a0, 0
    wsr     a0, WINDOWBASE
    rsync
    movi    a0, 1
    wsr     a0, WINDOWSTART
    rsync

    /* Initialise PS:
     *   PS.WOE=1   (bit 18) — REQUIRED for call8/entry/retw windowed ABI.
     *                          Without WOE, the 'entry' instruction at the
     *                          top of _alloy_appcpu_user_entry raises
     *                          IllegalInstruction.
     *   PS.UM=1    (bit 5)
     *   PS.EXCM=0  (bit 4)
     *   PS.INTLEVEL=3 — keep IRQs disabled until user code opts in.
     * Encoded value: (1<<18) | (1<<5) | 3 = 0x40023 */
    movi    a0, 0x40023
    wsr     a0, PS
    rsync

    /* Clear shift-amount register (SAR is undefined on reset) */
    movi    a0, 0
    wsr     a0, SAR

    /* Set APP_CPU stack pointer to its dedicated 8 KB buffer in .bss */
    movi    a1, _appcpu_stack_top

    /* Clear return address */
    movi    a0, 0

    /* call8: windowed call required — _alloy_appcpu_user_entry is compiled
     * with windowed ABI and starts with 'entry a1, N'.  Using call0 leaves
     * PS.CALLINC=0, which makes 'entry' undefined behaviour on Xtensa LX6.
     * call8 sets CALLINC=8 so 'entry' correctly rotates the register window. */
    call8   _alloy_appcpu_user_entry

_appcpu_loop:
    j       _appcpu_loop

    .size _alloy_appcpu_entry, . - _alloy_appcpu_entry
