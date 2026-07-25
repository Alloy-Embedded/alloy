// lwIP NO_SYS port hooks for alloy: the millisecond time source lwIP's timeout
// engine needs, and a weak RNG for TCP initial sequence numbers. Both are
// C-linkage symbols the vendored lwIP links against.
#include <cstdint>

#include "alloy/time.hpp"

extern "C" {

std::uint32_t sys_now(void) {
    return alloy::uptime_ms();
}

// Linear-congruential (glibc constants) stirred by the uptime — not
// cryptographic (alloy has no CSPRNG yet), fine for a v1 echo/HTTP demo where
// LWIP_RAND only seeds TCP ISNs (see arch/cc.h).
std::uint32_t alloy_lwip_rand(void) {
    static std::uint32_t s = 22695477u;
    s = s * 1103515245u + 12345u + alloy::uptime_ms();
    return s;
}

}  // extern "C"
