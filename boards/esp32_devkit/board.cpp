#include "board.hpp"
#include "board_uart_raw.hpp"

#include <cstdint>

namespace {

// Watchdog unlock key (same for TIMG and RTC WDTs)
inline constexpr std::uint32_t kWdtUnlockKey = 0x50D83AA1u;

inline auto& reg32(std::uint32_t addr) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(addr);
}

// Disable TIMG0 and TIMG1 main watchdogs + RTC watchdog.
// The ESP-IDF v5 bootloader leaves TIMG0 WDT running (~1s timeout).
// Without this, the chip resets before the second loop iteration.
void disable_watchdogs() noexcept {
    // Direct writes only — no read-modify-write on peripheral registers.
    // APP_CPU ROM polls DPORT/timer registers concurrently, creating a bus
    // race that corrupts the read half of any |= / &= on peripheral space.
    // Writing 0x00000000 to WDT_CONFIG0 with the protect register unlocked
    // clears WDTEN (bit 31) and disables all WDT stages — safe on all three WDTs.

    // TIMG0 WDT: WDTPROTECT=0x3FF5F064, WDTCONFIG0=0x3FF5F048 (WDTEN=bit31)
    reg32(0x3FF5F064u) = kWdtUnlockKey;
    reg32(0x3FF5F048u) = 0u;
    reg32(0x3FF5F064u) = 0u;

    // TIMG1 WDT: WDTPROTECT=0x3FF60064, WDTCONFIG0=0x3FF60048
    reg32(0x3FF60064u) = kWdtUnlockKey;
    reg32(0x3FF60048u) = 0u;
    reg32(0x3FF60064u) = 0u;

    // RTC WDT: WDTWPROTECT=RTC_CNTL_BASE+0xA4, WDTCONFIG0=RTC_CNTL_BASE+0x8C
    reg32(0x3FF480A4u) = kWdtUnlockKey;
    reg32(0x3FF4808Cu) = 0u;
    reg32(0x3FF480A4u) = 0u;

    // Note: ESP32 (LX6 original) does NOT have a Super WDT (SWD). SWD was
    // introduced in ESP32-S2/S3. No additional WDT registers to disable here.
}

// GPIO base: 0x3FF44000
// IO_MUX base: 0x3FF49000
inline constexpr std::uint32_t kGpioBase  = 0x3FF44000u;
inline constexpr std::uint32_t kIoMuxBase = 0x3FF49000u;

inline auto& gpio_reg(std::uint32_t offset) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(kGpioBase + offset);
}

// ESP32 IO_MUX is NOT sequential by GPIO number — each GPIO has a fixed offset.
// GPIO2: IO_MUX offset 0x40 (PERIPHS_IO_MUX_GPIO2_U per ESP32 TRM)
inline auto& iomux_gpio2() noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(kIoMuxBase + 0x40u);
}

// GPIO_FUNCn_OUT_SEL_CFG_REG = kGpioBase + 0x530 + n*4
inline auto& func_out_sel(std::uint32_t pin) noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(kGpioBase + 0x530u + pin * 4u);
}

inline constexpr std::uint32_t kLedPin    = 2u;    // GPIO2 built-in LED
inline constexpr std::uint32_t kMcuSelGpio = 2u;   // GPIO function in IO_MUX MCU_SEL
inline constexpr std::uint32_t kSigGpioOut = 256u; // SIG_GPIO_OUT_IDX

void gpio_output_init(std::uint32_t pin) noexcept {
    // IO_MUX: set MCU_SEL = 2 (GPIO function) at bits [12:10]
    iomux_gpio2() = (kMcuSelGpio << 10u);
    // Route to GPIO matrix software control
    func_out_sel(pin) = kSigGpioOut;
    // Enable output
    gpio_reg(0x24u) = (1u << pin);   // GPIO_ENABLE_W1TS_REG
}

void gpio_high(std::uint32_t pin) noexcept { gpio_reg(0x08u) = (1u << pin); }
void gpio_low (std::uint32_t pin) noexcept { gpio_reg(0x0Cu) = (1u << pin); }

