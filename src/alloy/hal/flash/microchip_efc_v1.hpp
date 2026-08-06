// Flash program/erase for the SAM E70/S70/V70 embedded flash (efc_v1 IP).
//
// EEFC is nothing like the ST controllers: writes to flash ADDRESSES land in a
// page-wide LATCH (not the array), and only an FCR WP command commits a 512-byte
// page; erase granularity is a 16-page block (8 KB, EPA command); and the ECC
// allows each 128-bit row to be written ONCE between erases (SAME70 datasheet
// §22). A caller streaming 8-byte double-words (the updater) would double-write
// every row if each program() committed — so this driver BUFFERS a page in RAM
// and commits it exactly once: automatically when the write stream crosses into
// the next page, and on the optional flush() that alloy/ota's updater and
// boot_store call before anything re-reads or must be durable. Each row is
// therefore written exactly once per erase, by construction.
//
// Single-owner by design (one inline static latch): the bootloader/updater path
// is strictly sequential. Interleaving writes to DIFFERENT pages through this
// driver would thrash commits — don't.
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "alloy/core/types.hpp"
#include "alloy/hal/flash/flash_impl.hpp"
#include "alloy/ip/microchip/efc_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::efc_v1>
struct flash_impl<Inst> {
    using IP = typename Inst::ip;

    static constexpr std::uint32_t page_bytes = 512u;      // WP program unit
    static constexpr std::uint32_t erase_pages = 16u;      // EPA block: FARG code 2
    static constexpr std::uint32_t page_size = page_bytes * erase_pages;  // 8 KB

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void wait_ready() {
        while ((r().FSR & IP::frdy.mask) == 0u) {
        }
    }

    [[nodiscard]] static bool check_ok(std::uint32_t fsr) {
        return (fsr & (IP::fcmde.mask | IP::flocke.mask | IP::flerr.mask)) == 0u;
    }

    [[nodiscard]] static bool command(std::uint32_t fcmd, std::uint32_t farg) {
        wait_ready();
        r().FCR = (0x5Au << IP::fkey.pos) | (farg << IP::farg.pos) | fcmd;
        wait_ready();
        return check_ok(r().FSR);
    }

    // Erase the 16-page (8 KB) block containing `addr`.
    [[nodiscard]] static bool erase_page(std::uintptr_t addr) {
        const std::uint32_t page =
            static_cast<std::uint32_t>(addr - Inst::mem_base) / page_bytes;
        // EPA: FARG[15:2] = first page (block-aligned), FARG[1:0] = 2 -> 16 pages.
        return command(0x07u, (page & ~(erase_pages - 1u)) | 2u);
    }

    // Buffer double-words into the current page; commit on page change. The
    // stream is ascending (updater/boot_store), so "different page" == "previous
    // page is complete or abandoned-for-good".
    [[nodiscard]] static bool program(std::uintptr_t addr, const std::uint64_t* data,
                                      std::size_t n) {
        bool ok = true;
        for (std::size_t i = 0; i < n && ok; ++i) {
            const std::uintptr_t a = addr + i * 8u;
            const std::uintptr_t base = a & ~static_cast<std::uintptr_t>(page_bytes - 1u);
            if (latch_.active && latch_.base != base) {
                ok = commit();
            }
            if (ok && !latch_.active) {
                latch_.base = base;
                latch_.active = true;
                std::memset(latch_.buf, 0xFF, sizeof latch_.buf);  // erased default
            }
            if (ok) {
                std::memcpy(latch_.buf + (a - base), &data[i], 8u);
            }
        }
        return ok;
    }

    // Commit the buffered page: fill the hardware latch (writes to the page's
    // own addresses) and issue WP. Called by the ota layer before verify/reset.
    [[nodiscard]] static bool flush() {
        if (!latch_.active) {
            return true;
        }
        return commit();
    }

private:
    [[nodiscard]] static bool commit() {
        auto* dst = reinterpret_cast<volatile std::uint32_t*>(latch_.base);
        const auto* src = reinterpret_cast<const std::uint32_t*>(latch_.buf);
        for (std::uint32_t w = 0; w < page_bytes / 4u; ++w) {
            dst[w] = src[w];
        }
        asm volatile("dmb" ::: "memory");
        const std::uint32_t page =
            static_cast<std::uint32_t>(latch_.base - Inst::mem_base) / page_bytes;
        latch_.active = false;
        return command(0x01u, page);  // WP
    }

    struct latch {
        std::uintptr_t base = 0;
        bool active = false;
        alignas(std::uint64_t) std::uint8_t buf[page_bytes];
    };
    inline static latch latch_{};
};

}  // namespace alloy::hal
