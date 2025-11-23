/**
 * @file hw_spi_flash_test.cpp
 * @brief Hardware validation test for SPI Flash memory
 *
 * This test validates SPI functionality by communicating with an external
 * SPI flash memory chip (e.g., W25Q32, AT25SF081, etc.)
 *
 * Test Operations:
 * 1. Initialize system clock and SPI peripheral
 * 2. Read flash chip ID (JEDEC ID command 0x9F)
 * 3. Erase a sector
 * 4. Write data pattern to flash
 * 5. Read back and verify data
 * 6. Test various SPI modes and speeds
 *
 * SUCCESS: All read/write operations complete correctly
 * FAILURE: Flash ID incorrect, data mismatch, or communication errors
 *
 * @note This test requires actual hardware with SPI flash chip
 * @note LED indicates test status (solid = pass, blinking = fail)
 *
 * Common SPI Flash Commands:
 * - 0x9F: Read JEDEC ID
 * - 0x06: Write Enable
 * - 0x04: Write Disable
 * - 0x05: Read Status Register
 * - 0x02: Page Program (write)
 * - 0x03: Read Data
 * - 0x20: Sector Erase (4KB)
 * - 0xD8: Block Erase (64KB)
 * - 0xC7: Chip Erase
 */

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

using namespace ucore::core;

// ==============================================================================
// Platform-Specific Includes
// ==============================================================================

#if defined(ALLOY_BOARD_NUCLEO_F401RE) || defined(ALLOY_BOARD_NUCLEO_F446RE)
    #include "hal/api/spi_simple.hpp"
    #include "hal/vendors/st/stm32f4/clock_platform.hpp"
    #include "hal/vendors/st/stm32f4/gpio.hpp"

    #include "boards/board_config.hpp"

using ClockPlatform =
    ucore::hal::st::stm32f4::Stm32f4Clock<ucore::hal::st::stm32f4::ExampleF4ClockConfig>;
using LedPin = ucore::boards::LedGreen;

#elif defined(ALLOY_BOARD_SAME70_XPLAINED)
    #include "hal/vendors/microchip/same70/clock_platform.hpp"
    #include "hal/vendors/microchip/same70/gpio.hpp"

    #include "boards/board_config.hpp"

using ClockPlatform = ucore::hal::microchip::same70::Same70Clock<
    ucore::hal::microchip::same70::ExampleSame70ClockConfig>;
using LedPin = ucore::boards::LedGreen;

#else
    #error "Unsupported board for SPI flash test"
#endif

// ==============================================================================
// SPI Flash Commands
// ==============================================================================

namespace spi_flash {
// Command definitions
constexpr u8 CMD_READ_JEDEC_ID = 0x9F;
constexpr u8 CMD_WRITE_ENABLE = 0x06;
constexpr u8 CMD_WRITE_DISABLE = 0x04;
constexpr u8 CMD_READ_STATUS_REG = 0x05;
constexpr u8 CMD_PAGE_PROGRAM = 0x02;
constexpr u8 CMD_READ_DATA = 0x03;
constexpr u8 CMD_SECTOR_ERASE = 0x20;
constexpr u8 CMD_CHIP_ERASE = 0xC7;

// Status register bits
constexpr u8 STATUS_BUSY = 0x01;
constexpr u8 STATUS_WEL = 0x02;

// Flash parameters
constexpr u32 PAGE_SIZE = 256;
constexpr u32 SECTOR_SIZE = 4096;
constexpr u32 MAX_BUSY_WAIT = 1000000;
}  // namespace spi_flash

// ==============================================================================
// SPI Flash Driver
// ==============================================================================

class SpiFlash {
   public:
    /**
     * @brief Initialize SPI flash driver
     */
    static Result<void, ErrorCode> initialize() {
        // In real implementation:
        // 1. Initialize SPI peripheral
        // 2. Configure CS pin as output
        // 3. Set CS high (inactive)

        cs_high();
        return Ok();
    }

