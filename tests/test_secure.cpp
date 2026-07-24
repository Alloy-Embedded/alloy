// Unit tests for the capability token (alloy/secure.hpp). The guarantee is
// compile-time (only the Holder can mint a passkey), so we assert it structurally
// with type traits: the Holder mints + calls the guarded op, an unrelated context
// cannot default-construct the key, and a minted key still forwards.

#include <type_traits>

#include "alloy/secure.hpp"
#include "alloy_test.hpp"

namespace {
struct Guard {
    static alloy::passkey<Guard> mint() { return {}; }  // Guard is the friend
    static bool call_guarded();
};

// A dangerous op whose signature demands Guard's key.
bool guarded_op(alloy::passkey<Guard> /*key*/) { return true; }

bool Guard::call_guarded() { return guarded_op(mint()); }
}  // namespace

ALLOY_TEST(passkey_holder_mints_and_calls) {
    ALLOY_CHECK(Guard::call_guarded());  // the authorized path compiles + runs
}

ALLOY_TEST(passkey_unauthorized_cannot_mint) {
    // This TU is not Guard, so the private default constructor is inaccessible:
    // a stray call site literally cannot conjure the key.
    ALLOY_CHECK(!std::is_default_constructible_v<alloy::passkey<Guard>>);
}

ALLOY_TEST(passkey_forwards_once_held) {
    // The holder can hand a minted key to a delegate, which then calls the op.
    const auto key = Guard::mint();
    ALLOY_CHECK(std::is_copy_constructible_v<decltype(key)>);
    ALLOY_CHECK(guarded_op(key));
}
