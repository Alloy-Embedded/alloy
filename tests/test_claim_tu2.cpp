// The SECOND translation unit of the cross-TU claim test. It holds no test
// cases: its entire job is to be a different .cpp that names the same instance
// types, so test_claim.cpp can prove the claim is one object and not one per
// compilation. See claim_cross_tu.hpp.

#include "claim_cross_tu.hpp"

#include <cstdint>

#include "alloy/core/claim.hpp"

namespace alloy::test {

using alloy::claim::personality;

void claim_seen_in_tu2() {
    alloy::claim::exclusive<cross_tu_seen, personality::uart>();
}

bool seen_held_in_tu2() {
    return alloy::claim::held<cross_tu_seen, personality::uart>();
}

void claim_trap_in_tu2() {
    alloy::claim::exclusive<cross_tu_trap, personality::uart>();
}

void claim_sub_in_tu2(std::uint32_t port_index) {
    alloy::claim::sub_shared<cross_tu_sub, 5u, personality::exti>(port_index);
}

}  // namespace alloy::test