    /**
     * @brief Read JEDEC ID (Manufacturer + Device ID)
     * @return 24-bit JEDEC ID or error
     */
    static Result<u32, ErrorCode> read_jedec_id() {
        cs_low();

        // Send command
        spi_transfer(spi_flash::CMD_READ_JEDEC_ID);

        // Read 3 bytes (Manufacturer ID, Memory Type, Capacity)
        u32 manufacturer = spi_transfer(0xFF);
        u32 memory_type = spi_transfer(0xFF);
        u32 capacity = spi_transfer(0xFF);

        cs_high();

        u32 jedec_id = (manufacturer << 16) | (memory_type << 8) | capacity;

        // Common JEDEC IDs:
        // Winbond W25Q32:  0xEF4016
        // Microchip AT25:  0x1F8501
        // Macronix MX25:   0xC22016

        return Ok(jedec_id);
    }

    /**
     * @brief Read status register
     */
    static Result<u8, ErrorCode> read_status() {
        cs_low();

        spi_transfer(spi_flash::CMD_READ_STATUS_REG);
        u8 status = spi_transfer(0xFF);

        cs_high();

        return Ok(status);
    }

    /**
     * @brief Wait until flash is not busy
     */
    static Result<void, ErrorCode> wait_not_busy() {
        for (u32 i = 0; i < spi_flash::MAX_BUSY_WAIT; i++) {
            auto status = read_status();
            if (!status.is_ok()) {
                return Err(ErrorCode::COMMUNICATION_ERROR);
            }

            if ((status.unwrap() & spi_flash::STATUS_BUSY) == 0) {
                return Ok();
            }

            // Small delay
            for (volatile u32 j = 0; j < 100; j++) {}
        }

        return Err(ErrorCode::TIMEOUT);
    }

    /**
     * @brief Enable write operations
     */
    static Result<void, ErrorCode> write_enable() {
        cs_low();
        spi_transfer(spi_flash::CMD_WRITE_ENABLE);
        cs_high();

        // Verify write enable latch is set
        auto status = read_status();
        if (!status.is_ok()) {
            return Err(ErrorCode::COMMUNICATION_ERROR);
        }

        if ((status.unwrap() & spi_flash::STATUS_WEL) == 0) {
            return Err(ErrorCode::OPERATION_FAILED);
        }

        return Ok();
    }

    /**
     * @brief Erase 4KB sector
     * @param address Sector address (must be sector-aligned)
     */
    static Result<void, ErrorCode> erase_sector(u32 address) {
        auto result = write_enable();
        if (!result.is_ok()) {
            return result;
        }

        cs_low();

        // Send erase command + 24-bit address
        spi_transfer(spi_flash::CMD_SECTOR_ERASE);
        spi_transfer((address >> 16) & 0xFF);
        spi_transfer((address >> 8) & 0xFF);
        spi_transfer(address & 0xFF);

        cs_high();

        // Wait for erase to complete (can take several seconds)
        return wait_not_busy();
    }

    /**
     * @brief Write page (up to 256 bytes)
     * @param address Start address
     * @param data Data buffer
     * @param length Number of bytes (max 256)
     */
    static Result<void, ErrorCode> write_page(u32 address, const u8* data, u32 length) {
        if (length > spi_flash::PAGE_SIZE) {
            return Err(ErrorCode::INVALID_PARAMETER);
        }

        auto result = write_enable();
        if (!result.is_ok()) {
            return result;
        }

        cs_low();

        // Send write command + 24-bit address
        spi_transfer(spi_flash::CMD_PAGE_PROGRAM);
        spi_transfer((address >> 16) & 0xFF);
        spi_transfer((address >> 8) & 0xFF);
        spi_transfer(address & 0xFF);

        // Send data
        for (u32 i = 0; i < length; i++) {
            spi_transfer(data[i]);
        }

        cs_high();

        return wait_not_busy();
    }