static bool s_initialized = false;

}  // namespace

// DPORT inter-core mailbox registers (ESP32 TRM §4.12)
//   CTRL_A 0x3FF00038 — bit 0: APPCPU_RESETTING (assert then deassert)
//   CTRL_B 0x3FF0003C — bit 0: APPCPU_CLKGATE_EN
//   CTRL_C 0x3FF00040 — bit 0: APPCPU_RUNSTALL  (0 = running)
//   CTRL_D 0x3FF00044 — bits 31:0: entry point address read by ROM call_start_cpu1

inline constexpr std::uint32_t kAppcpuCtrlA = 0x3FF00038u;
inline constexpr std::uint32_t kAppcpuCtrlB = 0x3FF0003Cu;
inline constexpr std::uint32_t kAppcpuCtrlC = 0x3FF00040u;
inline constexpr std::uint32_t kAppcpuCtrlD = 0x3FF00044u;

// ASM trampoline defined in startup.S; sets up SP then calls _alloy_appcpu_user_entry
extern "C" void _alloy_appcpu_entry();

// User function pointer installed by start_app_cpu, called from the trampoline
static void (*s_appcpu_fn)() = nullptr;

// Called by _alloy_appcpu_entry after the APP_CPU stack is set up
extern "C" [[noreturn]] void _alloy_appcpu_user_entry() {
    if (s_appcpu_fn) s_appcpu_fn();
    for (;;) {}
}

