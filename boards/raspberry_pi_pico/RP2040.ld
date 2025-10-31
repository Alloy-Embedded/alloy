/**
 * Linker script for RP2040 (Raspberry Pi Pico)
 *
 * Memory layout:
 *   FLASH: 2MB external (via XIP - eXecute In Place)
 *   RAM:   264KB SRAM (6 banks)
 *     SRAM0-3: 256KB striped (0x20000000-0x20040000)
 *     SRAM4-5: 8KB each (0x20040000-0x20042000)
 *
 * RP2040 unique features:
 * - Dual Cortex-M0+ cores
 * - XIP (eXecute In Place) from external QSPI flash
 * - 264KB on-chip SRAM
 * - Second stage bootloader required (256 bytes at start of flash)
 *
 * Stack grows downward from end of RAM
 * Heap grows upward from end of .bss
 */

/* Entry point */
ENTRY(_entry_point)

/* Memory regions */
MEMORY
{
    /* External flash accessed via XIP (2MB typical for Pico) */
    /* First 256 bytes reserved for second stage bootloader */
    FLASH (rx)  : ORIGIN = 0x10000100, LENGTH = 2048K - 256

    /* On-chip SRAM (264KB total) */
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 264K

    /* Scratch X and Y (single-cycle access from cores) */
    SCRATCH_X (rwx) : ORIGIN = 0x20040000, LENGTH = 4K
    SCRATCH_Y (rwx) : ORIGIN = 0x20041000, LENGTH = 4K
}

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);  /* Stack at end of RAM */

/* Minimum heap size */
_Min_Heap_Size = 0x1000;   /* 4KB */
_Min_Stack_Size = 0x1000;  /* 4KB */

/* Sections */
SECTIONS
{
    /* Second stage bootloader checksum (first 256 bytes) */
    /* This is handled separately and prepended to the binary */

    /* Boot entry point */
    .boot2 : {
        __boot2_start__ = .;
        KEEP (*(.boot2))
        __boot2_end__ = .;
    } > FLASH

    ASSERT(__boot2_end__ - __boot2_start__ == 256,
        "ERROR: .boot2 section must be 256 bytes")

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
        *(.time_critical*)  /* Time-critical code in RAM */

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

    /* Scratch X memory (optional, for special use) */
    .scratch_x (NOLOAD) :
    {
        *(.scratch_x*)
    } > SCRATCH_X

    /* Scratch Y memory (optional, for special use) */
    .scratch_y (NOLOAD) :
    {
        *(.scratch_y*)
    } > SCRATCH_Y

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