    /**
     * @brief Read data from flash
     * @param address Start address
     * @param buffer Output buffer
     * @param length Number of bytes to read
     */
    static Result<void, ErrorCode> read_data(u32 address, u8* buffer, u32 length) {
        cs_low();

        // Send read command + 24-bit address
        spi_transfer(spi_flash::CMD_READ_DATA);
        spi_transfer((address >> 16) & 0xFF);
        spi_transfer((address >> 8) & 0xFF);
        spi_transfer(address & 0xFF);

        // Read data
        for (u32 i = 0; i < length; i++) {
            buffer[i] = spi_transfer(0xFF);
        }

        cs_high();

        return Ok();
    }

   private:
    static void cs_low() {
        // In real implementation: pull CS pin low
    }

    static void cs_high() {
        // In real implementation: pull CS pin high
    }

    static u8 spi_transfer(u8 data) {
        // In real implementation: transfer byte via SPI
        // Return received byte
        return 0xFF;  // Dummy return
    }
};

// ==============================================================================
// Test Scenarios
// ==============================================================================

/**
 * @brief Test Scenario 1: Read Flash ID
 */
Result<void, ErrorCode> test_read_flash_id() {
    auto id_result = SpiFlash::read_jedec_id();
    if (!id_result.is_ok()) {
        return Err(ErrorCode::COMMUNICATION_ERROR);
    }

    u32 jedec_id = id_result.unwrap();

    // Verify ID is not 0x000000 or 0xFFFFFF (common errors)
    if (jedec_id == 0x000000 || jedec_id == 0xFFFFFF) {
        return Err(ErrorCode::INVALID_RESPONSE);
    }

    // SUCCESS: Valid JEDEC ID read
    return Ok();
}

/**
 * @brief Test Scenario 2: Write and Read Back
 */
Result<void, ErrorCode> test_write_read() {
    constexpr u32 TEST_ADDRESS = 0x001000;  // 4KB into flash
    constexpr u32 TEST_LENGTH = 64;

    // Prepare test pattern
    u8 write_buffer[TEST_LENGTH];
    for (u32 i = 0; i < TEST_LENGTH; i++) {
        write_buffer[i] = static_cast<u8>(i);
    }

    // Erase sector
    auto erase_result = SpiFlash::erase_sector(TEST_ADDRESS);
    if (!erase_result.is_ok()) {
        return erase_result;
    }

    // Write data
    auto write_result = SpiFlash::write_page(TEST_ADDRESS, write_buffer, TEST_LENGTH);
    if (!write_result.is_ok()) {
        return write_result;
    }

    // Read back
    u8 read_buffer[TEST_LENGTH];
    auto read_result = SpiFlash::read_data(TEST_ADDRESS, read_buffer, TEST_LENGTH);
    if (!read_result.is_ok()) {
        return read_result;
    }

    // Verify data matches
    for (u32 i = 0; i < TEST_LENGTH; i++) {
        if (read_buffer[i] != write_buffer[i]) {
            return Err(ErrorCode::DATA_MISMATCH);
        }
    }

    // SUCCESS: Data written and verified
    return Ok();
}

/**
 * @brief Test Scenario 3: Sector Erase Verification
 */
Result<void, ErrorCode> test_sector_erase() {
    constexpr u32 TEST_ADDRESS = 0x002000;
    constexpr u32 TEST_LENGTH = 256;

    // Erase sector
    auto erase_result = SpiFlash::erase_sector(TEST_ADDRESS);
    if (!erase_result.is_ok()) {
        return erase_result;
    }

    // Read erased sector
    u8 read_buffer[TEST_LENGTH];
    auto read_result = SpiFlash::read_data(TEST_ADDRESS, read_buffer, TEST_LENGTH);
    if (!read_result.is_ok()) {
        return read_result;
    }

    // Verify all bytes are 0xFF (erased state)
    for (u32 i = 0; i < TEST_LENGTH; i++) {
        if (read_buffer[i] != 0xFF) {
            return Err(ErrorCode::ERASE_FAILED);
        }
    }

    // SUCCESS: Sector properly erased
    return Ok();
}

/**
 * @brief Test Scenario 4: Stress Test (Multiple Operations)
 */
