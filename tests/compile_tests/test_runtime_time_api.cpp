#include BOARD_HEADER

#include "time.hpp"

using BoardTime = alloy::time::source<board::BoardSysTick>;

static_assert(alloy::time::Duration::from_millis(5).as_micros() == 5000u);
static_assert((alloy::time::Duration::from_seconds(2) - alloy::time::Duration::from_millis(500))
                  .as_millis() == 1500u);
static_assert((alloy::time::Instant::from_micros(7000) - alloy::time::Instant::from_micros(2000))
                  .as_micros() == 5000u);

int main() {
    [[maybe_unused]] auto now = BoardTime::now();
    [[maybe_unused]] auto uptime = BoardTime::uptime();
    [[maybe_unused]] auto deadline = BoardTime::deadline_after(alloy::time::Duration::from_millis(5));
    [[maybe_unused]] auto expired = BoardTime::expired(deadline);
    [[maybe_unused]] auto remaining = BoardTime::remaining(deadline);
    return 0;
}
