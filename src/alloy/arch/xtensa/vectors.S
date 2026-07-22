/* ESP32 (Xtensa LX6) relocatable vector table + level-1 interrupt dispatch.
 *
 * Installed by pointing VECBASE at _alloy_vector_table (see
 * alloy_vectors_install at the bottom of this file). Replaces the mask-ROM
 * vectors at XCHAL_VECBASE_RESET_VADDR (0x40000000).
 *
 * v1 scope: level-1 interrupts only. Everything else (level 2-5, debug,
 * NMI, kernel-mode exceptions, double exceptions, non-interrupt user
 * exceptions) traps in a tight loop — an honest, debuggable halt with
 * EXCCAUSE/EPC1/DEBUGCAUSE still intact for inspection over JTAG.
 *
 * The window overflow/underflow handler bodies (the 4/8/12 pairs) are the
 * CANONICAL Xtensa windowed-ABI sequences. They are reproduced from
 * xtensa_vectors.S as shipped in ESP-IDF and Zephyr:
 *   Copyright (c) 2015-2019 Cadence Design Systems, Inc.
 *   SPDX-License-Identifier: Apache-2.0 (also MIT-licensed in XTOS releases)
 * Reproducing these verbatim is established practice (same policy as our
 * RP2040 boot2 payload); they are the only correct instruction sequences
 * for this hardware mechanism and admit no creative variation.
 *
 * Vector offsets below match xtensa/config/core-isa.h for the ESP32 config
 * (verified): WindowOverflow4 +0x000, WindowUnderflow4 +0x040,
 * WindowOverflow8 +0x080, WindowUnderflow8 +0x0C0, WindowOverflow12
 * +0x100, WindowUnderflow12 +0x140, Level2 +0x180, Level3 +0x1C0, Level4
 * +0x200, Level5 +0x240, Debug (=level 6) +0x280, NMI (=level 7) +0x2C0,
 * Kernel +0x300, User +0x340, DoubleExc +0x3C0. Slot pitch is 0x40 bytes
 * (User has 0x80 before DoubleExc). Window handler bodies MUST fit and
 * return inside their 0x40 slots (largest is 42 bytes); all other slots
 * hold a 6-byte trampoline or a 3-byte trap loop.
 *
 * Placement requirements (generated linker script):
 *   - Section .alloy.vectors goes to IRAM (instruction SRAM0, executable,
 *     loaded directly by the ESP-IDF 2nd-stage bootloader).
 *   - VECBASE ignores its low 10 bits: the table needs 1 KiB (0x400)
 *     alignment. .balign below requests it; keep ALIGN(0x400) in the
 *     script as well and KEEP() the section.
 *   - _alloy_vector_table must be the FIRST byte of the output section
 *     (the .org slot offsets are section-relative).
 *   - This file is deliberately LITERAL-FREE inside .alloy.vectors (big
 *     constants are built with movi/slli/addi), so no .literal placement
 *     or --text-section-literals gymnastics are needed and the section is
 *     position-independent of any literal pool. If you add code needing a
 *     large constant, construct it the same way.
 *
 * Level-1 interrupt dispatch contract:
 *   - C handler:  void alloy_irq_dispatch(unsigned cpu_int);
 *     called once per pending+enabled level-1 CPU interrupt line, lowest
 *     line number first, re-polling INTERRUPT after each call.
 *   - Called with PS = WOE|UM|INTLEVEL=1 (EXCM clear): windowed calls and
 *     window exceptions work; level-1 sources stay masked (no nesting).
 *   - The handler and its callees must NOT use float/double (FPU coproc
 *     state is not saved) and must clear the source at the peripheral.
 *   - Level-triggered peripheral sources (UART, ...) are acknowledged at
 *     the peripheral (INT_CLR register); the INTERRUPT special register
 *     bit follows the line and drops by itself. Software/edge/timer
 *     sources would need "wsr INTCLEAR" — not used in v1.
 */

