/**
 * @file same70_xplained_led_blink.ld
 * @brief Minimal linker script for SAME70Q21B LED blink example
 */

/* Memory layout for SAME70Q21B */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00400000, LENGTH = 2048K  /* 2MB Flash */
    RAM (rwx)   : ORIGIN = 0x20400000, LENGTH = 384K   /* 384KB RAM */
}

/* Stack size */
_stack_size = 16K;

/* Entry point */
ENTRY(Reset_Handler)

SECTIONS
{
    /* Vector table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Data (initialized variables) */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* BSS (uninitialized variables) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Stack */
    .stack :
    {
        . = ALIGN(8);
        . = . + _stack_size;
        . = ALIGN(8);
        _estack = .;
    } > RAM
}