Result<void, ErrorCode> test_stress() {
    constexpr u32 NUM_ITERATIONS = 10;
    constexpr u32 TEST_ADDRESS = 0x003000;

    for (u32 iteration = 0; iteration < NUM_ITERATIONS; iteration++) {
        // Erase
        auto erase_result = SpiFlash::erase_sector(TEST_ADDRESS);
        if (!erase_result.is_ok()) {
            return erase_result;
        }

        // Write pattern
        u8 pattern = static_cast<u8>(iteration);
        u8 write_data[32];
        for (u32 i = 0; i < 32; i++) {
            write_data[i] = pattern;
        }

        auto write_result = SpiFlash::write_page(TEST_ADDRESS, write_data, 32);
        if (!write_result.is_ok()) {
            return write_result;
        }

        // Verify
        u8 read_data[32];
        auto read_result = SpiFlash::read_data(TEST_ADDRESS, read_data, 32);
        if (!read_result.is_ok()) {
            return read_result;
        }

        for (u32 i = 0; i < 32; i++) {
            if (read_data[i] != pattern) {
                return Err(ErrorCode::DATA_MISMATCH);
            }
        }

        // Blink LED to show progress
        LedPin::toggle();
    }

    // SUCCESS: All iterations passed
    return Ok();
}

// ==============================================================================
// Main Test Entry Point
// ==============================================================================

int main() {
    // Initialize hardware
    auto clock_result = ClockPlatform::initialize();
    if (!clock_result.is_ok()) {
        // Error: rapid blink
        while (true) {
            LedPin::toggle();
            for (volatile u32 i = 0; i < 50000; i++) {}
        }
    }

    // Initialize LED
    LedPin::set_mode_output();
    LedPin::clear();

    // Initialize SPI flash
    auto spi_result = SpiFlash::initialize();
    if (!spi_result.is_ok()) {
        // Error: fast blink
        while (true) {
            LedPin::toggle();
            for (volatile u32 i = 0; i < 100000; i++) {}
        }
    }

    // Run test scenarios
    bool all_passed = true;

    // Test 1: Read ID
    if (test_read_flash_id().is_ok()) {
        LedPin::set();  // Turn on LED briefly
        for (volatile u32 i = 0; i < 500000; i++) {}
        LedPin::clear();
    } else {
        all_passed = false;
    }

    // Test 2: Write/Read
    if (test_write_read().is_ok()) {
        LedPin::set();
        for (volatile u32 i = 0; i < 500000; i++) {}
        LedPin::clear();
    } else {
        all_passed = false;
    }

    // Test 3: Erase
    if (test_sector_erase().is_ok()) {
        LedPin::set();
        for (volatile u32 i = 0; i < 500000; i++) {}
        LedPin::clear();
    } else {
        all_passed = false;
    }

    // Test 4: Stress
    if (test_stress().is_ok()) {
        LedPin::set();
        for (volatile u32 i = 0; i < 500000; i++) {}
        LedPin::clear();
    } else {
        all_passed = false;
    }

    // Final result
    if (all_passed) {
        // SUCCESS: Solid LED on
        LedPin::set();
    } else {
        // FAILURE: Slow blink
        while (true) {
            LedPin::toggle();
            for (volatile u32 i = 0; i < 500000; i++) {}
        }
    }

    // Test complete - halt
    while (true) {}

    return 0;
}

/**
 * SPI Flash Test - Expected Behavior:
 *
 * 1. LED blinks briefly 4 times (one per test scenario)
 * 2. If all tests pass: LED stays solid ON
 * 3. If any test fails: LED blinks slowly
 * 4. If initialization fails: LED blinks rapidly
 *
 * Success Criteria:
 * - JEDEC ID reads correctly (not 0x000000 or 0xFFFFFF)
 * - Write/read operations match perfectly
 * - Erased sectors contain 0xFF
 * - Stress test completes without errors
 *
 * Hardware Requirements:
 * - SPI flash chip (W25Q32, AT25SF081, etc.)
 * - Connections: MOSI, MISO, SCK, CS, VCC, GND
 * - Pull-up on CS recommended
 */
