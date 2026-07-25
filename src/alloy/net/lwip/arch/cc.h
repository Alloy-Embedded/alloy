/* lwIP compiler-abstraction port for alloy (bare-metal, NO_SYS).
 *
 * lwIP pulls its integer types from <stdint.h> by default; this header only
 * supplies byte order, on-the-wire struct packing, the diagnostic/assert hooks,
 * and a random source. Firmware-only (no stdio) — diagnostics are swallowed. */
#ifndef ALLOY_LWIP_ARCH_CC_H
#define ALLOY_LWIP_ARCH_CC_H

#include <stdint.h>
#include <stdlib.h>

/* Cortex-M / xtensa are little-endian. */
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

/* GCC packing for protocol structs (Ethernet/IP/TCP headers). */
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

/* No stdio on firmware: drop lwIP's debug/trace lines. A fatal lwIP assert
 * means a stack invariant broke — stop rather than corrupt further. */
#define LWIP_PLATFORM_DIAG(x)
#define LWIP_PLATFORM_ASSERT(x) \
    do {                        \
        for (;;) {              \
        }                       \
    } while (0)

/* TCP initial-sequence-number randomness. alloy has no CSPRNG yet; a weak
 * source is acceptable for v1 (an echo/HTTP demo, not a security boundary). */
uint32_t alloy_lwip_rand(void);
#define LWIP_RAND() (alloy_lwip_rand())

#endif /* ALLOY_LWIP_ARCH_CC_H */