namespace board {

namespace led {

void init()   { gpio_output_init(kLedPin); off(); }
void on()     { gpio_high(kLedPin); }
void off()    { gpio_low(kLedPin);  }
void toggle() {
    static bool state = false;
    state ? off() : on();
    state = !state;
}

}  // namespace led

void init() {
    if (s_initialized) { return; }
    disable_watchdogs();
    led::init();
    s_initialized = true;
}

// ============================================================
// APP_CPU bring-up — open issue (2026-04-26)
// ============================================================
// WHAT WORKS
//   PRO_CPU boots, runs, and heartbeats correctly.
//   The four fixes that got us here:
//     1. WDT disable: replaced RMW (|=) with direct write (= 0) on
//        WDTCONFIG0 to avoid DPORT bus race with APP_CPU ROM polling.
//     2. DPORT RMW: replaced all |= / &= on DPORT CTRL_A/B/C/D with
//        direct writes for the same reason.
//     3. RTC_CNTL dual stall: ROM leaves APP_CPU stalled via TWO
//        independent fields; both must be cleared:
//          RTC_CNTL_SW_CPU_STALL_REG (0x3FF480AC) bits[25:20] = C1
//          RTC_CNTL_OPTIONS0_REG     (0x3FF48000) bits[1:0]   = C0
//     4. PRO_CPU I-cache glitch: releasing APP_CPU from reset disturbs
//        the shared ESP32 cache controller for ~1 flash fetch cycle,
//        causing PRO_CPU to decode garbage as IllegalInstruction.
//        Fix: place start_app_cpu in IRAM (.iram1.text) and at the end
//        call Cache_Read_Disable(0)→Cache_Flush(0)→Cache_Read_Enable(0)
//        from IRAM before returning to flash-cached IROM.
//
// WHAT IS BROKEN
//   APP_CPU never executes _alloy_appcpu_entry (0x400804b0, IRAM).
//   The UART diagnostic "#1\n" at the very first instruction of the
//   trampoline never appears.
//
// HYPOTHESES (to investigate next)
//   A. ets_set_appcpu_boot_addr (ROM 0x4000689C) writes a DRAM global
//      that ROM call_start_cpu1 reads.  call_start_cpu1 may require the
//      address to be in IROM range (0x400D0000+) and silently discards
//      or re-maps IRAM addresses (0x40080000+).  Evidence: trampoline
//      in IROM (0x400d04b4) DID dispatch; in IRAM (0x400804b0) does NOT.
//   B. Alternatively, call_start_cpu1 may need a separate handshake
//      (interrupt or flag) beyond just the CTRL_D write or boot-addr
//      global — something the ESP-IDF startup.c sends but we don't.
//   C. Direct CTRL_D write (reg32(0x3FF00044) = &entry) bypasses ROM's
//      dispatch path entirely but then PRO_CPU's cache reset sequence
//      fails (tested: PRO_CPU faults on return to IROM).  The two
//      mechanisms may be mutually exclusive.
//
// NEXT STEPS
//   1. Place a tiny IROM-resident relay stub (fixed VMA in .text) that
//      ets_set_appcpu_boot_addr targets; stub immediately jumps to
//      the IRAM trampoline.  This tests hypothesis A.
//   2. Inspect ROM call_start_cpu1 disassembly (starting at ROM addr
//      found via IDF's rom.ld / esp32_rom.ld) for range checks on the
//      boot address or additional flag requirements.
//   3. Try writing CTRL_D directly (hypothesis C) but perform the cache
//      reset before deassert CTRL_A rather than after.
// ============================================================

[[gnu::section(".iram1.text")]] void start_app_cpu(void (*fn)()) {
    s_appcpu_fn = fn;

    // Set APP_CPU entry via ROM helper.
    // ets_set_appcpu_boot_addr (ROM 0x4000689C) writes the global that
    // ROM's call_start_cpu1 reads on APP_CPU side before jumping to user code.
    // NOTE: This dispatches correctly when entry is in IROM (0x400D0000+).
    //       With entry in IRAM (0x40080400+) dispatch does NOT occur — see
    //       open issue above.
    auto ets_set_appcpu_boot_addr =
        reinterpret_cast<void(*)(std::uint32_t)>(0x4000689Cu);
    ets_set_appcpu_boot_addr(
        reinterpret_cast<std::uint32_t>(&_alloy_appcpu_entry));

    // Release RTC_CNTL software stall on APP_CPU (ROM leaves it asserted).
    //   RTC_CNTL_SW_CPU_STALL_REG 0x3FF480AC bits[25:20] SW_STALL_APPCPU_C1
    //   RTC_CNTL_OPTIONS0_REG     0x3FF48000 bits[1:0]   SW_STALL_APPCPU_C0
    // Both must be cleared — clearing only one is insufficient.
    reg32(0x3FF480ACu) &= ~(0x3Fu << 20u);
    reg32(0x3FF48000u) &= ~0x3u;

    // DPORT bring-up (direct writes only — no RMW, see fix #1/#2 above).
    reg32(kAppcpuCtrlB) = 1u;   // APPCPU_CLKGATE_EN
    reg32(kAppcpuCtrlC) = 0u;   // APPCPU_RUNSTALL = 0 (running)
    reg32(kAppcpuCtrlA) = 1u;   // APPCPU_RESETTING assert
    reg32(kAppcpuCtrlA) = 0u;   // APPCPU_RESETTING deassert → core released
    asm volatile("memw" ::: "memory");

    // Reset PRO_CPU I-cache from IRAM before returning to flash-cached IROM.
    // Releasing APP_CPU disturbs the shared cache controller; without this,
    // PRO_CPU's first post-return fetch decodes as garbage → IllegalInstruction.
    // ROM addresses verified against esp-idf v5 rom.ld for ESP32 (LX6):
    //   Cache_Read_Disable 0x40009AB8
    //   Cache_Flush        0x40009A14
    //   Cache_Read_Enable  0x40009A84
    auto Cache_Read_Disable = reinterpret_cast<void(*)(std::uint32_t)>(0x40009AB8u);
    auto Cache_Flush        = reinterpret_cast<void(*)(std::uint32_t)>(0x40009A14u);
    auto Cache_Read_Enable  = reinterpret_cast<int(*)(std::uint32_t)>(0x40009A84u);
    Cache_Read_Disable(0);
    Cache_Flush(0);
    Cache_Read_Enable(0);
}

}  // namespace board
