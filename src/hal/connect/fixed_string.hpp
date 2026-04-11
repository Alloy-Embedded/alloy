#pragma once

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace alloy::hal::connect {

template <std::size_t N>
struct FixedString {
    char value[N]{};

    constexpr FixedString(const char (&literal)[N]) {
        std::copy_n(literal, N, value);
    }

    [[nodiscard]] constexpr auto view() const -> std::string_view {
        return std::string_view{value, N - 1};
    }

    [[nodiscard]] constexpr operator std::string_view() const { return view(); }
};

template <std::size_t N>
FixedString(const char (&)[N]) -> FixedString<N>;

}  // namespace alloy::hal::connect
