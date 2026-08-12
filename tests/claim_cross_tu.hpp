// The other half of the cross-TU claim test — see tests/test_claim_tu2.cpp.
//
// `alloy::claim::owner<Inst>` is an INLINE VARIABLE TEMPLATE, and that word is
// the whole mechanism: it makes one object per instance across the entire
// image, however many translation units name it. The flag it replaced was a
// `static` member of a binder TYPE, so two binders on one peripheral had two
// flags and both "succeeded" — and every test written for the repair so far
// lives in a single .cpp, which is exactly the arrangement that cannot tell
// the two apart. This header + test_claim_tu2.cpp make the second TU real.

#pragma once

#include <cstdint>

namespace alloy::test {

// Instance stand-ins NAMED IN BOTH TUs. Distinct types, exactly as two
// peripherals are, and declared here so the two .cpp files provably mean the
// same `Inst` and not two locals that happen to share a spelling.
struct cross_tu_seen {};
struct cross_tu_trap {};
struct cross_tu_sub {};

// Claims made from the OTHER translation unit.
void claim_seen_in_tu2();
[[nodiscard]] bool seen_held_in_tu2();
void claim_trap_in_tu2();
void claim_sub_in_tu2(std::uint32_t port_index);

}  // namespace alloy::test