/* ---- constants (values verified against ESP32 core-isa.h) ---- */

    .equ    PS_WOE,                 (1 << 18)
    .equ    PS_UM,                  (1 << 5)
    .equ    PS_EXCM,                (1 << 4)
    .equ    EXCCAUSE_L1INT,         4           /* Level1Interrupt */
    .equ    ESP32_INTLEVEL1_MASK,   0x000637FF  /* XCHAL_INTLEVEL1_MASK */

/* Exception frame, allocated on the interrupted stack. Holds the full
 * interrupted register context: the asm below plus the windowed C call
 * chain clobber a0-a15/SAR (and C or ROM code may use zero-overhead
 * loops, hence LBEG/LEND/LCOUNT). 16-byte aligned size per ABI. */
    .equ    XFRAME_PC,      0x00    /* EPC1: interrupted PC   */
    .equ    XFRAME_PS,      0x04    /* interrupted PS (EXCM=1 as saved) */
    .equ    XFRAME_A0,      0x08
    .equ    XFRAME_A1,      0x0C    /* pre-exception SP */
    .equ    XFRAME_A2,      0x10
    .equ    XFRAME_A3,      0x14
    .equ    XFRAME_A4,      0x18
    .equ    XFRAME_A5,      0x1C
    .equ    XFRAME_A6,      0x20
    .equ    XFRAME_A7,      0x24
    .equ    XFRAME_A8,      0x28
    .equ    XFRAME_A9,      0x2C
    .equ    XFRAME_A10,     0x30
    .equ    XFRAME_A11,     0x34
    .equ    XFRAME_A12,     0x38
    .equ    XFRAME_A13,     0x3C
    .equ    XFRAME_A14,     0x40
    .equ    XFRAME_A15,     0x44
    .equ    XFRAME_SAR,     0x48
    .equ    XFRAME_LBEG,    0x4C
    .equ    XFRAME_LEND,    0x50
    .equ    XFRAME_LCOUNT,  0x54
    /* 0x58..0x7F deliberately unused: the TOP 16 bytes of the allocation
     * ([old_a1-0x10, old_a1)) are the interruptee's ABI base save area —
     * the window-spill protocol writes the CALLER's a0-a3 there, so our
     * frame must never touch it (esp-idf XT_STK_FRMSZ reserves the same
     * hole, plus alignment slack). */
    .equ    XFRAME_SIZE,    0x80

/* ===================================================================== */
/* The vector table. 0x400 bytes, 1 KiB aligned, offsets fixed by HW.    */
/* ===================================================================== */

    .section .alloy.vectors, "ax"
    .balign 1024
    .global _alloy_vector_table
_alloy_vector_table:

/* --- +0x000: WindowOverflow4 (canonical, see attribution above) ------ */
/* Entered with PS.EXCM=1, window rotated so a0-a3 are the frame to
 * spill; a5 is the SP of the next-newer frame. rfwo returns. */
    .org    0x000
_alloy_window_overflow4:
    s32e    a0, a5, -16
    s32e    a1, a5, -12
    s32e    a2, a5,  -8
    s32e    a3, a5,  -4
    rfwo

/* --- +0x040: WindowUnderflow4 (canonical) ---------------------------- */
    .org    0x040
_alloy_window_underflow4:
    l32e    a0, a5, -16
    l32e    a1, a5, -12
    l32e    a2, a5,  -8
    l32e    a3, a5,  -4
    rfwu

/* --- +0x080: WindowOverflow8 (canonical) ----------------------------- */
    .org    0x080
_alloy_window_overflow8:
    s32e    a0, a9, -16
    l32e    a0, a1, -12             /* a0 <- call[i-1]'s SP */
    s32e    a1, a9, -12
    s32e    a2, a9,  -8
    s32e    a3, a9,  -4
    s32e    a4, a0, -32
    s32e    a5, a0, -28
    s32e    a6, a0, -24
    s32e    a7, a0, -20
    rfwo

/* --- +0x0C0: WindowUnderflow8 (canonical) ---------------------------- */
    .org    0x0C0
