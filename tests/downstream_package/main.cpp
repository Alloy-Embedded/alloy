#include "core/result.hpp"

int main() {
    auto result = alloy::core::Result<int, int>(alloy::core::Ok(42));
    return result.is_ok() ? 0 : 1;
}