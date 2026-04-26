/* Linker script for Raspberry Pi Pico (RP2040)
 *
 * Memory:
 *   FLASH: 2 MB at 0x10000000 (XIP via QSPI)
 *   RAM:   264 KB at 0x20000000 (SRAM0-4)
 *
 * Note: RP2040 boots from a 256-byte second-stage bootloader in FLASH
 * which sets up XIP mode. For initial compile validation this minimal
 * script suffices; a full boot flow needs the pico-sdk boot2 blob.
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x10000000, LENGTH = 2048K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 264K
}

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))
        KEEP(*(.boot2))
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
    } > RAM AT > FLASH

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Dedicated stack for core 1. 8 KB, 8-byte aligned (AAPCS).
     * _core1_stack_top is loaded into SP by the RP2040 ROM after the
     * 5-word SIO FIFO handshake completes. */
    .core1_stack (NOLOAD) : ALIGN(8)
    {
        _core1_stack_bottom = .;
        . = . + 8192;
        _core1_stack_top = .;
    } > RAM

    end = .;
    _end = .;
    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
