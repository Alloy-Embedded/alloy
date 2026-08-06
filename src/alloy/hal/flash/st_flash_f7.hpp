// Flash program/erase for the STM32F7 embedded flash (flash_f7 IP).
//
// Unlike the G0's uniform 2 KB pages, F7 flash is SECTOR-based with mixed sizes
// (RM0431/RM0385 §3.3): a small-sector head then large sectors. erase_page(addr)
// therefore erases the SECTOR CONTAINING addr — callers that step a region in
// fixed strides simply re-hit the same big sector (harmless: repeat erase of an
// erased sector); the A/B slot layout for F7 places slots on the large sectors
// so the updater's stride equals one sector. Programming: unlock KEYR, CR.PG
// with x32 parallelism, write words to the target address, DSB, poll SR.BSY.
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/flash/flash_impl.hpp"
#include "alloy/ip/st/flash_f7.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::flash_f7>
struct flash_impl<Inst> {
    using IP = typename Inst::ip;

    // Sector layout patterns (single-bank; RM0431 512K, RM0385 1M/2M). Values
    // are sector SIZES in KB; every F7 part is one of these by flash size.
    // Exposed so the slot-layout codegen and this driver agree on geometry.
    static constexpr std::uint32_t kb = 1024u;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // Sector index + base + size for the sector containing `addr`.
    struct sector {
        std::uint32_t index;
        std::uintptr_t base;
        std::uint32_t size;
    };

    [[nodiscard]] static sector sector_of(std::uintptr_t addr) {
        const std::uint32_t off = static_cast<std::uint32_t>(addr - Inst::mem_base);
        // Head sizes by total flash: 512K -> 4x16K+64K then 128K sectors;
        // 1M -> 4x32K+128K then 256K; 2M (single bank) -> 4x32K+128K then 256K.
        const std::uint32_t small = Inst::mem_size >= 1024u * kb ? 32u * kb : 16u * kb;
        const std::uint32_t big = small * 8u;  // 128K or 256K
        std::uintptr_t base = Inst::mem_base;
        std::uint32_t idx = 0;
        for (std::uint32_t sz = small; base <= addr;) {
            if (idx < 4u) {
                sz = small;
            } else if (idx == 4u) {
                sz = small * 4u;  // the single medium sector (64K / 128K)
            } else {
                sz = big;
            }
            if (addr < base + sz) {
                return {idx, base, sz};
            }
            base += sz;
            ++idx;
        }
        __builtin_trap();  // addr below flash base: caller bug
    }

    static void wait_idle() {
        while (IP::bsy.read(r()) != 0u) {
        }
    }

    static void clear_errors() {
        r().SR = IP::operr.mask | IP::wrperr.mask | IP::pgaerr.mask |
                 IP::pgperr.mask | IP::erserr.mask | IP::eop.mask;  // w1c
    }

    [[nodiscard]] static bool check_ok() {
        return (r().SR & (IP::operr.mask | IP::wrperr.mask | IP::pgaerr.mask |
                          IP::pgperr.mask | IP::erserr.mask)) == 0u;
    }

    static void unlock() {
        if (IP::lock.read(r()) == 0u) {
            return;
        }
        r().KEYR = (0x4567u << 16) | 0x0123u;
        r().KEYR = (0xCDEFu << 16) | 0x89ABu;
    }

    static void lock() { IP::lock.set(r()); }

    // Erase the sector containing `addr`. Returns false on a flash error.
    [[nodiscard]] static bool erase_page(std::uintptr_t addr) {
        const sector s = sector_of(addr);
        wait_idle();
        clear_errors();
        unlock();
        r().CR = IP::ser.mask | (s.index << IP::snb.pos) | (2u << IP::psize.pos);
        IP::strt.set(r());
        wait_idle();
        const bool ok = check_ok();
        IP::ser.clear(r());
        lock();
        return ok;
    }

    // Program `n` 64-bit double-words starting at `addr` (8-byte aligned), as
    // two x32 writes each — matches the G0 driver's contract so the updater and
    // boot_store work unchanged.
    [[nodiscard]] static bool program(std::uintptr_t addr, const std::uint64_t* data,
                                      std::size_t n) {
        wait_idle();
        clear_errors();
        unlock();
        r().CR = IP::pg.mask | (2u << IP::psize.pos);
        bool ok = true;
        for (std::size_t i = 0; i < n && ok; ++i) {
            auto* w = reinterpret_cast<volatile std::uint32_t*>(addr + i * 8u);
            w[0] = static_cast<std::uint32_t>(data[i]);
            asm volatile("dsb" ::: "memory");
            w[1] = static_cast<std::uint32_t>(data[i] >> 32);
            asm volatile("dsb" ::: "memory");
            wait_idle();
            if (IP::eop.read(r()) != 0u) {
                r().SR = IP::eop.mask;  // w1c
            }
            ok = check_ok();
        }
        IP::pg.clear(r());
        lock();
        return ok;
    }
};

}  // namespace alloy::hal
