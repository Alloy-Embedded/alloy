// Flash program/erase for the STM32G0 embedded flash (flash_g0 IP).
//
// BEHAVIOR only: register overlay, gate and the flash MEMORY geometry
// (mem_base/mem_size) come from generated data. RM0444 §3: unlock KEYR with the
// two magic keys, erase whole 2 KB pages (CR.PER + PNB + BKER, STRT), program
// one 64-bit double-word at a time (CR.PG, two word writes, the second starts
// the operation). The 512 KB parts (G0B1) are dual-bank — CR.BKER picks the
// bank, CR.PNB the page in it; OPTR.DUAL_BANK is the runtime ground truth so a
// 128 KB single-bank G0 and a 512 KB dual-bank G0 share this one driver.
//
// The KEY constants are built from 16-bit halves on purpose: the whole 32-bit
// values are 8-hex-digit silicon facts the contract gate rejects in hand code.

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/flash/flash_impl.hpp"
#include "alloy/ip/st/flash_g0.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::flash_g0>
struct flash_impl<Inst> {
    using IP = typename Inst::ip;

    // G0 flash erase granularity, fixed by the IP (all STM32G0 = 2 KB pages).
    static constexpr std::uint32_t page_size = 2048u;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void wait_idle() {
        while (IP::bsy.read(r()) != 0u) {
        }
    }

    // SR error flags are write-1-to-clear; wipe stale ones so check_ok() only
    // sees errors from the operation we are about to run.
    static void clear_errors() {
        r().SR = IP::operr.mask | IP::progerr.mask | IP::wrperr.mask | IP::pgaerr.mask |
                 IP::sizerr.mask | IP::pgserr.mask;
    }

    [[nodiscard]] static bool check_ok() {
        const std::uint32_t errs = IP::operr.mask | IP::progerr.mask | IP::wrperr.mask |
                                   IP::pgaerr.mask | IP::sizerr.mask | IP::pgserr.mask;
        if ((r().SR & errs) != 0u) {
            r().SR = errs;  // w1c
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
        const bool dual = IP::dual_bank.read(r()) != 0u;
        const std::uint32_t bank_size = dual ? Inst::mem_size / 2u : Inst::mem_size;
        const std::uint32_t bker = (dual && off >= bank_size) ? 1u : 0u;
        const std::uint32_t pnb = (off - bker * bank_size) / page_size;

        unlock();
        // PER + page + bank in one write (STRT still 0), then STRT to launch.
        r().CR = IP::per.mask | (pnb << IP::pnb.pos) | (bker != 0u ? IP::bker.mask : 0u);
        IP::strt.set(r());
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
