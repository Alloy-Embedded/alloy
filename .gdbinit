# GDB initialization for SAME70 debugging
set pagination off
set print pretty on
set print array on
set print array-indexes on

# ARM Cortex-M7 specific
set mem inaccessible-by-default off
set architecture arm

# SAME70 peripheral base addresses
define show_pmc
    printf "PMC Base: 0x400E0600\n"
    printf "CKGR_MOR:  0x%08x\n", *(unsigned int*)0x400E0620
    printf "CKGR_MCFR: 0x%08x\n", *(unsigned int*)0x400E0624
    printf "CKGR_PLLAR:0x%08x\n", *(unsigned int*)0x400E0628
    printf "PMC_MCKR:  0x%08x\n", *(unsigned int*)0x400E0630
    printf "PMC_SR:    0x%08x\n", *(unsigned int*)0x400E0668
end

define show_pioc
    printf "PIOC Base: 0x400E1200\n"
    printf "PIO_PSR:   0x%08x\n", *(unsigned int*)0x400E1208
    printf "PIO_OSR:   0x%08x\n", *(unsigned int*)0x400E1218
    printf "PIO_ODSR:  0x%08x\n", *(unsigned int*)0x400E1238
    printf "PIO_PDSR:  0x%08x\n", *(unsigned int*)0x400E123C
end

define show_clock_status
    printf "Clock Status:\n"
    set $sr = *(unsigned int*)0x400E0668
    printf "  MOSCRCS (RC ready):  %d\n", ($sr >> 17) & 1
    printf "  MOSCSELS (XTAL sel): %d\n", ($sr >> 16) & 1
    printf "  MCKRDY (MCK ready):  %d\n", ($sr >> 3) & 1
    printf "  LOCKA (PLL ready):   %d\n", ($sr >> 1) & 1
end

# Commands for quick debugging
define reset_and_halt
    monitor reset halt
end

define reset_and_run
    monitor reset
    continue
end

# Helpful message
printf "SAME70 GDB initialized\n"
printf "Custom commands:\n"
printf "  show_pmc          - Show PMC registers\n"
printf "  show_pioc         - Show PIOC registers\n"
printf "  show_clock_status - Show clock status bits\n"
printf "  reset_and_halt    - Reset and halt target\n"
printf "  reset_and_run     - Reset and continue\n"
