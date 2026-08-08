// Cortex-M fault capture — hand-written BEHAVIOR.
//
// The generated vector table declares HardFault_Handler weak, aliased to a
// Default_Handler whose body is `for (;;) {}`. Defining it strongly here
// overrides that alias for every Cortex-M build, with no codegen change: the
// build compiles every .cpp under arch/<ns>/ into the firmware.
//
// Only HardFault is claimed. On ARMv7-M the configurable faults (MemManage,
// BusFault, UsageFault) are DISABLED at reset and escalate into HardFault, so
// one handler already catches everything the generated table has slots for. On
// ARMv6-M they do not exist at all.
//
// The handler runs on a machine that has already failed, so it does the least
// it can: read the stacked frame, copy a few words into RAM that survives the
// reset, and reset. No driver, no formatting, no clock work — anything that
// could fault again would turn one crash into a silent reboot loop.
//
// SCB register addresses below are ARM-ARCHITECTURAL, not silicon facts: they
// are identical on every Cortex-M and are not in any chip's data. arch/ is the
// one place the contract gate allows them.
//
// WHAT IS WITNESSED AND WHAT IS NOT (Renode 1.16.1, nucleo_g071rb/Cortex-M0
// and nucleo_f722ze/Cortex-M7). Witnessed end to end, repeatedly: handler entry
// through BOTH trampolines below with a correctly stacked frame (the recorded
// pc named the exact faulting instruction), the capture, the SYSRESETREQ
// reset, and the next boot reading the record back — three consecutive
// crash/report cycles ending in the example's safe mode. The NMI trampoline
// fires from firmware alone (fault::trigger() pends it via ICSR), the
// HardFault one via the NVIC (`sysbus.nvic SetPendingIRQ 3` in the CI leg).
// The v7-M CFSR/MMFAR/BFAR branch executed without incident on the M7.
//
// STILL SILICON-ONLY: ORGANIC fault entry. Renode's core does not perform
// M-profile fault exception entry at all — a udf or a wild jump to a non-Thumb
// address warps PC to 0 WITHOUT ever reading the vector table (verified on
// cortex-m0, m0+ and m7 cpuTypes; also with flash aliased at 0 and with
// VectorTableOffset set explicitly), and an unaligned load on ARMv6-M simply
// does not fault there. So "a real HardFault escalates into this handler" is
// still an unwitnessed claim about silicon; everything downstream of handler
// entry — capture, persistence across reset, take() — is not.

#include "alloy/fault.hpp"

#include <cstdint>

namespace {

// A magic that no plausible uninitialized RAM pattern reproduces, so a
// power-on boot (whose RAM is arbitrary) is not mistaken for a crash report.
constexpr std::uint32_t kMagic = 0x0A11'0FA0u;  // contract-ok: a RAM sentinel, not an address

struct persistent {
    std::uint32_t magic;
    std::uint32_t consecutive;
    alloy::fault::record rec;
};

// `.noinit` is placed after .bss by the generated linker script and is NOLOAD,
// so neither startup's zero-fill nor the .data copy touches it. RAM keeps its
// contents across a warm reset, which is what makes this readable next boot.
//
// The union is load-bearing, not decoration. `record` has default member
// initializers, so a plain `persistent` global is not trivially
// default-constructible and GCC emits a RUNTIME static initializer for it —
// which zeroes the record every boot and silently defeats .noinit, on silicon
// and emulator alike. Witnessed at instruction level: the ELF grew a
// _GLOBAL__sub_I_ function storing zeros over rec.* (magic and consecutive
// survived only because those two members happen to lack initializers), and
// take() could never report. Wrapping the storage in a union whose chosen
// constructor initializes only a char keeps the object out of static
// initialization; `constinit` makes the compiler PROVE it stays out, so the
// bug cannot creep back when someone touches `record`.
union crash_store {
    persistent p;
    char raw;
    constexpr crash_store() : raw(0) {}
};
[[gnu::section(".noinit")]] constinit crash_store g_store;
constinit persistent& g_crash = g_store.p;

// The exception frame the core stacks automatically (ARMv6-M and ARMv7-M
// stack the same eight words in the same order).
struct exception_frame {
    std::uint32_t r0, r1, r2, r3, r12, lr, pc, psr;
};

constexpr std::uintptr_t kSCB_AIRCR = 0xE000ED0Cu;
constexpr std::uint32_t kSysResetReq = 0x05FA'0004u;  // VECTKEY | SYSRESETREQ

// Cortex-M0/M0+ (ARMv6-M) has no CFSR/MMFAR/BFAR — those addresses are
// reserved there. The record reports 0 rather than reading them and inventing
// a number.
#if !defined(__ARM_ARCH_6M__)
constexpr std::uintptr_t kSCB_CFSR = 0xE000ED28u;
constexpr std::uintptr_t kSCB_MMFAR = 0xE000ED34u;
constexpr std::uintptr_t kSCB_BFAR = 0xE000ED38u;
constexpr std::uint32_t kMMARVALID = 1u << 7;
constexpr std::uint32_t kBFARVALID = 1u << 15;

std::uint32_t read32(std::uintptr_t addr) {
    return *reinterpret_cast<volatile std::uint32_t*>(addr);
}
#endif

}  // namespace

