#pragma once

// FlashBackend concept — minimal write-erase-read interface used by the DFU
// downloader to commit firmware blocks. Any driver satisfying this concept
// can be the storage target. The W25Q SPI-NOR driver and an STM32 internal
// flash adapter both satisfy it; tests use a host-side in-memory mock.

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::usb::dfu {

template <typename T>
concept FlashBackend = requires(T& flash,
                                std::uint32_t address,
                                std::size_t size,
                                std::span<const std::byte> data,
                                std::span<std::byte> rx) {
    /// Erase a region containing `[address, address + size)`. The backend may
    /// erase a larger aligned region — that's the application's contract.
    { flash.erase(address, size) } -> std::same_as<core::Result<void, core::ErrorCode>>;

    /// Write `data` starting at `address`. Returns once the data has been
    /// committed (no internal queue).
    { flash.write(address, data) } -> std::same_as<core::Result<void, core::ErrorCode>>;

    /// Read `rx.size()` bytes starting at `address`.
    { flash.read(address, rx) } -> std::same_as<core::Result<void, core::ErrorCode>>;
};

}  // namespace alloy::drivers::usb::dfu
