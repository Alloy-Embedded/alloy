// Capability (passkey) demo: a dangerous op — here "arm the actuators" — takes an
// alloy::passkey<PowerSupervisor>, so ONLY PowerSupervisor can call it. The
// supervisor mints its key and runs the op; the commented rogue call in main()
// would be a COMPILE error (main is not PowerSupervisor's friend), not a surprise
// at 2 a.m. Nothing here is board-specific beyond the debug UART.
#include <alloy/board.hpp>
#include <alloy/secure.hpp>

using namespace alloy::literals;

namespace {
struct PowerSupervisor;  // forward: the one authority allowed to arm the op

// The guarded operation — its signature demands PowerSupervisor's key.
template <class Uart>
void arm_actuators(const Uart& uart, alloy::passkey<PowerSupervisor> /*authorized*/) {
    uart.write("actuators ARMED (authorized by PowerSupervisor)\r\n");
}

struct PowerSupervisor {
    // A member of the befriended type, so it alone can mint the key.
    template <class Uart>
    void run(const Uart& uart) const {
        arm_actuators(uart, alloy::passkey<PowerSupervisor>{});
    }
};
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy secure (passkey) demo\r\n");

    // The authorized path: only through PowerSupervisor.
    const PowerSupervisor supervisor;
    supervisor.run(uart);

    // Uncommenting the rogue call below fails to COMPILE — main() cannot mint
    // PowerSupervisor's key, so it cannot arm the actuators:
    //   arm_actuators(uart, alloy::passkey<PowerSupervisor>{});
    uart.write("(a rogue arm_actuators() call would not compile)\r\n");

    for (;;) {
        alloy::sleep_for(1s);
    }
}
