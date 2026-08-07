// Flash program/erase for the STM32L4 embedded flash (flash_l4 IP).
//
// BEHAVIOR only: register overlay, gate and the flash MEMORY geometry
// (mem_base/mem_size) come from generated data. RM0394 §3: unlock KEYR with
// the same two magic keys as G0, erase whole 2 KB pages (CR.PER + PNB + BKER,
// START), program one 64-bit double-word at a time (CR.PG, two word writes,
// the second starts the operation). Structurally the G0 driver's sibling —
// what moved: PNB is 8 bits at [10:3], BKER sits at bit 11, START at bit 16,
// and SR grew four error flags (MISERR/FASTERR/RDERR/OPTVERR) that must be
// cleared and checked or a stale one wedges every later check_ok(). Dual-bank
// exists only on the 1 MB parts: OPTR.DUALBANK is the runtime ground truth
// and reads 0 on single-bank dies (L412), so one driver serves both.
//
// The KEY constants are built from 16-bit halves on purpose: the whole 32-bit
// values are 8-hex-digit silicon facts the contract gate rejects in hand code.

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/flash/flash_impl.hpp"
#include "alloy/ip/st/flash_l4.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::flash_l4>
struct flash_impl<Inst> {
    using IP = typename Inst::ip;

    // L4 flash erase granularity, fixed by the IP (all RM0394 parts = 2 KB).
    static constexpr std::uint32_t page_size = 2048u;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void wait_idle() {
        while (IP::bsy.read(r()) != 0u) {
        }
    }

    // SR error flags are write-1-to-clear; wipe stale ones so check_ok() only
    // sees errors from the operation we are about to run. OPTVERR in
    // particular is SET OUT OF RESET on parts whose option bytes were never
    // programmed — left standing it fails every later check.
    static constexpr std::uint32_t err_mask() {
        return IP::operr.mask | IP::progerr.mask | IP::wrperr.mask |
               IP::pgaerr.mask | IP::sizerr.mask | IP::pgserr.mask |
               IP::miserr.mask | IP::fasterr.mask | IP::rderr.mask |
               IP::optverr.mask;
    }
    static void clear_errors() { r().SR = err_mask(); }

    [[nodiscard]] static bool check_ok() {
        if ((r().SR & err_mask()) != 0u) {
            r().SR = err_mask();  // w1c
            return false;
        }
        return true;
    }

    static void unlock() {
        if (IP::lock.read(r()) == 0u) {
            return;  // already unlocked
        }
        r().KEYR = (0x4567u << 16) | 0x0123u;
        r().KEYR = (0xCDEFu << 16) | 0x89ABu;
    }

    static void lock() { IP::lock.set(r()); }

    // Erase the 2 KB page that contains `addr`. Returns false on a flash error.
    [[nodiscard]] static bool erase_page(std::uintptr_t addr) {
        wait_idle();
        clear_errors();
        const std::uint32_t off = static_cast<std::uint32_t>(addr - Inst::mem_base);
        const bool dual = IP::dualbank.read(r()) != 0u;
        const std::uint32_t bank_size = dual ? Inst::mem_size / 2u : Inst::mem_size;
        const std::uint32_t bker = (dual && off >= bank_size) ? 1u : 0u;
        const std::uint32_t pnb = (off - bker * bank_size) / page_size;

        unlock();
        // PER + page + bank in one write (START still 0), then START to launch.
        r().CR = IP::per.mask | (pnb << IP::pnb.pos) | (bker != 0u ? IP::bker.mask : 0u);
        IP::start.set(r());
        wait_idle();
        const bool ok = check_ok();
        IP::per.clear(r());
        lock();
        return ok;
    }

    // Program `n` 64-bit double-words starting at `addr` (8-byte aligned).
    // Each double-word is two 32-bit writes; the second launches the program.
    [[nodiscard]] static bool program(std::uintptr_t addr, const std::uint64_t* data,
                                      std::size_t n) {
        wait_idle();
        clear_errors();
        unlock();
        bool ok = true;
        for (std::size_t i = 0; i < n && ok; ++i) {
            IP::pg.set(r());
            auto* w = reinterpret_cast<volatile std::uint32_t*>(addr + i * 8u);
            w[0] = static_cast<std::uint32_t>(data[i]);
            w[1] = static_cast<std::uint32_t>(data[i] >> 32);
            wait_idle();
            if (IP::eop.read(r()) != 0u) {
                IP::eop.set(r());  // w1c
            }
            ok = check_ok();
            IP::pg.clear(r());
        }
        lock();
        return ok;
    }
};

}  // namespace alloy::hal
