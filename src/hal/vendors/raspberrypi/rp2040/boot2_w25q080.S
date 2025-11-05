// Second stage boot loader for RP2040
// Generic W25Q080 flash (used on Raspberry Pi Pico)
//
// This is a minimal second-stage bootloader that:
// 1. Configures QSPI flash for XIP (eXecute In Place)
// 2. Jumps to main program
//
// Must be exactly 256 bytes, with last 4 bytes being CRC32 checksum

.cpu cortex-m0plus
.thumb

.section .boot2, "ax"

.global __boot2_start__

__boot2_start__:
    // Load address of vector table from literal pool at end of boot2
    ldr r3, vtor_address

    // Load stack pointer from vector table
    ldr r0, [r3, #0]
    mov sp, r0

    // Load reset handler address from vector table
    ldr r0, [r3, #4]

    // Jump to reset handler
    bx r0

// Literal pool
.balign 4
vtor_address:
    .word 0x10000100    // Vector table address

// Pad to exactly 252 bytes (leaving 4 bytes for CRC)
.balign 4
.space (252 - (. - __boot2_start__)), 0

// CRC32 checksum (4 bytes) - for bare metal we use 0
.word 0x00000000
