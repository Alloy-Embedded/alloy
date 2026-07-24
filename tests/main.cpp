// Host test runner: bind the harness sink to stdout and run every registered
// test. Exit code = number of failing tests, so ctest reports pass/fail from
// the process result. On silicon the same run_all() binds to board::debug_uart
// instead (a future `alloy test --board <id>` flashes such a runner).

#include <cstdio>
#include <cstdint>

#include "alloy_test.hpp"

namespace {
struct stdout_sink {
    void write(std::uint8_t b) { std::fputc(static_cast<int>(b), stdout); }
    void write(const char* z) { std::fputs(z, stdout); }
};
}  // namespace

int main() {
    stdout_sink sink;
    return alloy::test::run_all(sink);
}
