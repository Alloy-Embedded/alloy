#ifndef UCORE_CORE_CONCEPTS_HPP
#define UCORE_CORE_CONCEPTS_HPP

#include <concepts>
#include <type_traits>

/// Core concepts for MicroCore framework
///
/// Defines utility concepts used for template constraints and validation.

namespace ucore::core {

/// Concept for types that are trivially copyable (zero overhead types)
template <typename T>
concept TrivialType = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

/// Concept for integral types
template <typename T>
concept Integral = std::is_integral_v<T>;

/// Concept for floating point types
template <typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

/// Concept for arithmetic types (integral or floating point)
template <typename T>
concept Arithmetic = Integral<T> || FloatingPoint<T>;

/// Concept for enum types
template <typename T>
concept Enum = std::is_enum_v<T>;

}  // namespace ucore::core

#endif  // ALLOY_CORE_CONCEPTS_HPP
