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
#include "claim_cross_tu.hpp"

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
struct inst_h {};
struct inst_i {};
struct inst_j {};
struct inst_k {};
struct inst_l {};
struct inst_m {};
struct inst_n {};
struct inst_o {};
struct inst_p {};
struct inst_q {};
struct inst_r {};
struct inst_s {};
struct inst_t {};
struct inst_u {};

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

ALLOY_TEST(claim_the_refusal_idiom_reports_false_when_nothing_traps) {
    // THE NEGATIVE CONTROL FOR EVERY `refuses(...)` BELOW, and it was missing:
    // this page's own record claimed one existed and the file had none, so
    // seventeen death tests rested on an idiom nobody had watched report a
    // negative. A `refuses()` that returned true unconditionally — a child that
    // crashed for any reason, or a fork that failed — would make all of them
    // pass while proving nothing.
    ALLOY_CHECK(!refuses([] {}));
    ALLOY_CHECK(!refuses([] { alloy::claim::exclusive<inst_p, personality::uart>(); }));
    ALLOY_CHECK(!refuses([] { alloy::claim::sub_shared<inst_p, 9u, personality::exti>(3u); }));
}

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

// ── The SECOND scope: a numbered part of one block ───────────────────────
//
// Hole (A) had two scopes and only one of them was closed. `owner<Inst>` says
// who owns TIM2; nothing said who owns TIM2 CHANNEL 1, and the flag that
// pretended to was a `static` member of `pwm::bind<Inst, Channel, Pin, Sig,
// Clock>` — templated on the PIN, so one channel got one flag per route. On
// the G0B1RE, TIM2_CH1 has four (PA0, PA5, PA15, PC4).

ALLOY_TEST(claim_sub_resources_of_one_instance_are_independent) {
    // Four channels of one timer are four resources. A claim on one must not
    // read as a claim on its neighbour, or opening ch1 would lock out ch2.
    alloy::claim::sub_exclusive<inst_h, 1u, personality::pwm>();
    ALLOY_CHECK((alloy::claim::sub_held<inst_h, 1u, personality::pwm>()));
    ALLOY_CHECK(!(alloy::claim::sub_held<inst_h, 2u, personality::pwm>()));
    alloy::claim::sub_exclusive<inst_h, 2u, personality::pwm>();
    ALLOY_CHECK((alloy::claim::sub_held<inst_h, 1u, personality::pwm>()));
    ALLOY_CHECK((alloy::claim::sub_held<inst_h, 2u, personality::pwm>()));
}

