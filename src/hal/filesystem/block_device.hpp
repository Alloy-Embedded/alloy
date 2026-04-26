#pragma once

#include <concepts>
#include <cstddef>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::hal::filesystem {

// BlockDevice<T> — compile-time interface for a raw block storage medium.
//
// A block device exposes uniform-sized blocks. All addressing is by block
// index. Callers are responsible for alignment (writes must not cross a block
// boundary; erases operate on whole blocks).
//
// Span element type is std::byte throughout. Drivers that work with uint8_t
// internally must cast in their adapter (reinterpret_cast is well-defined
// between std::byte* and unsigned char*).
template <typename T>
concept BlockDevice = requires(T& d,
                               std::size_t block,
                               std::size_t count,
                               std::span<std::byte> buf,
                               std::span<const std::byte> data) {
    // Read `buf.size()` bytes from block `block` into buf.
    // buf.size() must equal block_size(). Returns Err on I/O failure.
    { d.read(block, buf) } -> std::same_as<alloy::core::Result<void, alloy::core::ErrorCode>>;

    // Write `data.size()` bytes to block `block`. data.size() must equal
    // block_size(). Block must have been erased first.
    { d.write(block, data) } -> std::same_as<alloy::core::Result<void, alloy::core::ErrorCode>>;

    // Erase `count` blocks starting at `block`. Required before write.
    { d.erase(block, count) } -> std::same_as<alloy::core::Result<void, alloy::core::ErrorCode>>;

    // Bytes per block. Constant for the lifetime of the device object.
    { d.block_size() } -> std::same_as<std::size_t>;

    // Total number of blocks available.
    { d.block_count() } -> std::same_as<std::size_t>;
};

}  // namespace alloy::hal::filesystem
