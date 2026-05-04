/// @file hal/rtc/lite.hpp
/// Real-Time Clock (RTC) HAL — alloy.device.v2.1 lite driver.
///
/// Supports two STM32 RTC register layout variants:
///
///   Layout A — "classic"  (G4 / L4 / F3 / WB)
///     kIpVersion contains "rtc2_v3_3" (or rtc2_v3_2 / rtc2_v3_1)
///     Offsets: TR=0x00 DR=0x04 CR=0x08 ISR=0x0C PRER=0x10 WUTR=0x14
///              ALRMAR=0x18 ALRMBR=0x1C WPR=0x20 SSR=0x24
///              BKPRx=0x50 + 4×n
///
///   Layout B — "modern"   (G0 / H7 / U5 / WL)
///     kIpVersion contains "rtc2_v3_5" (or rtc2_v3_6)
///     Offsets: TR=0x00 DR=0x04 SSR=0x08 ICSR=0x0C PRER=0x10 WUTR=0x14
///              CR=0x18 WPR=0x1C ALRMAR=0x34 ALRMBR=0x3C
///              SR=0x44 SCR=0x4C  BKPRx=0x100 + 4×n
///
/// The driver selects the correct offsets at compile time via if constexpr.
///
/// IMPORTANT: the RTC clock source (LSE, LSI, or HSE/32) must be configured
/// and enabled via RCC before calling `configure()`.  Also ensure the PWR
/// backup domain write-protection is disabled:
/// @code
///   // Enable LSE and select it as RTC clock (done once after power-on)
///   // Then:
///   dev::peripheral_on<dev::rtc>();  // enable RTC APB clock
///   using Rtc = alloy::hal::rtc::lite::port<dev::rtc>;
///   Rtc::configure();  // 32.768 kHz LSE, default prescalers → 1 Hz
///   Rtc::set_time({.hours=12, .minutes=0, .seconds=0});
///   Rtc::set_date({.year=25, .month=1, .day=1, .weekday=3});  // 2025-01-01 Wed
/// @endcode
#pragma once

#include <concepts>
#include <cstdint>
#include <string_view>

#include "device/concepts.hpp"
#include "device/rcc_gate_table.hpp"

