// Freestanding, heapless, exceptions-free unit-test harness. The SAME suite
// compiles for the host (with a stdout sink, this is the CI gate) and,
// unchanged, for silicon (bind the sink to board::debug_uart and flash a
// runner). No Catch2/doctest: those need exceptions + RTTI + heap and could
// never cross-compile onto a -fno-exceptions target, which would break the
// "one suite runs on host AND silicon" promise. Tests self-register into a
// fixed static table; results stream to any alloy::ByteSink.
//
// This lives under tests/ (authored, not generated) — outside src/alloy (so
// the address gate never scans it) and outside examples/ (so the #ifdef gate
// never scans it), which is exactly why it may use test macros freely.

#pragma once

#include <cstddef>
#include <cstdint>

#include "alloy/concepts.hpp"

namespace alloy::test {

using test_fn = void (*)();

inline constexpr std::size_t max_tests = 512;

struct registry {
    test_fn fns[max_tests]{};
    const char* names[max_tests]{};
    std::size_t count{0};

    std::uint32_t total_checks{0};
    std::uint32_t total_failures{0};

    // state for the test currently running
    std::uint32_t cur_failures{0};
    const char* fail_expr{nullptr};
    const char* fail_file{nullptr};
    int fail_line{0};
};

// One shared registry (single-threaded harness; -fno-threadsafe-statics is
// fine — all registration happens during single-threaded static init).
inline registry& reg() {
    static registry r;
    return r;
}

struct registrar {
    registrar(const char* name, test_fn fn) {
        registry& r = reg();
        if (r.count < max_tests) {
            r.names[r.count] = name;
            r.fns[r.count] = fn;
            ++r.count;
        }
    }
};

inline void record(bool ok, const char* expr, const char* file, int line) {
    registry& r = reg();
    ++r.total_checks;
    if (!ok) {
        ++r.total_failures;
        ++r.cur_failures;
        if (r.fail_expr == nullptr) {  // keep the first failure of the test
            r.fail_expr = expr;
            r.fail_file = file;
            r.fail_line = line;
        }
    }
}

namespace detail {
template <class Sink>
void put_uint(Sink& s, std::uint32_t v) {
    char buf[10];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + (v % 10u));
        v /= 10u;
    } while (v != 0);
    while (n-- > 0) {
        s.write(static_cast<std::uint8_t>(buf[n]));
    }
}
}  // namespace detail

// Run every registered test, streaming a report to `sink`. Returns the number
// of FAILING tests (0 = all passed). Callers that turn this into a process exit
// code must clamp it (an 8-bit exit status truncates a count of 256/512 to 0).
template <class Sink>
    requires alloy::ByteSink<Sink>
int run_all(Sink& sink) {
    registry& r = reg();
    int failed = 0;
    for (std::size_t i = 0; i < r.count; ++i) {
        r.cur_failures = 0;
        r.fail_expr = nullptr;
        r.fns[i]();
        if (r.cur_failures == 0) {
            sink.write("[ PASS ] ");
            sink.write(r.names[i]);
            sink.write("\r\n");
        } else {
            ++failed;
            sink.write("[ FAIL ] ");
            sink.write(r.names[i]);
            sink.write(": ");
            sink.write(r.fail_expr != nullptr ? r.fail_expr : "?");
            sink.write(" (");
            sink.write(r.fail_file != nullptr ? r.fail_file : "?");
            sink.write(":");
            detail::put_uint(sink, static_cast<std::uint32_t>(r.fail_line));
            sink.write(")\r\n");
        }
    }
    sink.write(failed == 0 ? "PASSED " : "FAILED ");
    detail::put_uint(sink, static_cast<std::uint32_t>(r.count) - static_cast<std::uint32_t>(failed));
    sink.write("/");
    detail::put_uint(sink, static_cast<std::uint32_t>(r.count));
    sink.write("\r\n");
    return failed;
}

}  // namespace alloy::test

// Define a test that self-registers. Body follows the macro like a function.
#define ALLOY_TEST(test_name)                                                    \
    static void test_name();                                                     \
    static const ::alloy::test::registrar test_name##_registrar{#test_name,      \
                                                                &test_name};     \
    static void test_name()

#define ALLOY_CHECK(cond) \
    ::alloy::test::record(static_cast<bool>(cond), #cond, __FILE__, __LINE__)

#define ALLOY_CHECK_EQ(a, b) \
    ::alloy::test::record((a) == (b), #a " == " #b, __FILE__, __LINE__)
