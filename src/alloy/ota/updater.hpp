// Streams a raw [header|payload] image byte-for-byte into the INACTIVE flash slot
// over a FlashController (erase pages, program 64-bit double-words), buffering
// bytes into dwords, then RE-READS the slot and fully verify_slot()s it — so a bit
// flip or a bad program is caught before the image is ever armed. Touches NO
// boot-state; the caller arms the trial via install() only after finish() passes.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "alloy/concepts.hpp"
#include "alloy/ota/boot_state.hpp"
#include "alloy/ota/image.hpp"
#include "alloy/ota/verify.hpp"

namespace alloy::ota {

template <alloy::FlashController Flash>
class updater {
    Flash flash_{};
    slot tgt_{};
    std::uint32_t page_{};
    std::uint32_t flushed_{0};  // bytes programmed (always a multiple of 8)
    std::uint8_t stage_[8]{};   // partial double-word being assembled
    std::uint8_t stage_len_{0};
    bool open_{false};
    using rvoid = Result<void, ota_error>;

public:
    // `page_size` is a plain runtime value (like nvm::store's size) so host fakes
    // and any chip's flash HAL both satisfy the FlashController constraint.
    constexpr updater(Flash f, slot target, std::uint32_t page_size)
        : flash_(f), tgt_(target), page_(page_size) {}

    // Erase the whole target slot to 0xFF.
    [[nodiscard]] rvoid begin() {
        for (std::uint32_t off = 0; off < tgt_.size; off += page_) {
            if (!flash_.erase_page(tgt_.base + off)) return ota_error::flash_io;
        }
        flushed_ = 0;
        stage_len_ = 0;
        open_ = true;
        return alloy::ok<ota_error>();
    }

    // Append a chunk (any size); complete double-words are programmed as they fill.
    // On any error the updater is fenced (open_ = false) so a later write() returns
    // not_open rather than touching the now-invalid staging buffer.
    [[nodiscard]] rvoid write(std::span<const std::uint8_t> chunk) {
        if (!open_) return ota_error::not_open;
        for (std::uint8_t b : chunk) {
            // Reject BEFORE staging the byte that would complete a dword we can't
            // fit — so stage_len_ never persists at 8 across an error return.
            if (stage_len_ == 7u && static_cast<std::uint64_t>(flushed_) + 8u > tgt_.size) {
                open_ = false;
                return ota_error::slot_overflow;
            }
            stage_[stage_len_++] = b;
            if (stage_len_ == 8u) {
                std::uint64_t w;
                std::memcpy(&w, stage_, 8);  // byte-faithful on the (LE) target
                if (!flash_.program(tgt_.base + flushed_, &w, 1u)) {
                    open_ = false;
                    return ota_error::flash_io;
                }
                flushed_ += 8u;
                stage_len_ = 0;
            }
        }
        return alloy::ok<ota_error>();
    }

    [[nodiscard]] std::uint32_t written() const { return flushed_ + stage_len_; }

    // Flush any partial tail (padded to a dword with 0xFF — excluded from the CRC
    // because verify uses payload_length), then re-read + verify the slot.
    // `v` is the authenticity policy (v2 signing): defaulted to no_auth so v1
    // callers are unchanged, and passed straight to verify_slot so a REJECTED
    // image fails the UPDATE — not later, at boot, after a wasted trial.
    template <Verifier V = no_auth>
    [[nodiscard]] Result<image_header, ota_error> finish(V v = {}) {
        if (!open_) return ota_error::not_open;
        if (stage_len_ > 0) {
            if (static_cast<std::uint64_t>(flushed_) + 8u > tgt_.size) {
                open_ = false;
                return ota_error::slot_overflow;
            }
            while (stage_len_ < 8u) stage_[stage_len_++] = 0xFFu;
            std::uint64_t w;
            std::memcpy(&w, stage_, 8);
            if (!flash_.program(tgt_.base + flushed_, &w, 1u)) {
                open_ = false;
                return ota_error::flash_io;
            }
            flushed_ += 8u;
            stage_len_ = 0;
        }
        // Latch-buffered controllers (SAME70 EEFC: writes gather in a page latch
        // and only a WP command commits them) expose an optional flush(); verify
        // re-reads the FLASH, so everything must be committed first. Controllers
        // without one (G0/F7: program lands directly) skip this at compile time.
        if constexpr (requires(Flash f) { { f.flush() } -> std::convertible_to<bool>; }) {
            if (!flash_.flush()) {
                open_ = false;
                return ota_error::flash_io;
            }
        }
        open_ = false;
        return verify_slot(tgt_, v);  // check what ACTUALLY landed in flash
    }
};

// Tie updater + boot_manager: verify the freshly-written slot, apply the
// ANTI-ROLLBACK floor, THEN atomically arm the trial. A crash between finish()
// and mark_updated leaves pending==none, so the old (confirmed) firmware still
// boots — the confirmed slot is never perturbed.
//
// `min_version` is the floor: an image whose image_version is BELOW it is
// refused with ota_error::rollback and never armed. A replayed older image is
// genuinely signed, so no signature check can catch it — only a version policy
// can, and this is where alloy puts it.
//
// WHY AT ACCEPT TIME AND NOT AT BOOT. The bootloader's anti-brick rule is "if
// the slot you planned to boot doesn't verify, boot the other one" — and after
// any successful update the other one is, by definition, older. A boot-time
// version floor would therefore refuse exactly the fallback that keeps a device
// alive, turning "silently survives a corrupt slot" into "needs a field tech".
// So: a confirmed older slot stays bootable forever; what is refused is
// *writing* an older image over the wire.
//
// The floor is the CALLER's to compute, because only the caller knows what this
// device can prove it already has (the bootloader uses the highest verified
// image_version across the running slot and the target slot — see
// examples/bootloader_uart). Default 0 = no floor, so v1/v2 callers are
// unchanged.
//
// The refusal lands AFTER the whole image was streamed and the target slot was
// already erased. That is safe (the confirmed slot is untouched and pending
// stays none) but it does leave a garbage inactive slot, which will surprise an
// operator — the NAK an operator sees must say so.
template <alloy::FlashController Flash, BootStore Store, Verifier V = no_auth>
[[nodiscard]] Result<image_header, ota_error>
install(updater<Flash>& up, boot_manager<Store>& boot, std::uint8_t target, V v = {},
        std::uint32_t min_version = 0u) {
    auto h = up.finish(v);
    if (!h) return h.error();
    if (h->image_version < min_version) return ota_error::rollback;
    if (auto r = boot.mark_updated(target); !r) return r.error();
    return h;
}

}  // namespace alloy::ota
