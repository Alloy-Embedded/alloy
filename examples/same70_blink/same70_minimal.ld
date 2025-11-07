/**
 * Minimal linker script for ATSAME70Q21 (Cortex-M7)
 * Flash: 2MB @ 0x00400000
 * SRAM:  384KB @ 0x20400000
 */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00400000, LENGTH = 2M
    SRAM (rwx)  : ORIGIN = 0x20400000, LENGTH = 384K
}

/* Stack size */
_stack_size = 4K;

/* Top of stack (end of SRAM) */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    /* Vector table and code in Flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Data section in SRAM, loaded from Flash */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT> FLASH

    _sidata = LOADADDR(.data);

    /* BSS section in SRAM */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    /* Heap starts after BSS */
    . = ALIGN(8);
    _heap_start = .;
    _heap_end = _estack - _stack_size;
}
