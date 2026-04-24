// Compile test: AT24MAC402 seed driver instantiates against the documented
// public I2C HAL surface. Exercises every public method so drift in
// `write()` / `write_read()` or the driver itself fails the build.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "drivers/memory/at24mac402/at24mac402.hpp"

namespace {

struct MockI2cBus {
    [[nodiscard]] auto write(std::uint16_t /*address*/,
                             std::span<const std::uint8_t> /*data*/) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }

    [[nodiscard]] auto read(std::uint16_t /*address*/,
                            std::span<std::uint8_t> /*data*/) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }

    [[nodiscard]] auto write_read(std::uint16_t /*address*/,
                                  std::span<const std::uint8_t> /*tx*/,
                                  std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0;
        return alloy::core::Ok();
    }
};

[[maybe_unused]] void compile_at24mac402_against_public_i2c_handle() {
    MockI2cBus bus;
    alloy::drivers::memory::at24mac402::Device eeprom{bus};

    (void)eeprom.init();

    std::array<std::uint8_t, 32> read_buffer{};
    (void)eeprom.read(0x00, read_buffer);

    std::array<std::uint8_t, 48> payload{};
    (void)eeprom.write(0x08, payload);

    std::array<std::uint8_t, 6> eui48{};
    (void)eeprom.read_eui48(eui48);

    std::array<std::uint8_t, 8> eui64{};
    (void)eeprom.read_eui64(eui64);

    std::array<std::uint8_t, alloy::drivers::memory::at24mac402::kSerialNumberLengthBytes>
        serial{};
    (void)eeprom.read_serial_number(serial);
}

}  // namespace
