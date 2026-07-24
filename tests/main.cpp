// Host test runner: bind the harness sink to stdout and run every registered
// test. Exit code is 0 on success and 1 on any failure — clamped, NOT the raw
// failing-test count, because a POSIX exit status is only 8 bits and a count of
// exactly 256/512 would truncate to 0 and read as a green CI gate. On silicon
// the same run_all() binds to board::debug_uart (a future `alloy test --board
// <id>` flashes such a runner).

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
    return alloy::test::run_all(sink) == 0 ? 0 : 1;
}