namespace alloy::hal::rtc::lite {

// ============================================================================
// Detail
// ============================================================================

namespace detail {

// ---------------------------------------------------------------------------
// Layout A offsets (G4 / L4 / F3 / WB — "rtc2_v3_3...")
// ---------------------------------------------------------------------------
inline constexpr std::uintptr_t kA_Tr     = 0x00u;
inline constexpr std::uintptr_t kA_Dr     = 0x04u;
inline constexpr std::uintptr_t kA_Cr     = 0x08u;
inline constexpr std::uintptr_t kA_Isr    = 0x0Cu;
inline constexpr std::uintptr_t kA_Prer   = 0x10u;
inline constexpr std::uintptr_t kA_Wutr   = 0x14u;
inline constexpr std::uintptr_t kA_Alrmar = 0x18u;
inline constexpr std::uintptr_t kA_Alrmbr = 0x1Cu;
inline constexpr std::uintptr_t kA_Wpr    = 0x20u;
inline constexpr std::uintptr_t kA_Ssr    = 0x24u;
inline constexpr std::uintptr_t kA_Bkpr0  = 0x50u;  ///< BKP0R; BKPxR = 0x50 + 4×x

// ---------------------------------------------------------------------------
// Layout B offsets (G0 / H7 / U5 / WL — "rtc2_v3_5...")
// ---------------------------------------------------------------------------
inline constexpr std::uintptr_t kB_Tr     = 0x00u;
inline constexpr std::uintptr_t kB_Dr     = 0x04u;
inline constexpr std::uintptr_t kB_Ssr    = 0x08u;
inline constexpr std::uintptr_t kB_Icsr   = 0x0Cu;  ///< renamed ISR in layout B
inline constexpr std::uintptr_t kB_Prer   = 0x10u;
inline constexpr std::uintptr_t kB_Wutr   = 0x14u;
inline constexpr std::uintptr_t kB_Cr     = 0x18u;
inline constexpr std::uintptr_t kB_Wpr    = 0x1Cu;
inline constexpr std::uintptr_t kB_Alrmar = 0x34u;
inline constexpr std::uintptr_t kB_Alrmbr = 0x3Cu;
inline constexpr std::uintptr_t kB_Sr     = 0x44u;  ///< flags only (read-only)
inline constexpr std::uintptr_t kB_Scr    = 0x4Cu;  ///< status clear (write-1-to-clear)
inline constexpr std::uintptr_t kB_Bkpr0  = 0x100u;

// ---------------------------------------------------------------------------
// Common register bits
// ---------------------------------------------------------------------------

// WPR write-protection key sequence
inline constexpr std::uint32_t kWprUnlock1 = 0xCAu;
inline constexpr std::uint32_t kWprUnlock2 = 0x53u;
inline constexpr std::uint32_t kWprLock    = 0xFFu;

// ISR / ICSR — init and sync bits (layout A / B differ in which register,
// but the bit positions for INIT/INITF/RSF are the same)
inline constexpr std::uint32_t kIsrRsf    = 1u << 5;  ///< Register sync flag
inline constexpr std::uint32_t kIsrInitf  = 1u << 6;  ///< Init mode flag (read-only)
inline constexpr std::uint32_t kIsrInit   = 1u << 7;  ///< Init mode request (R/W)

// Layout A ISR alarm/wakeup flags (bits 8–10)
inline constexpr std::uint32_t kIsrAlraf  = 1u << 8;  ///< Alarm A flag
inline constexpr std::uint32_t kIsrAlrbf  = 1u << 9;  ///< Alarm B flag
inline constexpr std::uint32_t kIsrWutf   = 1u << 10; ///< Wakeup timer flag

// Layout B SR flags (same bit positions as ISR flags above but in SR register)
// Layout B SCR clear bits (write 1 to clear)
inline constexpr std::uint32_t kScrCAlraf = 1u << 0;  ///< Clear ALRAF
inline constexpr std::uint32_t kScrCAlrbf = 1u << 1;  ///< Clear ALRBF
inline constexpr std::uint32_t kScrCWutf  = 1u << 2;  ///< Clear WUTF

// CR bits (same offset in both layouts)
inline constexpr std::uint32_t kCrAlrae   = 1u << 8;  ///< Alarm A enable
inline constexpr std::uint32_t kCrAlrbe   = 1u << 9;  ///< Alarm B enable
inline constexpr std::uint32_t kCrWute    = 1u << 10; ///< Wakeup timer enable
inline constexpr std::uint32_t kCrAlraie  = 1u << 12; ///< Alarm A interrupt enable
inline constexpr std::uint32_t kCrAlrbie  = 1u << 13; ///< Alarm B interrupt enable
inline constexpr std::uint32_t kCrWutie   = 1u << 14; ///< Wakeup timer interrupt enable

// TR — time register fields
inline constexpr std::uint32_t kTrSuMask  = 0xFu;       ///< Seconds units BCD [3:0]
inline constexpr std::uint32_t kTrStMask  = 0x7u << 4;  ///< Seconds tens  BCD [6:4]
inline constexpr std::uint32_t kTrMnuMask = 0xFu << 8;  ///< Minutes units BCD [11:8]
inline constexpr std::uint32_t kTrMntMask = 0x7u << 12; ///< Minutes tens  BCD [14:12]
inline constexpr std::uint32_t kTrHuMask  = 0xFu << 16; ///< Hours units   BCD [19:16]
inline constexpr std::uint32_t kTrHtMask  = 0x3u << 20; ///< Hours tens    BCD [21:20]
inline constexpr std::uint32_t kTrPm      = 1u << 22;   ///< PM indicator

// DR — date register fields
inline constexpr std::uint32_t kDrDuMask  = 0xFu;        ///< Day units   BCD [3:0]
inline constexpr std::uint32_t kDrDtMask  = 0x3u << 4;   ///< Day tens    BCD [5:4]
inline constexpr std::uint32_t kDrMuMask  = 0xFu << 8;   ///< Month units BCD [11:8]
inline constexpr std::uint32_t kDrMtMask  = 1u   << 12;  ///< Month tens  BCD [12]
inline constexpr std::uint32_t kDrWduMask = 0x7u << 13;  ///< Weekday     [15:13]
inline constexpr std::uint32_t kDrYuMask  = 0xFu << 16;  ///< Year units  BCD [19:16]
inline constexpr std::uint32_t kDrYtMask  = 0xFu << 20;  ///< Year tens   BCD [23:20]

// BCD helpers
[[nodiscard]] constexpr std::uint8_t to_bcd(std::uint8_t dec) noexcept {
    return static_cast<std::uint8_t>(((dec / 10u) << 4u) | (dec % 10u));
}
[[nodiscard]] constexpr std::uint8_t from_bcd(std::uint8_t bcd) noexcept {
    return static_cast<std::uint8_t>((bcd >> 4u) * 10u + (bcd & 0xFu));
}

// Layout detection
[[nodiscard]] consteval bool is_rtc_layout_b(const char* ip) {
    // Layout B: G0/H7/U5/WL (rtc2_v3_5, rtc2_v3_6, ...)
    const std::string_view sv{ip};
    return sv.find("rtc2_v3_5") != std::string_view::npos ||
           sv.find("rtc2_v3_6") != std::string_view::npos;
}

[[nodiscard]] consteval bool is_rtc_v2(const char* tmpl) {
    return std::string_view{tmpl} == "rtc";
}

}  // namespace detail

// ============================================================================
// Concept
// ============================================================================

template <typename P>
concept StRtcV2 =
    device::PeripheralSpec<P> &&
    requires {
        { P::kTemplate  } -> std::convertible_to<const char*>;
        { P::kIpVersion } -> std::convertible_to<const char*>;
    } &&
    detail::is_rtc_v2(P::kTemplate);

// ============================================================================
// Data structures
// ============================================================================

/// 24-hour time (decoded from BCD register values).
struct Time {
    std::uint8_t hours{0};    ///< 0–23
    std::uint8_t minutes{0};  ///< 0–59
    std::uint8_t seconds{0};  ///< 0–59
};

/// Calendar date (decoded from BCD register values).
struct Date {
    std::uint8_t year{0};     ///< 0–99  (last two digits; 25 = 2025)
    std::uint8_t month{1};    ///< 1–12
    std::uint8_t day{1};      ///< 1–31
    std::uint8_t weekday{1};  ///< 1=Monday … 7=Sunday (per RTC hardware encoding)
};

// ============================================================================
// port<P>
// ============================================================================

/// Direct-MMIO RTC driver.  P must satisfy StRtcV2.
///
/// All time/date values are in plain decimal (not BCD).  The driver handles
/// BCD conversion internally.
///
/// Write-protection: the RTC has a hardware write-protection mechanism.
/// All write operations in this driver unlock WPR first and re-lock after.
template <typename P>
    requires StRtcV2<P>
class port {
   public:
    static constexpr std::uintptr_t kBase = P::kBaseAddress;