_alloy_window_underflow8:
    l32e    a0, a9, -16
    l32e    a1, a9, -12
    l32e    a2, a9,  -8
    l32e    a7, a1, -12             /* a7 <- call[i-1]'s SP */
    l32e    a3, a9,  -4
    l32e    a4, a7, -32
    l32e    a5, a7, -28
    l32e    a6, a7, -24
    l32e    a7, a7, -20
    rfwu

/* --- +0x100: WindowOverflow12 (canonical) ---------------------------- */
    .org    0x100
_alloy_window_overflow12:
    s32e    a0,  a13, -16
    l32e    a0,  a1,  -12           /* a0 <- call[i-1]'s SP */
    s32e    a1,  a13, -12
    s32e    a2,  a13,  -8
    s32e    a3,  a13,  -4
    s32e    a4,  a0,  -48
    s32e    a5,  a0,  -44
    s32e    a6,  a0,  -40
    s32e    a7,  a0,  -36
    s32e    a8,  a0,  -32
    s32e    a9,  a0,  -28
    s32e    a10, a0,  -24
    s32e    a11, a0,  -20
    rfwo

/* --- +0x140: WindowUnderflow12 (canonical) --------------------------- */
    .org    0x140
_alloy_window_underflow12:
    l32e    a0,  a13, -16
    l32e    a1,  a13, -12
    l32e    a2,  a13,  -8
    l32e    a11, a1,  -12           /* a11 <- call[i-1]'s SP */
    l32e    a3,  a13,  -4
    l32e    a4,  a11, -48
    l32e    a5,  a11, -44
    l32e    a6,  a11, -40
    l32e    a7,  a11, -36
    l32e    a8,  a11, -32
    l32e    a9,  a11, -28
    l32e    a10, a11, -24
    l32e    a11, a11, -20
    rfwu

/* --- +0x180..+0x2C0: medium/high-priority + debug + NMI: v1 traps ---- */
/* Never enabled in v1 (INTENABLE only ever carries level-1 bits). */
    .org    0x180
_alloy_level2_vector:
    j       _alloy_level2_vector

    .org    0x1C0
_alloy_level3_vector:
    j       _alloy_level3_vector

    .org    0x200
_alloy_level4_vector:
    j       _alloy_level4_vector

    .org    0x240
_alloy_level5_vector:
    j       _alloy_level5_vector

    .org    0x280
_alloy_debug_vector:
    j       _alloy_debug_vector

    .org    0x2C0
_alloy_nmi_vector:
    j       _alloy_nmi_vector

/* --- +0x300: Kernel exception (PS.UM=0) — runtime always runs UM=1 --- */
    .org    0x300
_alloy_kernel_vector:
    j       _alloy_kernel_vector

/* --- +0x340: User exception vector — the one live entry point ------- */
/* Level-1 interrupts arrive here (EXCCAUSE=4) as do all other general
 * exceptions taken with PS.UM=1. 6 bytes; slot budget 0x80. */
    .org    0x340
_alloy_user_vector:
    wsr     a0, excsave1            /* free a0; window handlers never
                                       touch EXCSAVE1, so it is stable
                                       until stored into the frame */
    j       _alloy_user_exc

/* --- +0x3C0: Double exception ---------------------------------------- */
    .org    0x3C0
_alloy_doubleexc_vector:
    j       _alloy_doubleexc_vector

/* ===================================================================== */
/* Handlers — placed directly after the table, same section (IRAM),      */
/* comfortably inside j-range (+-128 KiB).                               */
/* ===================================================================== */
    .org    0x400

    .align  4
    .type   _alloy_user_exc, @function
_alloy_user_exc:
    /* Entry state: PS.EXCM=1 (window exceptions + int levels <=
     * XCHAL_EXCM_LEVEL(3) masked), EPC1 = interrupted PC, original a0 in
     * EXCSAVE1, everything else still the interrupted context. */
    rsr     a0, exccause
    beqi    a0, EXCCAUSE_L1INT, _alloy_l1int
