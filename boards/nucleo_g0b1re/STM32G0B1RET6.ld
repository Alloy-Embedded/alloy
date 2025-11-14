/**
 * Linker Script for STM32G0B1RET6
 *
 * Memory configuration for STM32G0B1RET6 on Nucleo-G0B1RE board:
 * - Flash: 512 KB starting at 0x08000000
 * - RAM: 144 KB starting at 0x20000000
 *
 * ARM Cortex-M0+ architecture
 */

/* Entry point */
ENTRY(Reset_Handler)

/* Memory regions */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 144K
}

/* Stack and heap sizes */
_stack_size = 8K;   /* Stack size: 8 KB */
_heap_size = 0K;    /* Heap size: 0 KB (not using malloc) */

/* Sections */
SECTIONS
{
    /* Vector table and code in FLASH */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))  /* Vector table */
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)           /* Program code */
        *(.text*)
        *(.glue_7)         /* ARM/Thumb interworking */
        *(.glue_7t)
        *(.eh_frame)

        KEEP (*(.init))
        KEEP (*(.fini))

        . = ALIGN(4);
        _etext = .;        /* End of code section */
    } > FLASH

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)         /* Read-only data */
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ARM exception unwinding */
    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    .ARM :
    {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } > FLASH

    /* C++ constructors/destructors */
    .preinit_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
    } > FLASH

    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
    } > FLASH

    .fini_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
        . = ALIGN(4);
    } > FLASH

    /* Used by startup to initialize data */
    _sidata = LOADADDR(.data);

    /* Initialized data section in RAM, loaded from FLASH */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* Start of .data section */
        *(.data)           /* Initialized data */
        *(.data*)
        . = ALIGN(4);
        _edata = .;        /* End of .data section */
    } > RAM AT> FLASH

    /* Uninitialized data section in RAM */
    . = ALIGN(4);
    .bss :
    {
        _sbss = .;         /* Start of .bss section */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;         /* End of .bss section */
        __bss_end__ = _ebss;
    } > RAM

    /* Heap (optional, not used if malloc disabled) */
    ._heap (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE (end = .);
        PROVIDE (_end = .);
        PROVIDE (__heap_start = .);
        . = . + _heap_size;
        . = ALIGN(8);
        PROVIDE (__heap_end = .);
    } > RAM

    /* Stack (grows downward from end of RAM) */
    ._stack (NOLOAD) :
    {
        . = ALIGN(8);
        . = . + _stack_size;
        . = ALIGN(8);
        PROVIDE (_estack = .);
    } > RAM

    /* Remove debug info */
    /DISCARD/ :
    {
        libc.a (*)
        libm.a (*)
        libgcc.a (*)
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
