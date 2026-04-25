#pragma once

// alloy::hal::mdio::Same70Mdio — SAME70 GMAC management port (MDIO) driver.
//
// Wraps the SAME70 GMAC NCR/NCFGR/NSR/MAN registers to implement the
// MdioBus concept for Clause-22 (10/100 Ethernet PHY management).
//
// Prerequisites (caller's responsibility):
//   1. Enable GMAC peripheral clock: PMC_PCER1 |= (1u << 5).
//   2. Mux MDIO pins: PD8 = MDC (PERIPH_A), PD9 = MDIO (PERIPH_A).
//   3. Call enable_management() before any read/write.
//
// The Same70Mdio driver does NOT own the GMAC peripheral. Same70Gmac (the
// MAC driver) uses a non-overlapping subset of GMAC registers; the two can
// coexist when initialised in the documented order:
//   1. Same70Mdio::enable_management()
//   2. KSZ8081::init() — PHY soft-reset + auto-neg via MDIO
//   3. Same70Gmac::init()  — MAC DMA setup (does not touch MAN/NSR/NCR.MPE)
//
// Usage:
//   Same70Mdio mdio;
//   mdio.enable_management();
//   ksz8081::Device phy{mdio};
//   phy.init();

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "src/hal/mdio/mdio.hpp"

namespace alloy::hal::mdio {

class Same70Mdio {
   public:
    // mdc_divider_bits: value written to GMAC_NCFGR.CLK[2:0] field.
    //   0b000 = MCK/8   (19 MHz @ 150 MHz — exceeds KSZ8081 2.5 MHz limit)
    //   0b001 = MCK/16  (9.4 MHz — too fast)
    //   0b010 = MCK/32  (4.7 MHz — slightly over)
    //   0b011 = MCK/48  (3.1 MHz — over)
    //   0b100 = MCK/64  (2.34 MHz — safe default for KSZ8081 at 150 MHz MCK)
    //   0b101 = MCK/96  (1.56 MHz — conservative)
    // At 150 MHz MCK, use 0b100 (MCK/64 = 2.34 MHz < 2.5 MHz limit).
    explicit constexpr Same70Mdio(std::uint8_t mdc_divider_bits = 0b100u) noexcept
        : mdc_divider_bits_{mdc_divider_bits} {}

    // Enable the GMAC management port (NCR.MPE bit).
    // Programs the MDC clock divider in NCFGR.CLK before enabling.
    void enable_management() noexcept {
        // Set MDC clock divider: NCFGR bits [20:18].
        volatile auto* ncfgr = reg(kNcfgrOffset);
        const std::uint32_t cur = *ncfgr;
        *ncfgr = (cur & ~(0x7u << 18u)) | ((std::uint32_t{mdc_divider_bits_} & 0x7u) << 18u);
        // Enable management port.
        volatile auto* ncr = reg(kNcrOffset);
        *ncr = *ncr | kNcrMpe;
    }

    // Disable the GMAC management port (clears NCR.MPE).
    void disable_management() noexcept {
        volatile auto* ncr = reg(kNcrOffset);
        *ncr = *ncr & ~kNcrMpe;
    }

    // Read a Clause-22 PHY register.
    [[nodiscard]] core::Result<std::uint16_t, core::ErrorCode>
    read(std::uint8_t phy, std::uint8_t reg_addr) const noexcept {
        if (!wait_idle()) return core::Err(core::ErrorCode::Timeout);

        const std::uint32_t frame =
            kManSof                                      // SOF  = 0b01
            | (0x2u << 28u)                              // OP   = 0b10 (read)
            | ((std::uint32_t{phy}      & 0x1Fu) << 23u)
            | ((std::uint32_t{reg_addr} & 0x1Fu) << 18u)
            | kManTa;                                    // TA   = 0b10

        *reg(kManOffset) = frame;

        if (!wait_idle()) return core::Err(core::ErrorCode::Timeout);

        const std::uint16_t data =
            static_cast<std::uint16_t>(*reg(kManOffset) & 0xFFFFu);
        return core::Ok(data);
    }

    // Write a Clause-22 PHY register.
    [[nodiscard]] core::Result<void, core::ErrorCode>
    write(std::uint8_t phy, std::uint8_t reg_addr, std::uint16_t value) noexcept {
        if (!wait_idle()) return core::Err(core::ErrorCode::Timeout);

        const std::uint32_t frame =
            kManSof                                      // SOF  = 0b01
            | (0x1u << 28u)                              // OP   = 0b01 (write)
            | ((std::uint32_t{phy}      & 0x1Fu) << 23u)
            | ((std::uint32_t{reg_addr} & 0x1Fu) << 18u)
            | kManTa                                     // TA   = 0b10
            | std::uint32_t{value};

        *reg(kManOffset) = frame;

        if (!wait_idle()) return core::Err(core::ErrorCode::Timeout);
        return core::Ok();
    }

   private:
    static constexpr std::uint32_t kGmacBase    = 0x40050000u;
    static constexpr std::uint32_t kNcrOffset   = 0x00u;
    static constexpr std::uint32_t kNcfgrOffset = 0x04u;
    static constexpr std::uint32_t kNsrOffset   = 0x08u;
    static constexpr std::uint32_t kManOffset   = 0x34u;

    static constexpr std::uint32_t kNcrMpe  = 1u << 4u;  // management port enable
    static constexpr std::uint32_t kNsrIdle = 1u << 2u;  // MDIO idle
    static constexpr std::uint32_t kManSof  = 0x1u << 30u;
    static constexpr std::uint32_t kManTa   = 0x2u << 16u;

    static constexpr std::uint32_t kIdleMaxIterations = 10'000u;

    std::uint8_t mdc_divider_bits_;

    [[nodiscard]] static volatile std::uint32_t* reg(std::uint32_t offset) noexcept {
        return reinterpret_cast<volatile std::uint32_t*>(kGmacBase + offset);
    }

    [[nodiscard]] bool wait_idle() const noexcept {
        for (std::uint32_t i = 0u; i < kIdleMaxIterations; ++i) {
            if (*reg(kNsrOffset) & kNsrIdle) return true;
        }
        return false;
    }
};

static_assert(MdioBus<Same70Mdio>);

}  // namespace alloy::hal::mdio