_alloy_exc_trap:                    /* v1: any real exception = halt with
                                       EXCCAUSE/EPC1 intact for JTAG */
    j       _alloy_exc_trap
    .size   _alloy_user_exc, . - _alloy_user_exc

    .align  4
    .type   _alloy_l1int, @function
_alloy_l1int:
    /* ---- 1. Full context save on the interrupted stack. ----
     * Required because the exception does NOT rotate the register
     * window: the asm below and the windowed C call chain share (and
     * clobber) the interrupted context's a0-a15 and SAR. Only a0/a1 are
     * used before being saved. Safe under EXCM=1: plain l32i/s32i/wsr
     * can never raise window exceptions. */
    mov     a0, a1
    addi    a1, a1, -XFRAME_SIZE
    s32i    a0, a1, XFRAME_A1       /* pre-exception SP */
    rsr     a0, ps
    s32i    a0, a1, XFRAME_PS       /* saved with EXCM=1 — see exit path */
    rsr     a0, epc1
    s32i    a0, a1, XFRAME_PC       /* window overflows during dispatch
                                       clobber EPC1/EXCCAUSE: must save */
    rsr     a0, excsave1
    s32i    a0, a1, XFRAME_A0       /* interrupted a0 */
    s32i    a2, a1, XFRAME_A2
    s32i    a3, a1, XFRAME_A3
    s32i    a4, a1, XFRAME_A4
    s32i    a5, a1, XFRAME_A5
    s32i    a6, a1, XFRAME_A6
    s32i    a7, a1, XFRAME_A7
    s32i    a8, a1, XFRAME_A8
    s32i    a9, a1, XFRAME_A9
    s32i    a10, a1, XFRAME_A10
    s32i    a11, a1, XFRAME_A11
    s32i    a12, a1, XFRAME_A12
    s32i    a13, a1, XFRAME_A13
    s32i    a14, a1, XFRAME_A14
    s32i    a15, a1, XFRAME_A15
    rsr     a2, sar
    s32i    a2, a1, XFRAME_SAR
    rsr     a2, lbeg                /* ROM memcpy etc. use HW loops */
    s32i    a2, a1, XFRAME_LBEG
    rsr     a2, lend
    s32i    a2, a1, XFRAME_LEND
    rsr     a2, lcount
    s32i    a2, a1, XFRAME_LCOUNT

    /* ---- 2. Open the window machinery, keep level-1 masked. ----
     * PS := WOE | UM | INTLEVEL=1 (EXCM=0). From here on windowed
     * call8/entry/retw work and overflow/underflow vectors service them;
     * level-1 sources cannot nest (INTLEVEL=1); levels 2+ are unused in
     * v1 so nothing can preempt at all. Literal-free constant: 1<<18. */
    movi    a2, 1
    slli    a2, a2, 18              /* PS_WOE */
    addi    a2, a2, PS_UM | 1       /* | PS_UM | PS_INTLEVEL(1) = 0x40021 */
    wsr     a2, ps
    rsync

    /* ---- 2b. Spill ALL live register windows (canonical LX6 sequence,
     * as shipped in esp-idf's _xt_context_save). The spill protocol
     * stores each frame's a0-a3 into the 16 bytes below its CALLEE's SP,
     * discovered through the live a1 chain — so the ORIGINAL pre-
     * exception SP must be in a1 while it runs; otherwise spills land at
     * one address and the eventual retw underflows reload from another.
     * EPC1/EXCCAUSE are clobbered by the overflow exceptions this
     * triggers — both already saved above and rewritten before rfe. */
    addi    a1, a1, XFRAME_SIZE     /* back to the pre-exception SP */
    and     a12, a12, a12
    rotw    3
    and     a12, a12, a12
    rotw    3
    and     a12, a12, a12
    rotw    3
    and     a12, a12, a12
    rotw    3
    and     a12, a12, a12
    rotw    4                       /* 16 quads total: back to our window */
    addi    a1, a1, -XFRAME_SIZE    /* frame pointer again */

    /* ---- 3. Dispatch every pending+enabled level-1 line. ---- */
