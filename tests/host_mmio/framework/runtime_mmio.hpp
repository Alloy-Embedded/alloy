#pragma once

#include "hal/detail/runtime_ops.hpp"
#include "host_mmio/framework/mmio_space.hpp"

namespace alloy::test::mmio {

namespace rt = alloy::hal::detail::runtime;

[[nodiscard]] inline auto active_runtime_mmio() -> mmio_space*& {
    static auto* space = static_cast<mmio_space*>(nullptr);
    return space;
}

[[nodiscard]] inline auto host_mmio_read32(std::uintptr_t address) -> std::uint32_t {
    return active_runtime_mmio()->read32(address);
}

inline void host_mmio_write32(std::uintptr_t address, std::uint32_t value) {
    active_runtime_mmio()->write32(address, value);
}

class runtime_mmio_scope {
  public:
    explicit runtime_mmio_scope(mmio_space& mmio)
        : previous_space_{active_runtime_mmio()},
          previous_hooks_{rt::test_support::host_mmio_hooks()} {
        active_runtime_mmio() = &mmio;
        auto& hooks = rt::test_support::host_mmio_hooks();
        hooks.read32 = &host_mmio_read32;
        hooks.write32 = &host_mmio_write32;
    }

    runtime_mmio_scope(const runtime_mmio_scope&) = delete;
    auto operator=(const runtime_mmio_scope&) -> runtime_mmio_scope& = delete;

    ~runtime_mmio_scope() {
        rt::test_support::host_mmio_hooks() = previous_hooks_;
        active_runtime_mmio() = previous_space_;
    }

  private:
    mmio_space* previous_space_;
    rt::test_support::MmioHooks previous_hooks_{};
};

}  // namespace alloy::test::mmio
