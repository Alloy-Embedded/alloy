#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace alloy::test::mmio {

class boot_probe {
  public:
    void enter_reset_handler() {
        reset_handler_entered_ = true;
        checkpoints_.emplace_back("reset-handler");
    }

    void checkpoint(std::string_view label) { checkpoints_.emplace_back(label); }

    void enter_main() {
        main_entered_ = true;
        checkpoints_.emplace_back("main");
    }

    [[nodiscard]] auto reset_handler_entered() const -> bool { return reset_handler_entered_; }

    [[nodiscard]] auto main_entered() const -> bool { return main_entered_; }

    [[nodiscard]] auto checkpoints() const -> const std::vector<std::string>& { return checkpoints_; }

  private:
    bool reset_handler_entered_{false};
    bool main_entered_{false};
    std::vector<std::string> checkpoints_{};
};

}  // namespace alloy::test::mmio
