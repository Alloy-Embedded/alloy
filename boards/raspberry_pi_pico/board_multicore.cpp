// RP2040 multicore launch — SIO FIFO 5-word handshake (§2.8.2 of the TRM).
//
// Intentionally standalone: no device-descriptor or HAL includes. Only uses
// direct MMIO so this TU compiles without the full alloy device layer.

#include <cstdint>

namespace board {
void launch_core1(void (*fn)());
}

namespace {

// SIO (Single-cycle IO) inter-core FIFO registers
inline constexpr std::uint32_t kSioBase    = 0xD0000000u;
inline constexpr std::uint32_t kFifoStOff  = 0x050u;  // status
inline constexpr std::uint32_t kFifoWrOff  = 0x054u;  // TX
inline constexpr std::uint32_t kFifoRdOff  = 0x058u;  // RX
inline constexpr std::uint32_t kFifoVld    = 1u << 0; // RX has data
inline constexpr std::uint32_t kFifoRdy    = 1u << 1; // TX has space

// SCB VTOR — both cores share the same vector table (same flash)
inline constexpr std::uint32_t kScbVtor = 0xE000ED08u;

inline auto& sio_reg(std::uint32_t off) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(kSioBase + off);
}

// Drain the RX FIFO (discard stale words from prior launches / resets)
void fifo_drain() noexcept {
    while (sio_reg(kFifoStOff) & kFifoVld) {
        (void)sio_reg(kFifoRdOff);
    }
}

// Linker-defined top of the core 1 stack (.core1_stack section in rp2040.ld)
extern "C" std::uint32_t _core1_stack_top;

}  // namespace

namespace board {

void launch_core1(void (*fn)()) {
    // Read core 0's VTOR — core 1 will use the same vector table
    const std::uint32_t vtor =
        *reinterpret_cast<volatile std::uint32_t*>(kScbVtor);

    // 5-word launch sequence per RP2040 datasheet §2.8.2:
    //   [0]  0     — flush / reset core 1 state machine
    //   [1]  0     — second flush
    //   [2]  vtor  — vector table base address (SCB_VTOR for core 1)
    //   [3]  sp    — initial stack pointer
    //   [4]  pc    — entry point
    const std::uint32_t seq[5] = {
        0u,
        0u,
        vtor,
        reinterpret_cast<std::uint32_t>(&_core1_stack_top),
        reinterpret_cast<std::uint32_t>(fn),
    };

    std::uint32_t step = 0u;
    do {
        const std::uint32_t cmd = seq[step];

        if (cmd == 0u) {
            // Flush words tell core 1 to restart its state machine
            fifo_drain();
            __asm volatile("sev");  // wake core 1 if it is in WFE
        }

        // Wait for TX FIFO space, then push
        while (!(sio_reg(kFifoStOff) & kFifoRdy)) {
            __asm volatile("sev");
        }
        __asm volatile("dmb" ::: "memory");  // ensure write visible before wake
        sio_reg(kFifoWrOff) = cmd;
        __asm volatile("sev");

        // Wait for the echo from core 1
        while (!(sio_reg(kFifoStOff) & kFifoVld)) {
            __asm volatile("wfe");
        }
        const std::uint32_t response = sio_reg(kFifoRdOff);

        if (response == cmd) {
            ++step;
        } else {
            step = 0u;  // mismatch — restart handshake
        }
    } while (step < 5u);
}

}  // namespace board
