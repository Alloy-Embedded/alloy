/* Linker script for ESP32-DevKit (Xtensa LX6) — IRAM-only app
 *
 * Boot flow: ROM → ESP-IDF 2nd-stage bootloader (flash 0x1000)
 *            → our app (flash 0x10000, loaded to IRAM by bootloader)
 *
 * All code+data lands in IRAM/DRAM — no flash cache mapping needed.
 * The ESP-IDF bootloader reads segments from the ESP32 image header
 * and copies each segment to its load address before jumping to entry.
 *
 * Memory:
 *   IRAM : 128 KB at 0x40080000  (instruction RAM)
 *   DRAM : 272 KB at 0x3FFB0000  (data RAM, conservative — top reserved for ROM/BT)
 */

ENTRY(call_start_cpu0)

MEMORY
{
    IRAM (rwx) : ORIGIN = 0x40080000, LENGTH = 128K
    DRAM (rw)  : ORIGIN = 0x3FFB0000, LENGTH = 272K
}

_stack_top = ORIGIN(DRAM) + LENGTH(DRAM);   /* 0x3FFF4000 */

SECTIONS
{
    /* Entry + IRAM-forced code.
     * .iram1.literal must precede .iram1.text so l32r offsets are negative. */
    .iram1.text :
    {
        . = ALIGN(4);
        *(.iram1.literal)
        *(.iram0.literal)
        KEEP(*(.iram1.text))
        KEEP(*(.entry.text))
        *(.iram0.text*)
        *(.iram1.text*)
    } > IRAM

    /* Xtensa l32r literals MUST come before their .text counterparts.
     * Placing all .literal* here ensures the linker never emits a forward l32r. */
    .text :
    {
        *(.literal .literal.*)
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
        _etext = .;
    } > IRAM

    .preinit_array :
    {
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP(*(.preinit_array*))
        __preinit_array_end = .;
    } > IRAM

    .init_array :
    {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        __init_array_end = .;
    } > IRAM

    .fini_array :
    {
        . = ALIGN(4);
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        __fini_array_end = .;
    } > IRAM

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > DRAM AT > IRAM

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > DRAM

    end = .;
    _end = .;
}