    /// True when the peripheral uses layout B (G0/H7/U5 modern register map).
    static constexpr bool kLayoutB = detail::is_rtc_layout_b(P::kIpVersion);

   private:
    [[nodiscard]] static auto reg(std::uintptr_t ofs) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(kBase + ofs);
    }

    // Layout-aware register helpers
    [[nodiscard]] static auto cr() noexcept -> volatile std::uint32_t& {
        if constexpr (kLayoutB) { return reg(detail::kB_Cr); }
        else                    { return reg(detail::kA_Cr); }
    }
    [[nodiscard]] static auto isr_or_icsr() noexcept -> volatile std::uint32_t& {
        if constexpr (kLayoutB) { return reg(detail::kB_Icsr); }
        else                    { return reg(detail::kA_Isr); }
    }
    [[nodiscard]] static auto wpr() noexcept -> volatile std::uint32_t& {
        if constexpr (kLayoutB) { return reg(detail::kB_Wpr); }
        else                    { return reg(detail::kA_Wpr); }
    }
    [[nodiscard]] static auto prer() noexcept -> volatile std::uint32_t& {
        return reg(detail::kA_Prer);  // same offset in both layouts
    }
    [[nodiscard]] static auto wutr() noexcept -> volatile std::uint32_t& {
        return reg(detail::kA_Wutr);  // same offset
    }
    [[nodiscard]] static auto alrmar() noexcept -> volatile std::uint32_t& {
        if constexpr (kLayoutB) { return reg(detail::kB_Alrmar); }
        else                    { return reg(detail::kA_Alrmar); }
    }

    // -----------------------------------------------------------------------
    // Write-protection helpers (internal)
    // -----------------------------------------------------------------------

