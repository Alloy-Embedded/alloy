/* Linker script for ESP32-C3-DevKitM-1 — direct boot (no second-stage bootloader)
 *
 * Direct boot: ROM checks flash[0] == 0xaebd041d, then jumps to flash[4].
 *
 * Memory:
 *   FLASH : 4 MB at 0x42000000 (XIP instruction bus)
 *   SRAM  : 384 KB at 0x3FC80000 (SRAM1 data bus)
 */

ENTRY(_start)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x42000000, LENGTH = 4096K
    SRAM  (rwx) : ORIGIN = 0x3FC80000, LENGTH = 384K
}

_stack_top = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    /* Direct boot magic word at flash[0] = 0x42000000 */
    .boot_magic :
    {
        LONG(0xaebd041d)
    } > FLASH

    /* Reset vector at flash[4] = 0x42000004 — ROM jumps here */
    .reset_vector :
    {
        KEEP(*(.reset_vector))
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .preinit_array :
    {
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP(*(.preinit_array*))
        __preinit_array_end = .;
    } > FLASH

    .init_array :
    {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        __init_array_end = .;
    } > FLASH

    .fini_array :
    {
        . = ALIGN(4);
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        __fini_array_end = .;
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    end = .;
    _end = .;
}
