// Capability tokens: make a dangerous operation demand proof of authorization at
// COMPILE TIME, so "any code that linked can call it" becomes "only the one type
// allowed to can call it".
//
//   struct PowerSupervisor { void run(); };
//   // guarded op — its signature demands the key:
//   void enable_high_voltage(alloy::passkey<PowerSupervisor>);
//
//   void PowerSupervisor::run() { enable_high_voltage({}); }  // OK: mints its key
//   void some_isr()            { enable_high_voltage({}); }   // COMPILE ERROR
//
// passkey<Holder> has a private default constructor befriended to Holder, so
// only Holder can mint one from nothing. Once minted it forwards freely (a
// helper the supervisor delegates to can take one by value), but no unrelated
// call site can conjure one — reset(), flash erase, motor-enable and friends get
// a checked authorized caller instead of a comment that says "don't call this".

#pragma once

namespace alloy {

template <class Holder>
class passkey {
    friend Holder;
    passkey() = default;  // only Holder can mint one from scratch

public:
    // Freely forwarded once you hold one (the supervisor can hand it to a
    // helper), but never default-constructed from an unauthorized context.
    passkey(const passkey&) = default;
    passkey& operator=(const passkey&) = default;
    ~passkey() = default;
};

}  // namespace alloy