    static void unlock() noexcept {
        wpr() = detail::kWprUnlock1;
        wpr() = detail::kWprUnlock2;
    }
    static void lock() noexcept {
        wpr() = detail::kWprLock;
    }

    // -----------------------------------------------------------------------
    // Init mode helpers (internal — must be called while WPR unlocked)
    // -----------------------------------------------------------------------

    static void enter_init() noexcept {
        isr_or_icsr() |= detail::kIsrInit;
        while ((isr_or_icsr() & detail::kIsrInitf) == 0u) { /* spin */ }
    }
    static void exit_init() noexcept {
        isr_or_icsr() &= ~detail::kIsrInit;
    }

   public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /// Configure the RTC prescalers for a 1 Hz calendar clock.
    ///
    /// For 32.768 kHz LSE (typical crystal): prediv_a=127, prediv_s=255.
    ///   f_ck_spre = f_rtcclk / ((prediv_s + 1) × (prediv_a + 1))
    ///             = 32768 / (256 × 128) = 1 Hz
    ///
    /// For ~37 kHz LSI:  prediv_a=127, prediv_s=289  (approximate).
    ///
    /// @param prediv_a  Asynchronous prescaler divider  (7-bit, 0–127).
    /// @param prediv_s  Synchronous prescaler divider  (15-bit, 0–32767).
    static void configure(std::uint8_t  prediv_a = 127u,
                          std::uint16_t prediv_s = 255u) noexcept {
        unlock();
        enter_init();
        // PRER: PREDIV_A[22:16], PREDIV_S[14:0]
        prer() = (static_cast<std::uint32_t>(prediv_a) << 16u) |
                 static_cast<std::uint32_t>(prediv_s);
        exit_init();
        lock();
    }

    // -----------------------------------------------------------------------
    // Time
    // -----------------------------------------------------------------------

    /// Write the time registers (TR).  Operates in 24-hour mode.
    ///
    /// Enters init mode automatically; the calendar keeps counting after exit.
    static void set_time(const Time& t) noexcept {
        const auto su = detail::to_bcd(t.seconds) & 0xFu;
        const auto st = (detail::to_bcd(t.seconds) >> 4u) & 0x7u;
        const auto mn = detail::to_bcd(t.minutes) & 0xFu;
        const auto mt = (detail::to_bcd(t.minutes) >> 4u) & 0x7u;
        const auto hu = detail::to_bcd(t.hours) & 0xFu;
        const auto ht = (detail::to_bcd(t.hours) >> 4u) & 0x3u;
        const std::uint32_t tr =
            su | (st << 4u) | (mn << 8u) | (mt << 12u) |
            (hu << 16u) | (ht << 20u);
        unlock();
        enter_init();
        if constexpr (kLayoutB) {
            reg(detail::kB_Tr) = tr;
        } else {
            reg(detail::kA_Tr) = tr;
        }
        exit_init();
        lock();
    }

    /// Read and decode the current time from TR.
    ///
    /// Uses shadow registers (reads TR then DR to unlock the shadow latch on
    /// layout A hardware).  If BYPSHAD is not set, the values are stable.
    [[nodiscard]] static auto get_time() noexcept -> Time {
        // Wait for shadow registers to be synchronised.
        wait_sync();
        const std::uint32_t tr = (kLayoutB) ? reg(detail::kB_Tr) : reg(detail::kA_Tr);
        Time t{};
        const auto raw_s = static_cast<std::uint8_t>(
            ((tr >> 4u) & 0x7u) * 10u + (tr & 0xFu));
        const auto raw_m = static_cast<std::uint8_t>(
            ((tr >> 12u) & 0x7u) * 10u + ((tr >> 8u) & 0xFu));
        const auto raw_h = static_cast<std::uint8_t>(
            ((tr >> 20u) & 0x3u) * 10u + ((tr >> 16u) & 0xFu));
        t.seconds = raw_s;
        t.minutes = raw_m;
        t.hours   = raw_h;
        return t;
    }

    // -----------------------------------------------------------------------
    // Date
    // -----------------------------------------------------------------------

