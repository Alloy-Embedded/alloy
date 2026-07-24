// Async + a vendored ecosystem driver, together. Two coroutine tasks share one
// core: one blinks the LED, the other reads an SHT31 temperature/humidity sensor
// over I2C once a second and prints the temperature. The sensor's I2C calls are
// blocking, but they run INSIDE an async task — so the blink keeps its own
// cadence and never stalls waiting on the sensor. No RTOS, no heap, no #ifdef.
//
// The driver came from the library ecosystem (`alloy lib add sht31`), vendored
// under ./libs and pulled onto the include path by the build. Retarget to any
// board with an I2C role by changing the id in alloy.toml.

#include <cstdint>

#include <alloy/async/delay.hpp>
#include <alloy/async/executor.hpp>
#include <alloy/async/task.hpp>
#include <alloy/board.hpp>
#include <alloy/delay.hpp>

#include <sht31.hpp>

using namespace alloy::async;
using namespace alloy::literals;

namespace {

executor<4> sched;
task_storage<128> blink_frame;
task_storage<192> sensor_frame;  // sized from `alloy frame-audit` (96 B) + headroom

task blink(task_storage<128>&) {
    for (;;) {
        board::led.toggle();
        co_await delay(250ms);
    }
}

task sample(task_storage<192>&) {
    auto bus = board::i2c::open({.speed_hz = 100'000});
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    alloy::delay d;
    alloy::lib::sht31 sensor{bus, d};

    for (;;) {
        co_await delay(1'000ms);
        const auto r = sensor.measure();  // blocking I2C — only THIS task waits
        if (r.valid) {
            uart.write(static_cast<std::uint8_t>(static_cast<int>(r.temperature_c)));
            uart.write("\r\n");
        }
    }
}

}  // namespace

int main() {
    board::init();
    sched.spawn(blink(blink_frame));
    sched.spawn(sample(sensor_frame));
    sched.run();  // cooperative superloop; never returns
}
