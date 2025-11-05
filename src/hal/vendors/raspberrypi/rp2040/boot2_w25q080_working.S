// Second stage boot loader for RP2040
// Based on official pico-sdk boot2_w25q080.S
// Configures W25Q080 flash for execute-in-place (XIP)

.cpu cortex-m0plus
.thumb

.section .boot2, "ax"

// Must be 256 bytes total, with CRC32 as last word

.global __boot2_start__
__boot2_start__:

// Configure SSI (SPI) for XIP from flash
// Base: SSI = 0x18000000
.equ SSI_BASE, 0x18000000
.equ CTRLR0, 0x00
.equ SSIENR, 0x08
.equ BAUDR, 0x14
.equ SPI_CTRLR0, 0xF4

    // Disable SSI
    movs r3, #0
    ldr r1, =SSI_BASE
    str r3, [r1, #SSIENR]

    // Set baud rate (divide by 4 for conservative speed)
    movs r3, #4
    str r3, [r1, #BAUDR]

    // Configure CTRLR0 for standard SPI
    // DFS_32 = 31 (32-bit), FRF = 0 (Motorola SPI), SCPH = 0, SCPOL = 0
    ldr r3, =0x001f0000  // 32-bit data frame
    str r3, [r1, #CTRLR0]

    // Configure SPI_CTRLR0 for read command
    // TRANS_TYPE = 0 (standard), ADDR_L = 6 (24-bit address)
    movs r3, #0x18  // 24-bit address (6 * 4bits)
    str r3, [r1, #SPI_CTRLR0]

    // Re-enable SSI
    movs r3, #1
    str r3, [r1, #SSIENR]

    // Jump to main firmware
    // Vector table is at 0x10000100 (after boot2)
    ldr r3, =0x10000100

    // Load stack pointer from vector table
    ldr r0, [r3, #0]
    mov sp, r0

    // Load reset handler from vector table
    ldr r0, [r3, #4]

    // Jump to reset handler
    bx r0

// Pad to 252 bytes
.org 252

// CRC32 checksum placeholder (to be calculated)
// For testing, we can use 0 but ideally should be computed
.word 0x00000000
