/**
 * Linker script for ATSAME70Q21 (Atmel SAME70 Xplained)
 *
 * Memory layout:
 *   FLASH: 2048KB (2MB) at 0x00400000
 *   RAM:   384KB at 0x20000000 (SRAM)
 *
 * SAME70Q21 features:
 * - ARM Cortex-M7 @ 300MHz
 * - 2MB Flash
 * - 384KB SRAM
 * - FPU (double precision)
 * - DSP instructions
 * - Cache (I-Cache and D-Cache)
 *
 * Stack grows downward from end of RAM
 * Heap grows upward from end of .bss
 */

/* Entry point */
ENTRY(Reset_Handler)

/* Memory regions */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00400000, LENGTH = 2048K
    RAM (rwx)   : ORIGIN = 0x20400000, LENGTH = 384K  /* SAME70 RAM starts at 0x20400000! */
}

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);  /* Stack at end of RAM */

/* Minimum heap size */
_Min_Heap_Size = 0x2000;   /* 8KB */
_Min_Stack_Size = 0x2000;  /* 8KB */

/* Sections */
SECTIONS
{
    /* Interrupt vector table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))  /* Vector table */
        . = ALIGN(4);
    } >FLASH

    /* Program code and read-only data */
    .text :
    {
        . = ALIGN(4);
        *(.text)           /* Normal code */
        *(.text*)          /* C++ code */
        *(.glue_7)         /* ARM/Thumb interworking */
        *(.glue_7t)
        *(.eh_frame)

        KEEP(*(.init))     /* Startup code */
        KEEP(*(.fini))     /* Cleanup code */

        . = ALIGN(4);
        _etext = .;        /* End of code section */
    } >FLASH

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)         /* Constants */
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* C++ exception handling (if needed) */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >FLASH

    /* C++ constructors/destructors */
    .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } >FLASH

    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } >FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } >FLASH

    /* Used by startup to initialize data */
    _sidata = LOADADDR(.data);

    /* Initialized data (copied from flash to RAM at startup) */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* Start of .data in RAM */
        *(.data)
        *(.data*)
        *(.RamFunc)        /* Functions in RAM */

        . = ALIGN(4);
        _edata = .;        /* End of .data in RAM */
    } >RAM AT> FLASH

    /* Uninitialized data (zeroed at startup) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;         /* Start of .bss */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;         /* End of .bss */
        __bss_end__ = _ebss;
    } >RAM

    /* Heap (grows upward) */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM

    /* Remove debugging sections */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    /* ARM attributes */
    .ARM.attributes 0 : { *(.ARM.attributes) }
}
