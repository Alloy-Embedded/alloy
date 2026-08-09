// Instance ownership (alloy/core/claim.hpp) — the hole the layered peripheral
// surface left open and this closes.
//
// The layers make a CALL SITE impossible to lie in. They said nothing about
// who owns the peripheral, so two `bind<>` specializations naming one
// `usart2_t` each carried their own `detail_opened` flag: both opened, no
// diagnostic, and the second silently reprogrammed the port under the first
// handle. The flag lives on the INSTANCE now.
//
// Board-free, so it runs in the host suite: an "instance" here is just a tag
// type, exactly as `alloy::dev::usart2_t` is to the claim.
//
// The refusals are death tests — the same idiom test_async.cpp uses for the
// single-owner event guard, and for the same reason: a trap is the honest
// runtime answer where C++ cannot see across translation units.

#include <sys/wait.h>
#include <unistd.h>

#include "alloy/core/claim.hpp"
#include "alloy_test.hpp"

using alloy::claim::personality;

namespace {
// Stand-ins for generated instance descriptors. Distinct types, exactly as two
// peripherals are.
struct inst_a {};
struct inst_b {};
struct inst_c {};
struct inst_d {};
struct inst_e {};
struct inst_f {};
struct inst_g {};

// A child that must NOT exit cleanly: the guard under test has to fire.
template <class Fn>
bool refuses(Fn body) {
    const pid_t pid = fork();
    if (pid == 0) {
        body();
        _exit(0);  // reached only if the guard FAILED to fire
    }
    int status = 0;
    (void)waitpid(pid, &status, 0);
    return !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
}  // namespace

ALLOY_TEST(claim_first_exclusive_owner_is_recorded) {
    ALLOY_CHECK(!(alloy::claim::held<inst_a, personality::uart>()));
    alloy::claim::exclusive<inst_a, personality::uart>();
    ALLOY_CHECK((alloy::claim::held<inst_a, personality::uart>()));
    // The claim is on the INSTANCE, not on the personality: a different
    // personality does not "hold" a port it never opened, which is what
    // reconfigure() now asks before touching a register.
    ALLOY_CHECK(!(alloy::claim::held<inst_a, personality::spi>()));
}

ALLOY_TEST(claim_instances_are_independent) {
    alloy::claim::exclusive<inst_b, personality::i2c>();
    // Claiming one instance must not claim its neighbour — the flag used to be
    // per binder TYPE, which had the opposite failure (one instance, many flags).
    ALLOY_CHECK(!(alloy::claim::held<inst_c, personality::i2c>()));
    alloy::claim::exclusive<inst_c, personality::i2c>();
    ALLOY_CHECK((alloy::claim::held<inst_b, personality::i2c>()));
    ALLOY_CHECK((alloy::claim::held<inst_c, personality::i2c>()));
}

ALLOY_TEST(claim_second_binder_on_one_instance_traps) {
    // THE REGRESSION. Two different binders, same personality, same instance:
    // this is the second legitimately-routed pin pair on one USART that used
    // to compile clean and reprogram the port.
    ALLOY_CHECK(refuses([] {
        alloy::claim::exclusive<inst_d, personality::uart>();
        alloy::claim::exclusive<inst_d, personality::uart>();
    }));
}

ALLOY_TEST(claim_two_personalities_on_one_block_trap) {
    // A timer cannot be a PWM generator and an encoder counter at once. This
    // is the runtime half of the rule; emit/board.py refuses the same conflict
    // at generation time when the board file is what states it.
    ALLOY_CHECK(refuses([] {
        alloy::claim::exclusive<inst_e, personality::pwm>();
        alloy::claim::exclusive<inst_e, personality::encoder>();
    }));
}

ALLOY_TEST(claim_shared_admits_agreeing_claimants) {
    // Four PWM channels on one timer are one owner in one personality — legal,
    // as long as they agree about the block-scoped value (one PSC, one ARR).
    alloy::claim::shared<inst_f, personality::pwm>(20'000u);
    alloy::claim::shared<inst_f, personality::pwm>(20'000u);
    alloy::claim::shared<inst_f, personality::pwm>(20'000u);
    ALLOY_CHECK((alloy::claim::held<inst_f, personality::pwm>()));
}

ALLOY_TEST(claim_shared_refuses_disagreeing_claimants) {
    // ...and a live defect in the shipped PWM facade until now: two channels
    // of one timer asking for different frequencies both "succeeded", and the
    // second one's PSC/ARR write silently retuned the first.
    ALLOY_CHECK(refuses([] {
        alloy::claim::shared<inst_g, personality::pwm>(20'000u);
        alloy::claim::shared<inst_g, personality::pwm>(1'000u);
    }));
}

ALLOY_TEST(claim_shared_refuses_a_foreign_personality) {
    ALLOY_CHECK(refuses([] {
        alloy::claim::shared<inst_g, personality::pwm>(20'000u);
        alloy::claim::exclusive<inst_g, personality::encoder>();
    }));
}
