#include "core/error_code.hpp"
#include "core/result.hpp"

int main() {
    ucore::core::Result<int, ucore::core::ErrorCode> value = ucore::core::Ok(42);
    return value.is_ok() ? 0 : 1;
}
