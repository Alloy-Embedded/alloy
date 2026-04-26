/* Linker script for ESP32-DevKit (Xtensa LX6)
 *
 * Boot flow: ROM → ESP-IDF 2nd-stage bootloader (flash 0x1000)
 *            → our app (flash 0x10000)
 *
 * ESP-IDF v5 bootloader calls cache_flash_mmu_set() for DROM and IROM
 * unconditionally (even when size=0, which aborts with vaddr=0).
 * We must have at least one byte in DROM and IROM so the bootloader
 * gets valid virtual addresses to map.
 *
 * All code runs from IROM (flash cache) — bootloader sets up the MMU
 * before jumping to the entry point, so there is no IRAM bootstrap needed.
 *
 * Memory map (ESP32):
 *   DROM  (flash-cached RO data) : 0x3F400000 – 0x3FBFFFFF
 *   IRAM  (fast SRAM, optional)  : 0x40080000 – 0x4009FFFF
 *   IROM  (flash-cached code)    : 0x400D0000 – 0x40BFFFFF
 *   DRAM  (SRAM data/BSS/stack)  : 0x3FFB0000 – 0x3FFFFFFF
 */

ENTRY(call_start_cpu0)

MEMORY
{
    /* Flash-cached read-only data (MMU-mapped by bootloader) */
    DROM (r)   : ORIGIN = 0x3F400010, LENGTH = (3584K - 16)
    /* Flash-cached executable code (MMU-mapped by bootloader) */
    IROM (rx)  : ORIGIN = 0x400D0010, LENGTH = (1984K - 16)
    /* Internal SRAM for code that must not depend on flash cache.
     * The 2nd-stage bootloader uses 0x40080000-0x40080EE8 area, but it
     * hands control off before our code starts, so we can reuse it.
     * Use 0x40080400 onwards to be safe (avoids low-IRAM ROM trampolines). */
    IRAM (rwx) : ORIGIN = 0x40080400, LENGTH = 126K
    /* Direct SRAM for data, BSS, and stack */
    DRAM (rw)  : ORIGIN = 0x3FFB0000, LENGTH = 272K
}

_stack_top = ORIGIN(DRAM) + LENGTH(DRAM);   /* 0x3FFF4000 */

SECTIONS
{
    /* ------------------------------------------------------------------ *
     * DROM — flash-cached read-only data                                  *
     * Bootloader maps this region via MMU to flash physical address.      *
     * LMA = VMA; esptool places segment at a 64KB-page-aligned offset.   *
     * ------------------------------------------------------------------ */
    .rodata : ALIGN(4)
    {
        *(.rodata .rodata.*)
        *(.rodata1)
        . = ALIGN(4);
    } > DROM

    /* ------------------------------------------------------------------ *
     * IROM — flash-cached executable code                                 *
     * All .text (including call_start_cpu0) runs here after MMU setup.   *
     * Literals must precede their l32r instructions → put all .literal*  *
     * before .text* within this section.                                  *
     * ------------------------------------------------------------------ */
    /* ------------------------------------------------------------------ *
     * IRAM — internal-SRAM code that must run without flash cache.        *
     * Used by APP_CPU bring-up: when APP_CPU is released, there's a brief *
     * window of cache-controller arbitration glitch that returns garbage  *
     * on PRO_CPU's flash fetches.  Code reachable in that window must     *
     * live here.                                                           *
     * ------------------------------------------------------------------ */
    .iram_text : ALIGN(4)
    {
        _siram_text = .;
        *(.iram1.literal .iram1.literal.*)
        *(.iram0.literal .iram0.literal.*)
        *(.iram1.text .iram1.text.*)
        *(.iram0.text .iram0.text.*)
        . = ALIGN(4);
        _eiram_text = .;
    } > IRAM

    .text : ALIGN(4)
    {
        /* Literal pools — must be at lower address than the l32r users */
        *(.literal .literal.*)
        /* Code */
        *(.text .text.*)
        . = ALIGN(4);
        _etext = .;
    } > IROM

    .init_array : ALIGN(4)
    {
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        __init_array_end = .;
    } > IROM

    .fini_array : ALIGN(4)
    {
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        __fini_array_end = .;
    } > IROM

    /* ------------------------------------------------------------------ *
     * DRAM — read-write data and BSS                                      *
     * ------------------------------------------------------------------ */
    _sidata = LOADADDR(.data);

    .data : ALIGN(4)
    {
        _sdata = .;
        *(.data .data.*)
        . = ALIGN(4);
        _edata = .;
    } > DRAM

    .bss (NOLOAD) : ALIGN(4)
    {
        _sbss = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > DRAM

    /* Dedicated stack for APP_CPU (core 1). 8 KB, 16-byte aligned.
     * _appcpu_stack_top is loaded into a1 (SP) by _alloy_appcpu_entry. */
    .appcpu_stack (NOLOAD) : ALIGN(16)
    {
        _appcpu_stack_bottom = .;
        . = . + 8192;
        _appcpu_stack_top = .;
    } > DRAM

    _end = .;
}
