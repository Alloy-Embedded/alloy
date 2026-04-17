#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace alloy::test::mmio {

enum class access_kind : std::uint8_t {
    read,
    write,
    set_bits,
    clear_bits,
    write_masked,
};

struct access {
    access_kind kind{};
    std::uintptr_t address{};
    std::uint32_t value{};
    std::uint32_t mask{};

    auto operator==(const access&) const -> bool = default;
};

class trace_log {
  public:
    void record(access entry) { entries_.push_back(entry); }

    [[nodiscard]] auto entries() const -> std::span<const access> { return entries_; }

    void clear() { entries_.clear(); }

  private:
    std::vector<access> entries_{};
};

}  // namespace alloy::test::mmio
