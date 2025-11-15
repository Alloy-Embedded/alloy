/**
 * Linker Script for STM32F722ZET6
 *
 * Memory configuration for STM32F722ZET6 on Nucleo-F722ZE board:
 * - Flash: 512 KB starting at 0x08000000
 * - RAM: 96 KB starting at 0x20000000
 *
 * ARM Cortex-M4F architecture (with FPU)
 */

/* Entry point */
ENTRY(Reset_Handler)

/* Memory regions */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 256K
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

    /* Initialized data will be copied from FLASH to RAM during startup */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* Start of .data section */
        *(.data)           /* Initialized data */
        *(.data*)
        . = ALIGN(4);
        _edata = .;        /* End of .data section */
    } > RAM AT> FLASH

    /* Uninitialized data (zero-initialized) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;         /* Start of .bss section */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;         /* End of .bss section */
        __bss_end__ = _ebss;
    } > RAM

    /* Heap (if needed) */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        _heap_start = .;
        . = . + _heap_size;
        . = ALIGN(8);
        _heap_end = .;
    } > RAM

    /* Stack (grows downward from end of RAM) */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        . = . + _stack_size;
        . = ALIGN(8);
        _estack = .;       /* Top of stack */
    } > RAM

    /* Remove unwanted sections */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    /* Debugging symbols */
    .ARM.attributes 0 : { *(.ARM.attributes) }
}