    /// Write the date registers (DR).
    ///
    /// @param d  Date with decimal fields.  weekday: 1=Monday … 7=Sunday.
    static void set_date(const Date& d) noexcept {
        const auto du  = detail::to_bcd(d.day)   & 0xFu;
        const auto dt  = (detail::to_bcd(d.day)   >> 4u) & 0x3u;
        const auto mu  = detail::to_bcd(d.month)  & 0xFu;
        const auto mt  = (detail::to_bcd(d.month) >> 4u) & 0x1u;
        const auto wdu = static_cast<std::uint32_t>(d.weekday) & 0x7u;
        const auto yu  = detail::to_bcd(d.year)   & 0xFu;
        const auto yt  = (detail::to_bcd(d.year)  >> 4u) & 0xFu;
        const std::uint32_t dr =
            du | (dt << 4u) | (mu << 8u) | (mt << 12u) |
            (wdu << 13u) | (yu << 16u) | (yt << 20u);
        unlock();
        enter_init();
        if constexpr (kLayoutB) {
            reg(detail::kB_Dr) = dr;
        } else {
            reg(detail::kA_Dr) = dr;
        }
        exit_init();
        lock();
    }

    /// Read and decode the current date from DR.
    [[nodiscard]] static auto get_date() noexcept -> Date {
        wait_sync();
        const std::uint32_t dr = (kLayoutB) ? reg(detail::kB_Dr) : reg(detail::kA_Dr);
        Date d{};
        d.day     = static_cast<std::uint8_t>(((dr >> 4u) & 0x3u) * 10u + (dr & 0xFu));
        d.month   = static_cast<std::uint8_t>(((dr >> 12u) & 0x1u) * 10u + ((dr >> 8u) & 0xFu));
        d.weekday = static_cast<std::uint8_t>((dr >> 13u) & 0x7u);
        d.year    = static_cast<std::uint8_t>(((dr >> 20u) & 0xFu) * 10u + ((dr >> 16u) & 0xFu));
        return d;
    }

    // -----------------------------------------------------------------------
    // Sync
    // -----------------------------------------------------------------------

    /// Wait for the shadow registers to synchronise with the RTC counters.
    ///
    /// The hardware sets ISR.RSF one clock cycle after initialisation or
    /// wakeup from Stop.  After the sync, TR and DR are stable to read.
    static void wait_sync() noexcept {
        // Clear RSF, then wait for it to be set again by hardware.
        unlock();
        isr_or_icsr() &= ~detail::kIsrRsf;
        lock();
        while ((isr_or_icsr() & detail::kIsrRsf) == 0u) { /* spin */ }
    }

    // -----------------------------------------------------------------------
    // Alarm A
    // -----------------------------------------------------------------------

    /// Configure Alarm A to fire at a specific time (hours, minutes, seconds).
    ///
    /// Sets the ALRMAR register with mask bits cleared for H/M/S comparison
    /// and the date/weekday match disabled (MSK4=1).  The alarm fires once
    /// per day when the time matches.
    ///
    /// Disable the alarm before reconfiguring (hardware requirement).
    static void set_alarm_a(const Time& t) noexcept {
        const auto su = detail::to_bcd(t.seconds) & 0xFu;
        const auto st = (detail::to_bcd(t.seconds) >> 4u) & 0x7u;
        const auto mn = detail::to_bcd(t.minutes) & 0xFu;
        const auto mt = (detail::to_bcd(t.minutes) >> 4u) & 0x7u;
        const auto hu = detail::to_bcd(t.hours) & 0xFu;
        const auto ht = (detail::to_bcd(t.hours) >> 4u) & 0x3u;
        // MSK4=1 (ignore date), MSK3..1=0 (match H/M/S)
        const std::uint32_t alrmar =
            su | (st << 4u) | (mn << 8u) | (mt << 12u) |
            (hu << 16u) | (ht << 20u) | (1u << 31u); // MSK4
        unlock();
        alrmar() = alrmar;
        lock();
    }

    /// Enable Alarm A and its interrupt (CR.ALRAE + CR.ALRAIE).
    ///
    /// Also enable the NVIC IRQ for RTC_Alarm and the EXTI line (family-
    /// specific — done externally).
    static void enable_alarm_a() noexcept {
        unlock();
        cr() |= detail::kCrAlrae | detail::kCrAlraie;
        lock();
    }

    /// Disable Alarm A and wait for the write-access flag (ALRAWF).
    static void disable_alarm_a() noexcept {
        unlock();
        cr() &= ~(detail::kCrAlrae | detail::kCrAlraie);
        // Wait until the alarm registers are writable again (ALRAWF=1 in ISR/ICSR).
        while ((isr_or_icsr() & 0x1u) == 0u) { /* spin — ALRAWF bit 0 */ }
        lock();
    }

