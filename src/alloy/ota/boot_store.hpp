// A POWER-ATOMIC single-value store for the OTA boot-state, over TWO erasable
// flash pages (ping-pong / A-B redundancy). Each set() rewrites the STALE page
// with an incremented sequence number + a record CRC; the OTHER page holds the
// last committed value the whole time, so a power loss during the write leaves the
// previous value authoritative. This is the atomicity the boot_manager's anti-brick
// guarantee actually depends on.
//
// (alloy::nvm::store also satisfies BootStore, but its append-log compacts by
// erasing-then-rewriting one page in place — NOT power-atomic — so it must NOT hold
// the boot-state: a power loss mid-compaction drops the record and the fallback
// would abandon a confirmed slot. Use nvm::store for ordinary settings, boot_store
// for the boot-state.)
//
// Record per page (two 64-bit double-words, the flash program unit):
//   dw0 = (seq:u32 << 32) | value:u32
//   dw1 = (~crc << 32) | crc          crc = CRC-32 over (seq, value) little-endian
// A torn write leaves either an erased seq (dw0 not written) or a CRC that fails
// its own complement/data check (dw1 not written), so the half-written page is
// ignored and the other page wins. The highest valid seq is current.
#pragma once

#include <cstddef>
#include <cstdint>

#include "alloy/concepts.hpp"
#include "alloy/ota/crc32.hpp"
#include "alloy/util/byteorder.hpp"

namespace alloy::ota {

template <alloy::FlashController Flash>
class boot_store {
    Flash flash_{};
    std::uintptr_t page_[2]{};

    static constexpr std::uint32_t seq_erased = 0xFFFF'FFFFu;  // an erased-flash slot

    static std::uint32_t record_crc(std::uint32_t seq, std::uint32_t value) {
        std::uint8_t buf[8];
        alloy::byteorder::store_le32(buf + 0, seq);
        alloy::byteorder::store_le32(buf + 4, value);
        return crc::crc32_of(buf);
    }

    // Read page i; on a valid, intact record set seq/value and return true.
    [[nodiscard]] bool read(int i, std::uint32_t& seq, std::uint32_t& value) const {
        const auto* p = reinterpret_cast<const volatile std::uint64_t*>(page_[i]);
        const std::uint64_t dw0 = p[0];
        const std::uint64_t dw1 = p[1];
        const std::uint32_t s = static_cast<std::uint32_t>(dw0 >> 32);
        const std::uint32_t v = static_cast<std::uint32_t>(dw0);
        const std::uint32_t crc_lo = static_cast<std::uint32_t>(dw1);
        const std::uint32_t crc_hi = static_cast<std::uint32_t>(dw1 >> 32);
        if (s == 0u || s == seq_erased) return false;             // unwritten / erased
        if (crc_hi != (crc_lo ^ 0xFFFF'FFFFu)) return false;      // torn dw1
        if (crc_lo != record_crc(s, v)) return false;            // corrupt record
        seq = s;
        value = v;
        return true;
    }

public:
    constexpr boot_store(Flash f, std::uintptr_t page_a, std::uintptr_t page_b)
        : flash_(f), page_{page_a, page_b} {}

    // BootStore: stores exactly one value; the key is ignored.
    [[nodiscard]] std::uint32_t get(std::uint32_t /*key*/, std::uint32_t fallback = 0u) const {
        std::uint32_t best_v = fallback;
        std::uint32_t best_seq = 0u;
        bool found = false;
        for (int i = 0; i < 2; ++i) {
            std::uint32_t s = 0u;
            std::uint32_t v = 0u;
            if (read(i, s, v) && (!found || s > best_seq)) {
                best_seq = s;
                best_v = v;
                found = true;
            }
        }
        return best_v;
    }

    [[nodiscard]] bool set(std::uint32_t /*key*/, std::uint32_t value) {
        std::uint32_t seq[2] = {0u, 0u};
        std::uint32_t val[2] = {0u, 0u};
        bool valid[2] = {false, false};
        std::uint32_t max_seq = 0u;
        for (int i = 0; i < 2; ++i) {
            valid[i] = read(i, seq[i], val[i]);
            if (valid[i] && seq[i] > max_seq) max_seq = seq[i];
        }
        // Overwrite the invalid page, or (both valid) the lower-seq one — never the
        // page currently holding the newest committed record.
        const int stale = !valid[0] ? 0 : (!valid[1] ? 1 : (seq[0] <= seq[1] ? 0 : 1));

        std::uint32_t new_seq = max_seq + 1u;
        if (new_seq == 0u || new_seq == seq_erased) new_seq = 1u;  // skip sentinels (wrap: ~never)

        if (!flash_.erase_page(page_[stale])) return false;
        const std::uint64_t dw0 = (static_cast<std::uint64_t>(new_seq) << 32) | value;
        if (!flash_.program(page_[stale], &dw0, 1u)) return false;
        const std::uint32_t crc = record_crc(new_seq, value);
        const std::uint64_t dw1 = (static_cast<std::uint64_t>(crc ^ 0xFFFF'FFFFu) << 32) | crc;
        if (!flash_.program(page_[stale] + 8u, &dw1, 1u)) return false;
        return true;
    }
};

}  // namespace alloy::ota
