#ifndef ALLOY_CORE_ERROR_HPP
#define ALLOY_CORE_ERROR_HPP

#include "error_code.hpp"

/// Error handling for Alloy framework
///
/// This file only contains the ErrorCode enum. For the Result<T,E> type,
/// see result.hpp which provides a full implementation with two template
/// parameters for flexible error handling.

namespace alloy::core {

// ErrorCode enum is imported from error_code.hpp
// No Result implementation here - use result.hpp instead

} // namespace alloy::core

#endif // ALLOY_CORE_ERROR_HPP