extern "C" {

// Called by the naked trampoline below with the frame the core stacked.
[[gnu::used]] void alloy_fault_capture(const exception_frame* frame,
                                       std::uint32_t what) {
    // A crash DURING crash handling must not clear the count that would tell
    // the next boot it is looping, so `consecutive` accumulates before anything
    // else can go wrong.
    if (g_crash.magic != kMagic) {
        g_crash.magic = kMagic;
        g_crash.consecutive = 0;
    }
    if (g_crash.consecutive < 255u) {
        ++g_crash.consecutive;
    }

    alloy::fault::record& r = g_crash.rec;
    r.what = static_cast<alloy::fault::cause>(what);
    r.consecutive = static_cast<std::uint8_t>(g_crash.consecutive);
    r.pc = frame->pc;
    r.lr = frame->lr;
    r.psr = frame->psr;
    // The frame sits at the top of the stack the core was using, so its address
    // IS the stack pointer at the moment of the fault.
    r.sp = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(frame));
    r.status = 0;
    r.address = 0;
#if !defined(__ARM_ARCH_6M__)
    r.status = read32(kSCB_CFSR);
    if (r.status & kMMARVALID) {
        r.address = read32(kSCB_MMFAR);
    } else if (r.status & kBFARVALID) {
        r.address = read32(kSCB_BFAR);
    }
#endif

    // Reset. A dead device that reboots and explains itself beats a dead device.
    *reinterpret_cast<volatile std::uint32_t*>(kSCB_AIRCR) = kSysResetReq;
    for (;;) {
    }
}

// Naked so the stacked frame is untouched: the prologue a normal function would
// emit runs on whatever stack the fault left behind. EXC_RETURN bit 2 says which
// stack the core pushed onto (0 = MSP, 1 = PSP). The sequence is deliberately
// Thumb-1 only, so it assembles for ARMv6-M and ARMv7-M alike.
[[gnu::naked]] void HardFault_Handler() {
    __asm__ volatile(
        "movs r0, #4          \n"
        "mov  r1, lr          \n"
        "tst  r0, r1          \n"
        "beq  1f              \n"
        "mrs  r0, psp         \n"
        "b    2f              \n"
        "1:                   \n"
        "mrs  r0, msp         \n"
        "2:                   \n"
        "movs r1, #1          \n"   // cause::hard
        "ldr  r2, =alloy_fault_capture \n"
        "bx   r2              \n");
}

[[gnu::naked]] void NMI_Handler() {
    __asm__ volatile(
        "movs r0, #4          \n"
        "mov  r1, lr          \n"
        "tst  r0, r1          \n"
        "beq  1f              \n"
        "mrs  r0, psp         \n"
        "b    2f              \n"
        "1:                   \n"
        "mrs  r0, msp         \n"
        "2:                   \n"
        "movs r1, #2          \n"   // cause::nmi
        "ldr  r2, =alloy_fault_capture \n"
        "bx   r2              \n");
}

}  // extern "C"

namespace alloy::fault {

bool take(record& out) noexcept {
    if (g_crash.magic != kMagic || g_crash.rec.what == cause::none) {
        return false;
    }
    out = g_crash.rec;
    g_crash.rec.what = cause::none;  // reported once
    return true;
}

std::uint8_t consecutive() noexcept {
    return g_crash.magic == kMagic
               ? static_cast<std::uint8_t>(g_crash.consecutive)
               : std::uint8_t{0};
}

void healthy() noexcept {
    g_crash.magic = kMagic;
    g_crash.consecutive = 0;
}

[[noreturn]] void trigger() noexcept {
    // ICSR.NMIPENDSET — pend the NMI from software. Architectural on ARMv6-M
    // and ARMv7-M alike, so the same trigger serves the bench and the emulator.
    // Chosen over a wild jump or `udf` deliberately: those rely on fault
    // ESCALATION, which Renode does not model (see the header comment), while
    // a pended NMI takes the normal exception-entry path everywhere and runs
    // the identical capture/persist/reset sequence.
    constexpr std::uintptr_t kSCB_ICSR = 0xE000ED04u;
    constexpr std::uint32_t kNMIPendSet = 1u << 31;
    auto* const icsr = reinterpret_cast<volatile std::uint32_t*>(kSCB_ICSR);
    *icsr = kNMIPendSet;
    for (;;) {
        // The NMI preempts within a few instructions; waiting here keeps the
        // captured pc honest — it points at this wait, not at unrelated code.
        // The wait POLLS ICSR instead of parking in an empty `b .` self-loop,
        // and that is load-bearing in emulation: Renode takes a pended
        // exception at a translation-block boundary, and a branch-to-self is a
        // block that re-enters itself, observed to wedge the virtual core at
        // NMI entry (frame stacked, handler never executed, virtual time
        // free-running). A volatile MMIO read ends the block every iteration,
        // so the NMI is taken within a few instructions. On silicon the read
        // is harmless — ICSR is always readable — and the NMI preempts either
        // loop just the same.
        (void)*icsr;
    }
}

}  // namespace alloy::fault
