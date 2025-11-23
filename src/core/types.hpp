#ifndef UCORE_CORE_TYPES_HPP
#define UCORE_CORE_TYPES_HPP

#include <cstddef>
#include <cstdint>

/// Core types for MicroCore framework
///
/// Defines fundamental type aliases and constants used throughout the framework.

namespace ucore::core {

// Fixed-width integer types (aliased for clarity)
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using usize = size_t;
using isize = ptrdiff_t;

// Floating point types
using f32 = float;
using f64 = double;

// Byte type
using byte = uint8_t;

}  // namespace ucore::core

#endif  // ALLOY_CORE_TYPES_HPP