    /// True if Alarm A has fired (ALRAF flag).
    [[nodiscard]] static auto alarm_a_pending() noexcept -> bool {
        if constexpr (kLayoutB) {
            return (reg(detail::kB_Sr) & 0x1u) != 0u;   // SR.ALRAF = bit 0
        } else {
            return (isr_or_icsr() & detail::kIsrAlraf) != 0u;
        }
    }

    /// Clear the Alarm A flag.
    static void clear_alarm_a_flag() noexcept {
        if constexpr (kLayoutB) {
            reg(detail::kB_Scr) = detail::kScrCAlraf;
        } else {
            unlock();
            isr_or_icsr() &= ~detail::kIsrAlraf;
            lock();
        }
    }

    // -----------------------------------------------------------------------
    // Backup registers
    // -----------------------------------------------------------------------

    /// Write to backup register `idx` (0-based; 20 registers on most devices).
    ///
    /// Backup registers are powered by VBAT and survive system reset and
    /// Stop/Standby.  Useful for storing boot flags or calibration data.
    static void write_backup(std::uint8_t idx, std::uint32_t val) noexcept {
        const auto base = kLayoutB ? detail::kB_Bkpr0 : detail::kA_Bkpr0;
        reg(base + static_cast<std::uintptr_t>(idx) * 4u) = val;
    }

    /// Read backup register `idx`.
    [[nodiscard]] static auto read_backup(std::uint8_t idx) noexcept
        -> std::uint32_t {
        const auto base = kLayoutB ? detail::kB_Bkpr0 : detail::kA_Bkpr0;
        return reg(base + static_cast<std::uintptr_t>(idx) * 4u);
    }

    // -----------------------------------------------------------------------
    // Device-data bridge — sourced from alloy.device.v2.1 flat-struct
    // -----------------------------------------------------------------------

    /// Returns the NVIC IRQ line for this peripheral.
    /// Sourced from `P::kIrqLines[idx]` (flat-struct v2.1).
    /// RTC typically exposes two IRQ lines: alarm and wakeup.
    /// Compiles to a constexpr constant — zero overhead.
    [[nodiscard]] static constexpr auto irq_number(std::size_t idx = 0u) noexcept
        -> std::uint32_t {
        if constexpr (requires { P::kIrqLines[0]; }) {
            return static_cast<std::uint32_t>(P::kIrqLines[idx]);
        } else {
            static_assert(sizeof(P) == 0,
                "P::kIrqLines not present; upgrade device artifact to v2.1");
            return 0u;
        }
    }

    /// Returns the number of IRQ lines for this peripheral.
    /// Sourced from `P::kIrqCount` (flat-struct v2.1). Returns 0 if absent.
    [[nodiscard]] static constexpr auto irq_count() noexcept -> std::size_t {
        if constexpr (requires { P::kIrqCount; }) {
            return static_cast<std::size_t>(P::kIrqCount);
        }
        return 0u;
    }

    // -----------------------------------------------------------------------
    // Clock gate — sourced from alloy.device.v2.1 flat-struct kRccEnable
    // -----------------------------------------------------------------------

    /// Enable the peripheral clock (APBx/AHBx ENR bit).
    ///
    /// Requires `ALLOY_DEVICE_RCC_TABLE_AVAILABLE` at build time (set by the
    /// `alloy_device_rcc_table` CMake target) to actually write the register.
    static void clock_on() noexcept
        requires (requires { P::kRccEnable; })
    {
#if defined(ALLOY_DEVICE_RCC_TABLE_AVAILABLE)
        constexpr auto gate = device::detail::find_rcc_gate(P::kRccEnable);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *reinterpret_cast<volatile std::uint32_t*>(gate.addr) |= gate.mask;
#endif
    }

    /// Disable the peripheral clock.
    static void clock_off() noexcept
        requires (requires { P::kRccEnable; })
    {
#if defined(ALLOY_DEVICE_RCC_TABLE_AVAILABLE)
        constexpr auto gate = device::detail::find_rcc_gate(P::kRccEnable);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *reinterpret_cast<volatile std::uint32_t*>(gate.addr) &= ~gate.mask;
#endif
    }

    port() = delete;
};

}  // namespace alloy::hal::rtc::lite