.Lpoll:
    rsr     a2, interrupt           /* pending lines (level ints follow
                                       the peripheral's INT_CLR) */
    rsr     a3, intenable
    and     a2, a2, a3
    /* mask to level-1 lines: 0x000637FF = (0x18E << 10) - 1 */
    movi    a3, 0x18E
    slli    a3, a3, 10
    addi    a3, a3, -1
    and     a2, a2, a3
    beqz    a2, .Ldone
    neg     a3, a2                  /* isolate lowest set bit */
    and     a3, a3, a2
    nsau    a2, a3                  /* 31 - bit index */
    movi    a6, 31
    sub     a6, a6, a2              /* a6 = CPU int line = call4 arg0 */
    /* call4 (NOT call8): a spill of a call8-tagged frame runs
     * WindowOverflow8, which chases the caller-SP word at [a1-12] — a
     * word nobody wrote for THIS exception-entered frame. Overflow4
     * never dereferences it; the canonical asm->C interrupt boundary is
     * call4-sized for exactly this reason. Callee still sees the
     * argument in its a2. */
    call4   alloy_irq_dispatch      /* void alloy_irq_dispatch(unsigned) */
    j       .Lpoll                  /* re-poll: handler acked at source */

.Ldone:
    /* ---- 4. Restore and return. ----
     * Restore saved PS FIRST: it carries EXCM=1, which re-masks window
     * exceptions and int levels <= 3, making the remaining restores
     * atomic. Then EPC1/SAR/loops, then GPRs (a2 is the shuttle, so it
     * is reloaded after the special registers; a1 last). rfe clears
     * EXCM and resumes at EPC1 with the original PS. */
    l32i    a2, a1, XFRAME_PS
    wsr     a2, ps
    rsync
    l32i    a2, a1, XFRAME_PC
    wsr     a2, epc1
    l32i    a2, a1, XFRAME_SAR
    wsr     a2, sar
    l32i    a2, a1, XFRAME_LBEG
    wsr     a2, lbeg
    l32i    a2, a1, XFRAME_LEND
    wsr     a2, lend
    l32i    a2, a1, XFRAME_LCOUNT
    wsr     a2, lcount
    rsync                           /* complete wsr's before rfe uses EPC1 */
    l32i    a0, a1, XFRAME_A0
    l32i    a2, a1, XFRAME_A2
    l32i    a3, a1, XFRAME_A3
    l32i    a4, a1, XFRAME_A4
    l32i    a5, a1, XFRAME_A5
    l32i    a6, a1, XFRAME_A6
    l32i    a7, a1, XFRAME_A7
    l32i    a8, a1, XFRAME_A8
    l32i    a9, a1, XFRAME_A9
    l32i    a10, a1, XFRAME_A10
    l32i    a11, a1, XFRAME_A11
    l32i    a12, a1, XFRAME_A12
    l32i    a13, a1, XFRAME_A13
    l32i    a14, a1, XFRAME_A14
    l32i    a15, a1, XFRAME_A15
    l32i    a1, a1, XFRAME_A1       /* pre-exception SP last */
    rfe
    .size   _alloy_l1int, . - _alloy_l1int

/* ===================================================================== */
/* alloy_vectors_install — normal windowed C-callable function, in       */
/* .text (NOT the vectors section) so its _alloy_vector_table literal    */
/* follows the existing .literal placement convention from startup.S.    */
/* Call from board::init() BEFORE enabling any interrupt source.         */
/* ===================================================================== */

    .section .text, "ax"
    .literal_position
    .align  4
    .global alloy_vectors_install
    .type   alloy_vectors_install, @function
alloy_vectors_install:
    entry   a1, 32
    movi    a2, 0                   /* no lines enabled across the swap */
    wsr     a2, intenable
    rsync
    movi    a2, _alloy_vector_table
    wsr     a2, vecbase             /* ROM vectors out, ours in */
    rsync
    retw
    .size   alloy_vectors_install, . - alloy_vectors_install
