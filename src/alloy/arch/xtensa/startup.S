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

    /* Stack pointer from the generated linker script */
    movi    a1, _stack_top

    /* Clear return address; plain jump avoids window rotation */
    movi    a0, 0
    j       _alloy_startup

    .size call_start_cpu0, . - call_start_cpu0
