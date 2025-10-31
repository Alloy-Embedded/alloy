/**
 * Linker script for ESP32 (Xtensa LX6 Dual-Core)
 *
 * Memory layout:
 *   IRAM:  128KB at 0x40080000 (instruction RAM)
 *   DRAM:  320KB at 0x3FFB0000 (data RAM)
 *   FLASH: 4MB at 0x400D0000 (external SPI flash, via cache)
 *
 * ESP32 has complex memory architecture:
 * - IRAM: Fast instruction memory
 * - DRAM: Fast data memory
 * - Flash: Slower but large, accessed via cache
 * - RTC: Ultra-low-power memory for deep sleep
 */

/* Entry point */
ENTRY(call_start_cpu0)

/* Memory regions */
MEMORY
{
    /* Internal SRAM for instructions (128KB) */
    IRAM (rwx)    : ORIGIN = 0x40080000, LENGTH = 128K

    /* Internal SRAM for data (320KB) */
    DRAM (rw)     : ORIGIN = 0x3FFB0000, LENGTH = 320K

    /* External SPI Flash (mapped, 4MB) */
    FLASH (rx)    : ORIGIN = 0x400D0000, LENGTH = 4M

    /* RTC Fast Memory (8KB - for deep sleep) */
    RTC_FAST (rw) : ORIGIN = 0x3FF80000, LENGTH = 8K
}

/* Stack configuration */
_stack_size = 8K;
_heap_size = 32K;

/* Sections */
SECTIONS
{
    /* Reset vector and interrupt handlers in IRAM */
    .iram0.vectors : ALIGN(4)
    {
        _iram_start = ABSOLUTE(.);
        . = 0x0;
        _init_start = ABSOLUTE(.);
        KEEP(*(.WindowVectors.text));
        . = 0x180;
        KEEP(*(.Level2InterruptVector.text));
        . = 0x1c0;
        KEEP(*(.Level3InterruptVector.text));
        . = 0x200;
        KEEP(*(.Level4InterruptVector.text));
        . = 0x240;
        KEEP(*(.Level5InterruptVector.text));
        . = 0x280;
        KEEP(*(.DebugExceptionVector.text));
        . = 0x2c0;
        KEEP(*(.NMIExceptionVector.text));
        . = 0x300;
        KEEP(*(.KernelExceptionVector.text));
        . = 0x340;
        KEEP(*(.UserExceptionVector.text));
        . = 0x3C0;
        KEEP(*(.DoubleExceptionVector.text));
        . = 0x400;
        _init_end = ABSOLUTE(.);
    } > IRAM

    /* Code that needs to run from IRAM (interrupt handlers, critical code) */
    .iram0.text : ALIGN(4)
    {
        *(.iram .iram.*)
        *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)
        *libfreertos.a:(.literal .text .literal.* .text.*)
        *libhal.a:*(.literal .text .literal.* .text.*)
    } > IRAM

    /* Most code goes to Flash (accessed via cache) */
    .flash.text : ALIGN(4)
    {
        _stext = .;
        *(.literal .text .literal.* .text.*)
        *(.stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
        *(.fini .gnu.version)
        _etext = .;
    } > FLASH

    /* Read-only data in Flash */
    .flash.rodata : ALIGN(4)
    {
        _rodata_start = ABSOLUTE(.);
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
        *(.rodata1)
        __XT_EXCEPTION_TABLE__ = ABSOLUTE(.);
        *(.xt_except_table)
        *(.gcc_except_table)
        *(.gnu.linkonce.e.*)
        *(.gnu.version_r)
        *(.eh_frame)

        /* C++ global constructors */
        . = ALIGN(4);
        __init_array_start = ABSOLUTE(.);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = ABSOLUTE(.);

        . = ALIGN(4);
        _rodata_end = ABSOLUTE(.);
    } > FLASH

    /* C++ exception handling */
    .flash.rodata_noload (NOLOAD) :
    {
        . = ALIGN(4);
    } > FLASH

    /* Initialized data (copied from Flash to DRAM at startup) */
    .data : ALIGN(4)
    {
        _data_start = ABSOLUTE(.);
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        *(.data1)
        *(.sdata)
        *(.sdata.*)
        *(.gnu.linkonce.s.*)
        *(.sdata2)
        *(.sdata2.*)
        *(.gnu.linkonce.s2.*)
        KEEP(*(.jcr))
        *(.dram .dram.*)
        _data_end = ABSOLUTE(.);
        . = ALIGN(4);
    } > DRAM AT > FLASH

    /* Used by startup to initialize data */
    _data_load = LOADADDR(.data);

    /* Uninitialized data (zeroed at startup) */
    .bss (NOLOAD) : ALIGN(4)
    {
        . = ALIGN (8);
        _bss_start = ABSOLUTE(.);
        *(.dynsbss)
        *(.sbss)
        *(.sbss.*)
        *(.gnu.linkonce.sb.*)
        *(.scommon)
        *(.sbss2)
        *(.sbss2.*)
        *(.gnu.linkonce.sb2.*)
        *(.dynbss)
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN (8);
        _bss_end = ABSOLUTE(.);
    } > DRAM

    /* Heap (grows upward) */
    .heap (NOLOAD) : ALIGN(4)
    {
        _heap_start = ABSOLUTE(.);
        . = . + _heap_size;
        _heap_end = ABSOLUTE(.);
    } > DRAM

    /* Stack for CPU0 (grows downward from end of DRAM) */
    .stack (NOLOAD) : ALIGN(16)
    {
        . = . + _stack_size;
        _stack_end = ABSOLUTE(.);
    } > DRAM

    /* RTC Fast Memory (for deep sleep data) */
    .rtc.text (NOLOAD) :
    {
        . = ALIGN(4);
        *(.rtc.literal .rtc.text)
    } > RTC_FAST

    .rtc.data (NOLOAD) :
    {
        *(.rtc.data)
        *(.rtc.rodata)
    } > RTC_FAST

    .rtc.bss (NOLOAD) :
    {
        *(.rtc.bss)
    } > RTC_FAST

    /* IRAM end marker */
    .iram0.bss (NOLOAD) : ALIGN(4)
    {
        . = ALIGN (4);
        _iram_end = ABSOLUTE(.);
    } > IRAM

    /* Discard debug sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
    }

    /* Xtensa-specific */
    .xt.prop 0 :
    {
        KEEP(*(.xt.prop .xt.prop.* .gnu.linkonce.prop.*))
    }
    .xt.lit 0 :
    {
        KEEP(*(.xt.lit .xt.lit.* .gnu.linkonce.p.*))
    }
}
