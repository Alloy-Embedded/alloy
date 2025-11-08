/**
 * @file test_scoped_device.cpp
 * @brief Unit tests for RAII device wrappers
 *
 * Tests cover:
 * - ScopedDevice creation and destruction
 * - ScopedI2c bus locking
 * - ScopedSpi bus locking with chip select
 * - RAII guarantees (automatic cleanup)
 * - Error handling
 * - Move semantics where applicable
 */

#include <cassert>
#include <iostream>

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/scoped_device.hpp"
#include "core/scoped_i2c.hpp"
#include "core/scoped_spi.hpp"

using namespace alloy::core;

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                                \
    void test_##name();                                           \
    void run_test_##name() {                                      \
        tests_run++;                                              \
        std::cout << "Running test: " #name << "...";             \
        try {                                                     \
            test_##name();                                        \
            tests_passed++;                                       \
            std::cout << " PASS" << std::endl;                    \
        } catch (const std::exception& e) {                       \
            std::cout << " FAIL: " << e.what() << std::endl;      \
        } catch (...) {                                           \
            std::cout << " FAIL: Unknown exception" << std::endl; \
        }                                                         \
    }                                                             \
    void test_##name()

#define ASSERT(condition)                                              \
    do {                                                               \
        if (!(condition)) {                                            \
            throw std::runtime_error("Assertion failed: " #condition); \
        }                                                              \
    } while (0)

// ============================================================================
// Mock Device Classes for Testing
// ============================================================================

/**
 * @brief Mock device for testing basic ScopedDevice
 */
class MockDevice {
   public:
    MockDevice() : m_open(false), m_access_count(0) {}

    bool isOpen() const { return m_open; }
    void open() { m_open = true; }
    void close() { m_open = false; }

    void doSomething() { m_access_count++; }
    int getAccessCount() const { return m_access_count; }

   private:
    bool m_open;
    int m_access_count;
};

/**
 * @brief Mock I2C device for testing ScopedI2c
 */
class MockI2cDevice {
   public:
    MockI2cDevice()
        : m_open(false),
          m_write_count(0),
          m_read_count(0),
          m_register_write_count(0),
          m_register_read_count(0) {}

    bool isOpen() const { return m_open; }
    void open() { m_open = true; }
    void close() { m_open = false; }

    Result<size_t, ErrorCode> write(uint8_t addr, const uint8_t* data, size_t size) {
        m_write_count++;
        m_last_addr = addr;
        return Ok(static_cast<size_t>(size));
    }

    Result<size_t, ErrorCode> read(uint8_t addr, uint8_t* data, size_t size) {
        m_read_count++;
        m_last_addr = addr;
        // Fill with dummy data
        for (size_t i = 0; i < size; i++) {
            data[i] = static_cast<uint8_t>(i);
        }
        return Ok(static_cast<size_t>(size));
    }

    Result<void, ErrorCode> writeRegister(uint8_t addr, uint8_t reg, uint8_t value) {
        m_register_write_count++;
        m_last_addr = addr;
        m_last_reg = reg;
        m_last_value = value;
        return Ok();
    }

    Result<void, ErrorCode> readRegister(uint8_t addr, uint8_t reg, uint8_t* value) {
        m_register_read_count++;
        m_last_addr = addr;
        m_last_reg = reg;
        *value = 0x42;  // Dummy value
        return Ok();
    }

    // Test accessors
    int getWriteCount() const { return m_write_count; }
    int getReadCount() const { return m_read_count; }
    int getRegisterWriteCount() const { return m_register_write_count; }
    int getRegisterReadCount() const { return m_register_read_count; }
    uint8_t getLastAddr() const { return m_last_addr; }
    uint8_t getLastReg() const { return m_last_reg; }
    uint8_t getLastValue() const { return m_last_value; }

   private:
    bool m_open;
    int m_write_count;
    int m_read_count;
    int m_register_write_count;
    int m_register_read_count;
    uint8_t m_last_addr;
    uint8_t m_last_reg;
    uint8_t m_last_value;
};

/**
 * @brief Mock SPI device for testing ScopedSpi
 */
class MockSpiDevice {
   public:
    enum class ChipSelect : uint8_t { CS0 = 0, CS1 = 1, CS2 = 2, CS3 = 3 };

    MockSpiDevice()
        : m_open(false),
          m_transfer_count(0),
          m_write_count(0),
          m_read_count(0),
          m_last_cs(ChipSelect::CS0) {}

    bool isOpen() const { return m_open; }
    void open() { m_open = true; }
    void close() { m_open = false; }

    Result<size_t, ErrorCode> transfer(const uint8_t* tx, uint8_t* rx, size_t size, ChipSelect cs) {
        m_transfer_count++;
        m_last_cs = cs;
        // Echo back with increment
        for (size_t i = 0; i < size; i++) {
            rx[i] = tx[i] + 1;
        }
        return Ok(static_cast<size_t>(size));
    }

    Result<size_t, ErrorCode> write(const uint8_t* data, size_t size, ChipSelect cs) {
        m_write_count++;
        m_last_cs = cs;
        return Ok(static_cast<size_t>(size));
    }

    Result<size_t, ErrorCode> read(uint8_t* data, size_t size, ChipSelect cs) {
        m_read_count++;
        m_last_cs = cs;
        // Fill with dummy data
        for (size_t i = 0; i < size; i++) {
            data[i] = static_cast<uint8_t>(i);
        }
        return Ok(static_cast<size_t>(size));
    }

    // Test accessors
    int getTransferCount() const { return m_transfer_count; }
    int getWriteCount() const { return m_write_count; }
    int getReadCount() const { return m_read_count; }
    ChipSelect getLastCs() const { return m_last_cs; }

   private:
    bool m_open;
    int m_transfer_count;
    int m_write_count;
    int m_read_count;
    ChipSelect m_last_cs;
};

// ============================================================================
// ScopedDevice Tests
// ============================================================================

TEST(scoped_device_create_closed_fails) {
    MockDevice device;
    // Device is not open

    auto result = ScopedDevice<MockDevice>::create(device);

    ASSERT(!result.is_ok());
    ASSERT(result.err() == ErrorCode::NotInitialized);
}

TEST(scoped_device_create_open_succeeds) {
    MockDevice device;
    device.open();

    auto result = ScopedDevice<MockDevice>::create(device);

    ASSERT(result.is_ok());
}

TEST(scoped_device_pointer_access) {
    MockDevice device;
    device.open();

    auto result = ScopedDevice<MockDevice>::create(device);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    scoped->doSomething();

    ASSERT(device.getAccessCount() == 1);
}

TEST(scoped_device_reference_access) {
    MockDevice device;
    device.open();

    auto result = ScopedDevice<MockDevice>::create(device);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    (*scoped).doSomething();

    ASSERT(device.getAccessCount() == 1);
}

TEST(scoped_device_get_method) {
    MockDevice device;
    device.open();

    auto result = ScopedDevice<MockDevice>::create(device);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    MockDevice* ptr = scoped.get();
    ptr->doSomething();

    ASSERT(device.getAccessCount() == 1);
}

TEST(scoped_device_move_constructor) {
    MockDevice device;
    device.open();

    auto result = ScopedDevice<MockDevice>::create(device);
    ASSERT(result.is_ok());

    auto scoped1 = std::move(result).unwrap();
    auto scoped2 = std::move(scoped1);

    scoped2->doSomething();
    ASSERT(device.getAccessCount() == 1);
}

// ============================================================================
// ScopedI2c Tests
// ============================================================================

TEST(scoped_i2c_create_closed_fails) {
    MockI2cDevice device;
    // Device is not open

    auto result = ScopedI2c<MockI2cDevice>::create(device);

    ASSERT(!result.is_ok());
    ASSERT(result.err() == ErrorCode::NotInitialized);
}

TEST(scoped_i2c_create_open_succeeds) {
    MockI2cDevice device;
    device.open();

    auto result = ScopedI2c<MockI2cDevice>::create(device);

    ASSERT(result.is_ok());
}

TEST(scoped_i2c_write_operation) {
    MockI2cDevice device;
    device.open();

    auto result = ScopedI2c<MockI2cDevice>::create(device);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    uint8_t data[] = {0x01, 0x02, 0x03};
    auto write_result = scoped.write(0x50, data, 3);

    ASSERT(write_result.is_ok());
    ASSERT(write_std::move(result).unwrap() == 3);
    ASSERT(device.getWriteCount() == 1);
    ASSERT(device.getLastAddr() == 0x50);
}

TEST(scoped_i2c_read_operation) {
    MockI2cDevice device;
    device.open();

    auto result = ScopedI2c<MockI2cDevice>::create(device);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    uint8_t data[5];
    auto read_result = scoped.read(0x51, data, 5);

    ASSERT(read_result.is_ok());
    ASSERT(read_std::move(result).unwrap() == 5);
    ASSERT(device.getReadCount() == 1);
    ASSERT(device.getLastAddr() == 0x51);
}

TEST(scoped_i2c_write_register) {
    MockI2cDevice device;
    device.open();

    auto result = ScopedI2c<MockI2cDevice>::create(device);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    auto write_result = scoped.writeRegister(0x52, 0x10, 0xAB);

    ASSERT(write_result.is_ok());
    ASSERT(device.getRegisterWriteCount() == 1);
    ASSERT(device.getLastAddr() == 0x52);
    ASSERT(device.getLastReg() == 0x10);
    ASSERT(device.getLastValue() == 0xAB);
}

TEST(scoped_i2c_read_register) {
    MockI2cDevice device;
    device.open();

    auto result = ScopedI2c<MockI2cDevice>::create(device);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    uint8_t value;
    auto read_result = scoped.readRegister(0x53, 0x20, &value);

    ASSERT(read_result.is_ok());
    ASSERT(value == 0x42);  // Mock returns 0x42
    ASSERT(device.getRegisterReadCount() == 1);
    ASSERT(device.getLastAddr() == 0x53);
    ASSERT(device.getLastReg() == 0x20);
}

TEST(scoped_i2c_helper_function) {
    MockI2cDevice device;
    device.open();

    auto result = makeScopedI2c(device, 200);  // 200ms timeout

    ASSERT(result.is_ok());
}

TEST(scoped_i2c_scope_lifetime) {
    MockI2cDevice device;
    device.open();

    // Ensure device is accessible within scope
    {
        auto result = ScopedI2c<MockI2cDevice>::create(device);
        ASSERT(result.is_ok());

        auto scoped = std::move(result).unwrap();
        uint8_t data[] = {0xAA};
        scoped.write(0x54, data, 1);
    }  // Scoped destructor called here

    // Device should still be accessible after scope ends
    ASSERT(device.getWriteCount() == 1);
}

// ============================================================================
// ScopedSpi Tests
// ============================================================================

TEST(scoped_spi_create_closed_fails) {
    MockSpiDevice device;
    // Device is not open

    auto result = ScopedSpi<MockSpiDevice, MockSpiDevice::ChipSelect>::create(
        device, MockSpiDevice::ChipSelect::CS0);

    ASSERT(!result.is_ok());
    ASSERT(result.err() == ErrorCode::NotInitialized);
}

TEST(scoped_spi_create_open_succeeds) {
    MockSpiDevice device;
    device.open();

    auto result = ScopedSpi<MockSpiDevice, MockSpiDevice::ChipSelect>::create(
        device, MockSpiDevice::ChipSelect::CS0);

    ASSERT(result.is_ok());
}

TEST(scoped_spi_transfer_operation) {
    MockSpiDevice device;
    device.open();

    auto result = ScopedSpi<MockSpiDevice, MockSpiDevice::ChipSelect>::create(
        device, MockSpiDevice::ChipSelect::CS1);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    uint8_t tx_data[] = {0x10, 0x20, 0x30};
    uint8_t rx_data[3];
    auto transfer_result = scoped.transfer(tx_data, rx_data, 3);

    ASSERT(transfer_result.is_ok());
    ASSERT(transfer_std::move(result).unwrap() == 3);
    ASSERT(device.getTransferCount() == 1);
    ASSERT(device.getLastCs() == MockSpiDevice::ChipSelect::CS1);

    // Mock increments each byte
    ASSERT(rx_data[0] == 0x11);
    ASSERT(rx_data[1] == 0x21);
    ASSERT(rx_data[2] == 0x31);
}

TEST(scoped_spi_write_operation) {
    MockSpiDevice device;
    device.open();

    auto result = ScopedSpi<MockSpiDevice, MockSpiDevice::ChipSelect>::create(
        device, MockSpiDevice::ChipSelect::CS2);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    uint8_t data[] = {0xAA, 0xBB};
    auto write_result = scoped.write(data, 2);

    ASSERT(write_result.is_ok());
    ASSERT(write_std::move(result).unwrap() == 2);
    ASSERT(device.getWriteCount() == 1);
    ASSERT(device.getLastCs() == MockSpiDevice::ChipSelect::CS2);
}

TEST(scoped_spi_read_operation) {
    MockSpiDevice device;
    device.open();

    auto result = ScopedSpi<MockSpiDevice, MockSpiDevice::ChipSelect>::create(
        device, MockSpiDevice::ChipSelect::CS3);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    uint8_t data[4];
    auto read_result = scoped.read(data, 4);

    ASSERT(read_result.is_ok());
    ASSERT(read_std::move(result).unwrap() == 4);
    ASSERT(device.getReadCount() == 1);
    ASSERT(device.getLastCs() == MockSpiDevice::ChipSelect::CS3);
}

TEST(scoped_spi_write_byte) {
    MockSpiDevice device;
    device.open();

    auto result = ScopedSpi<MockSpiDevice, MockSpiDevice::ChipSelect>::create(
        device, MockSpiDevice::ChipSelect::CS0);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    auto write_result = scoped.writeByte(0xCC);

    ASSERT(write_result.is_ok());
    ASSERT(device.getWriteCount() == 1);
}

TEST(scoped_spi_read_byte) {
    MockSpiDevice device;
    device.open();

    auto result = ScopedSpi<MockSpiDevice, MockSpiDevice::ChipSelect>::create(
        device, MockSpiDevice::ChipSelect::CS0);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    auto read_result = scoped.readByte();

    ASSERT(read_result.is_ok());
    ASSERT(read_std::move(result).unwrap() == 0);  // Mock returns 0 for first byte
    ASSERT(device.getReadCount() == 1);
}

TEST(scoped_spi_transfer_byte) {
    MockSpiDevice device;
    device.open();

    auto result = ScopedSpi<MockSpiDevice, MockSpiDevice::ChipSelect>::create(
        device, MockSpiDevice::ChipSelect::CS0);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    auto transfer_result = scoped.transferByte(0x55);

    ASSERT(transfer_result.is_ok());
    ASSERT(transfer_std::move(result).unwrap() == 0x56);  // Mock increments by 1
    ASSERT(device.getTransferCount() == 1);
}

TEST(scoped_spi_chip_select_access) {
    MockSpiDevice device;
    device.open();

    auto result = ScopedSpi<MockSpiDevice, MockSpiDevice::ChipSelect>::create(
        device, MockSpiDevice::ChipSelect::CS2);
    ASSERT(result.is_ok());

    auto scoped = std::move(result).unwrap();
    ASSERT(scoped.chipSelect() == MockSpiDevice::ChipSelect::CS2);
}

TEST(scoped_spi_helper_function) {
    MockSpiDevice device;
    device.open();

    auto result = makeScopedSpi(device, MockSpiDevice::ChipSelect::CS1, 150);

    ASSERT(result.is_ok());
}

TEST(scoped_spi_scope_lifetime) {
    MockSpiDevice device;
    device.open();

    // Ensure device is accessible within scope
    {
        auto result = ScopedSpi<MockSpiDevice, MockSpiDevice::ChipSelect>::create(
            device, MockSpiDevice::ChipSelect::CS0);
        ASSERT(result.is_ok());

        auto scoped = std::move(result).unwrap();
        uint8_t data[] = {0xFF};
        scoped.write(data, 1);
    }  // Scoped destructor called here

    // Device should still be accessible after scope ends
    ASSERT(device.getWriteCount() == 1);
}

// ============================================================================
// RAII Guarantee Tests
// ============================================================================

TEST(raii_i2c_cleanup_early_return) {
    MockI2cDevice device;
    device.open();

    auto test_func = [&device]() -> bool {
        auto result = ScopedI2c<MockI2cDevice>::create(device);
        if (!result.is_ok()) {
            return false;
        }

        auto scoped = std::move(result).unwrap();
        uint8_t data[] = {0x12};
        scoped.write(0x60, data, 1);

        // Early return - destructor should still be called
        return true;
    };

    ASSERT(test_func() == true);
    ASSERT(device.getWriteCount() == 1);
}

TEST(raii_spi_cleanup_early_return) {
    MockSpiDevice device;
    device.open();

    auto test_func = [&device]() -> bool {
        auto result = ScopedSpi<MockSpiDevice, MockSpiDevice::ChipSelect>::create(
            device, MockSpiDevice::ChipSelect::CS0);
        if (!result.is_ok()) {
            return false;
        }

        auto scoped = std::move(result).unwrap();
        uint8_t data[] = {0x34};
        scoped.write(data, 1);

        // Early return - destructor should still be called
        return true;
    };

    ASSERT(test_func() == true);
    ASSERT(device.getWriteCount() == 1);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "RAII Device Wrapper Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // ScopedDevice tests
    std::cout << "--- ScopedDevice Tests ---" << std::endl;
    run_test_scoped_device_create_closed_fails();
    run_test_scoped_device_create_open_succeeds();
    run_test_scoped_device_pointer_access();
    run_test_scoped_device_reference_access();
    run_test_scoped_device_get_method();
    run_test_scoped_device_move_constructor();

    // ScopedI2c tests
    std::cout << std::endl << "--- ScopedI2c Tests ---" << std::endl;
    run_test_scoped_i2c_create_closed_fails();
    run_test_scoped_i2c_create_open_succeeds();
    run_test_scoped_i2c_write_operation();
    run_test_scoped_i2c_read_operation();
    run_test_scoped_i2c_write_register();
    run_test_scoped_i2c_read_register();
    run_test_scoped_i2c_helper_function();
    run_test_scoped_i2c_scope_lifetime();

    // ScopedSpi tests
    std::cout << std::endl << "--- ScopedSpi Tests ---" << std::endl;
    run_test_scoped_spi_create_closed_fails();
    run_test_scoped_spi_create_open_succeeds();
    run_test_scoped_spi_transfer_operation();
    run_test_scoped_spi_write_operation();
    run_test_scoped_spi_read_operation();
    run_test_scoped_spi_write_byte();
    run_test_scoped_spi_read_byte();
    run_test_scoped_spi_transfer_byte();
    run_test_scoped_spi_chip_select_access();
    run_test_scoped_spi_helper_function();
    run_test_scoped_spi_scope_lifetime();

    // RAII tests
    std::cout << std::endl << "--- RAII Guarantee Tests ---" << std::endl;
    run_test_raii_i2c_cleanup_early_return();
    run_test_raii_spi_cleanup_early_return();

    // Summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Tests run:    " << tests_run << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << (tests_run - tests_passed) << std::endl;
    std::cout << "========================================" << std::endl;

    return (tests_run == tests_passed) ? 0 : 1;
}
