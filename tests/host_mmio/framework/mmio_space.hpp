#pragma once

#include "host_mmio/framework/mmio_trace.hpp"

#include <cstdint>
#include <unordered_map>

namespace alloy::test::mmio {

class mmio_space {
  public:
    explicit mmio_space(trace_log& trace) : trace_{trace} {}

    void preload(std::uintptr_t address, std::uint32_t value) { registers_[address] = value; }

    [[nodiscard]] auto read32(std::uintptr_t address) -> std::uint32_t {
        const auto value = current(address);
        trace_.record(access{.kind = access_kind::read, .address = address, .value = value, .mask = 0u});
        return value;
    }

    void write32(std::uintptr_t address, std::uint32_t value) {
        registers_[address] = value;
        trace_.record(access{.kind = access_kind::write, .address = address, .value = value, .mask = 0u});
    }

    void set_bits(std::uintptr_t address, std::uint32_t mask) {
        registers_[address] = current(address) | mask;
        trace_.record(access{.kind = access_kind::set_bits, .address = address, .value = registers_[address], .mask = mask});
    }

    void clear_bits(std::uintptr_t address, std::uint32_t mask) {
        registers_[address] = current(address) & ~mask;
        trace_.record(access{.kind = access_kind::clear_bits, .address = address, .value = registers_[address], .mask = mask});
    }

    void write_masked(std::uintptr_t address, std::uint32_t mask, std::uint32_t value) {
        registers_[address] = (current(address) & ~mask) | (value & mask);
        trace_.record(access{.kind = access_kind::write_masked, .address = address, .value = registers_[address], .mask = mask});
    }

    [[nodiscard]] auto peek(std::uintptr_t address) const -> std::uint32_t { return current(address); }

  private:
    [[nodiscard]] auto current(std::uintptr_t address) const -> std::uint32_t {
        if (const auto it = registers_.find(address); it != registers_.end()) {
            return it->second;
        }
        return 0u;
    }

    trace_log& trace_;
    std::unordered_map<std::uintptr_t, std::uint32_t> registers_{};
};

}  // namespace alloy::test::mmio
