// The server's register bank contract — the piece the reference C library
// never had (no datastore, no response builder: its server role could parse
// a request and then had nothing to consult and no way to answer).
//
// One concept, four Modbus data spaces, per-item access: coils (RW bits),
// discrete inputs (RO bits), holding registers (RW u16), input registers
// (RO u16). Every method returns bool in the alloy honest-error convention —
// false means THE address does not exist in that space, and the server turns
// it into the wire's exception 0x02 (illegal data address). Per-item rather
// than per-block so a model can compute values on demand (a live ADC reading
// behind an input register) without buffering a block protocol.
//
// The user's model is plain memory or code behind these six methods; the
// server template never sees past them, so the same rtu_server serves a
// static bank, a computed map, or a mix.

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace alloy::lib::modbus {

template <class M>
concept DataModel = requires(M& m, const M& cm, std::uint16_t addr,
                             std::uint16_t& reg, bool& bit) {
    { cm.read_coil(addr, bit) } -> std::same_as<bool>;
    { m.write_coil(addr, true) } -> std::same_as<bool>;
    { cm.read_discrete_input(addr, bit) } -> std::same_as<bool>;
    { cm.read_holding(addr, reg) } -> std::same_as<bool>;
    { m.write_holding(addr, std::uint16_t{}) } -> std::same_as<bool>;
    { cm.read_input(addr, reg) } -> std::same_as<bool>;
};

// The 80%-case model: N holding registers at addresses 0..N-1, plain public
// memory the application reads and writes directly between polls. The other
// three spaces hold NO addresses — a request against them earns exception
// 0x02, honestly reporting what this device is rather than mirroring.
template <std::size_t N>
struct holding_bank {
    static_assert(N >= 1 && N <= 0x10000, "a holding bank is 1..65536 registers");

    std::uint16_t regs[N]{};

    [[nodiscard]] bool read_holding(std::uint16_t addr, std::uint16_t& v) const {
        if (addr >= N) {
            return false;
        }
        v = regs[addr];
        return true;
    }
    [[nodiscard]] bool write_holding(std::uint16_t addr, std::uint16_t v) {
        if (addr >= N) {
            return false;
        }
        regs[addr] = v;
        return true;
    }

    [[nodiscard]] bool read_coil(std::uint16_t, bool&) const { return false; }
    [[nodiscard]] bool write_coil(std::uint16_t, bool) { return false; }
    [[nodiscard]] bool read_discrete_input(std::uint16_t, bool&) const { return false; }
    [[nodiscard]] bool read_input(std::uint16_t, std::uint16_t&) const { return false; }
};
static_assert(DataModel<holding_bank<1>>);

}  // namespace alloy::lib::modbus
