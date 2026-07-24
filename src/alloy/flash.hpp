// User-facing flash controller: a stateless handle over the generated flash
// instance, exposing page erase + double-word program. Reads are memory-mapped
// (the flash array is directly addressable), so there is no read method.
//
// This is the low-level seam; most users want the key/value store built on top
// (alloy/util/nvm_kv.hpp, wired as board::nvm).

#pragma once

#include <cstddef>
#include <cstdint>

// Only the primary template — the concrete per-IP driver is pulled in by the
// generated device.hpp for the chip that actually has the flash IP.
#include "alloy/hal/flash/flash_impl.hpp"

namespace alloy::flash {

template <class Inst>
class controller {
public:
    // Flash erase granularity (bytes), fixed by the IP.
    static constexpr std::uint32_t page_size = hal::flash_impl<Inst>::page_size;

    [[nodiscard]] bool erase_page(std::uintptr_t addr) const {
        return hal::flash_impl<Inst>::erase_page(addr);
    }
    [[nodiscard]] bool program(std::uintptr_t addr, const std::uint64_t* data,
                               std::size_t n) const {
        return hal::flash_impl<Inst>::program(addr, data, n);
    }
};

// No-op stand-in for boards without a flash/nvm role — keeps caps-guarded code
// compiling everywhere.
struct null_controller {
    static constexpr std::uint32_t page_size = 0u;
    [[nodiscard]] bool erase_page(std::uintptr_t) const { return false; }
    [[nodiscard]] bool program(std::uintptr_t, const std::uint64_t*, std::size_t) const {
        return false;
    }
};

}  // namespace alloy::flash