ALLOY_TEST(claim_sub_resource_of_one_instance_is_not_the_instance) {
    // ...and the two scopes are genuinely separate variables: claiming the
    // block does not claim its channels, nor the other way round. If they
    // shared storage, `pwm::open()` — which does BOTH — would trap on itself.
    alloy::claim::shared<inst_i, personality::pwm>(1'000u);
    ALLOY_CHECK(!(alloy::claim::sub_held<inst_i, 1u, personality::pwm>()));
    alloy::claim::sub_exclusive<inst_i, 1u, personality::pwm>();
    ALLOY_CHECK((alloy::claim::held<inst_i, personality::pwm>()));
}

ALLOY_TEST(claim_second_binder_on_one_sub_resource_traps) {
    // THE REGRESSION FOR THIS SCOPE. Two binders, same instance, same channel,
    // different pin — both legally routed, both compiling, and until now both
    // opening: the second muxed its pin onto the same output compare and said
    // nothing. The two claims here are what those two open() calls reduce to.
    ALLOY_CHECK(refuses([] {
        alloy::claim::sub_exclusive<inst_j, 1u, personality::pwm>();
        alloy::claim::sub_exclusive<inst_j, 1u, personality::pwm>();
    }));
}

ALLOY_TEST(claim_two_personalities_on_one_sub_resource_trap) {
    // A DMA channel handed out twice, once to the DMA facade and once to a
    // facade that drives it itself. Distinct trap code from the case above,
    // because the two bugs have different fixes.
    ALLOY_CHECK(refuses([] {
        alloy::claim::sub_exclusive<inst_k, 3u, personality::dma>();
        alloy::claim::sub_exclusive<inst_k, 3u, personality::adc>();
    }));
}

ALLOY_TEST(claim_sub_resources_of_different_instances_are_independent) {
    // Channel 1 of DMA1 is not channel 1 of DMA2. The key is the pair.
    alloy::claim::sub_exclusive<inst_l, 1u, personality::dma>();
    ALLOY_CHECK(!(alloy::claim::sub_held<inst_m, 1u, personality::dma>()));
    alloy::claim::sub_exclusive<inst_m, 1u, personality::dma>();
    ALLOY_CHECK((alloy::claim::sub_held<inst_l, 1u, personality::dma>()));
}

// ── The watchdog's block-scoped timeout ──────────────────────────────────

ALLOY_TEST(claim_watchdog_admits_two_claimants_of_one_timeout) {
    // A bootloader arming 2 s and the app it starts arming 2 s is not a
    // conflict — the same deadline stated twice.
    alloy::claim::shared<inst_n, personality::wdt>(2'000u);
    alloy::claim::shared<inst_n, personality::wdt>(2'000u);
    ALLOY_CHECK((alloy::claim::held<inst_n, personality::wdt>()));
}

ALLOY_TEST(claim_watchdog_refuses_two_different_timeouts) {
    // ...and a live defect in the shipped wdt facade until now: `start()`
    // programs one prescaler and one reload for the block, there is no
    // close(), and nothing recorded that it had been called. The second
    // start() silently replaced the first and the program went on believing
    // in a deadline the silicon had stopped enforcing.
    ALLOY_CHECK(refuses([] {
        alloy::claim::shared<inst_o, personality::wdt>(4'000u);
        alloy::claim::shared<inst_o, personality::wdt>(10'000u);
    }));
}

// ── A SHARED sub-resource, and the doctrine that said there was no such thing ─
//
// claim.hpp used to argue that a sub-resource is a sub-resource *because* it
// has no config for two claimants to disagree about. The EXTI line refutes it:
// one line has one EXTICR port-select field, one trigger pair and one callback
// slot, and the claimants are PINS — PA5 and PB5 are both line 5. The witness
// is the PORT INDEX, so re-arming the same pin is admitted and a second pin is
// refused. Proven live under Renode before the fix: PA5 armed, PB5 armed, an
// edge driven on PB5, and PA5's handle reported it as its own.

ALLOY_TEST(claim_sub_shared_admits_the_same_claimant_twice) {
    // `on_edge()` then `on_edge()` on ONE pin: legal, and documented — it
    // replaces the edge and the callback. Same line, same port, no conflict.
    alloy::claim::sub_shared<inst_p, 5u, personality::exti>(0u);
    alloy::claim::sub_shared<inst_p, 5u, personality::exti>(0u);
    ALLOY_CHECK((alloy::claim::sub_held<inst_p, 5u, personality::exti>()));
}

ALLOY_TEST(claim_sub_shared_refuses_a_second_claimant_of_one_line) {
    // THE REGRESSION. Port 0 pin 5 and port 1 pin 5 are one EXTI line.
    ALLOY_CHECK(refuses([] {
        alloy::claim::sub_shared<inst_q, 5u, personality::exti>(0u);
        alloy::claim::sub_shared<inst_q, 5u, personality::exti>(1u);
    }));
}

ALLOY_TEST(claim_sub_shared_lines_are_independent) {
    // ...and it must be the LINE that is contested, not the controller: PA5 on
    // line 5 and PB6 on line 6 are two resources of one EXTI block, and a
    // claim keyed one level too high would have refused the second.
    alloy::claim::sub_shared<inst_r, 5u, personality::exti>(0u);
    alloy::claim::sub_shared<inst_r, 6u, personality::exti>(1u);
    ALLOY_CHECK((alloy::claim::sub_held<inst_r, 5u, personality::exti>()));
    ALLOY_CHECK((alloy::claim::sub_held<inst_r, 6u, personality::exti>()));
}

ALLOY_TEST(claim_sub_release_hands_the_line_over) {
    // The one release in the mechanism. `pa5.clear_on_edge()` genuinely frees
    // line 5, so PB5 may take it — refusing it forever would be a new lie in
    // place of the old one.
    alloy::claim::sub_shared<inst_s, 5u, personality::exti>(0u);
    alloy::claim::sub_release<inst_s, 5u, personality::exti>(0u);
    ALLOY_CHECK(!(alloy::claim::sub_held<inst_s, 5u, personality::exti>()));
    alloy::claim::sub_shared<inst_s, 5u, personality::exti>(1u);
    ALLOY_CHECK((alloy::claim::sub_held<inst_s, 5u, personality::exti>()));
}

ALLOY_TEST(claim_sub_release_of_an_unclaimed_line_is_a_no_op) {
    // Disarming a pin that was never armed was always harmless, and stays so —
    // trapping there would break code that clears an interrupt defensively.
    alloy::claim::sub_release<inst_t, 5u, personality::exti>(0u);
    ALLOY_CHECK(!(alloy::claim::sub_held<inst_t, 5u, personality::exti>()));
}

ALLOY_TEST(claim_sub_release_by_the_wrong_claimant_traps) {
    // The second half of the same defect: `pb5.clear_on_edge()` while PA5 owns
    // line 5 used to silently disarm PA5's interrupt.
    ALLOY_CHECK(refuses([] {
        alloy::claim::sub_shared<inst_u, 5u, personality::exti>(0u);
        alloy::claim::sub_release<inst_u, 5u, personality::exti>(1u);
    }));
}

// ── The cross-TU case: one object, or one per compilation? ───────────────
//
// Every test above lives in ONE .cpp, which is precisely the arrangement that
// cannot tell an `inline` variable template apart from the per-binder `static`
// it replaced — both look identical inside a single translation unit. These
// three name instances that tests/test_claim_tu2.cpp also names.

ALLOY_TEST(claim_crosses_translation_units) {
    // Claimed HERE, observed THERE. If `owner<Inst>` were per-TU this would be
    // false and the whole repair would be decoration.
    ALLOY_CHECK(!alloy::test::seen_held_in_tu2());
    alloy::claim::exclusive<alloy::test::cross_tu_seen, personality::uart>();
    ALLOY_CHECK(alloy::test::seen_held_in_tu2());
}

ALLOY_TEST(claim_second_owner_in_another_translation_unit_traps) {
    // THE CASE THE MECHANISM EXISTS FOR, and the one C++ cannot make a compile
    // error: a driver in libs/ and an app in main.cpp both opening usart2.
    ALLOY_CHECK(refuses([] {
        alloy::test::claim_trap_in_tu2();
        alloy::claim::exclusive<alloy::test::cross_tu_trap, personality::uart>();
    }));
}

ALLOY_TEST(claim_sub_resource_crosses_translation_units) {
    // ...and the sub scope crosses TUs too: `sub_witness<Inst, Sub>` is one
    // object, so the second pin is refused from anywhere in the image.
    ALLOY_CHECK(refuses([] {
        alloy::test::claim_sub_in_tu2(0u);
        alloy::claim::sub_shared<alloy::test::cross_tu_sub, 5u, personality::exti>(1u);
    }));
}
