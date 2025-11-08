/**
 * @file uart_mock.hpp
 * @brief Mock UART registers for unit testing (without hardware)
 *
 * This file provides a mock implementation of UART registers that can be
 * used for testing the UART template without requiring actual hardware.
 *
 * Key features:
 * - Simulates UART register behavior
 * - Tracks register writes for verification
 * - Provides test helpers
 * - Zero hardware dependency
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

// Include actual UART register definition for compatibility
#include "hal/vendors/atmel/same70/atsame70q21/registers/uart0_registers.hpp"

namespace alloy::hal::test {

// Import the actual register type
using UartRegisters = alloy::hal::atmel::same70::atsame70q21::uart0::UART0_Registers;

/**
 * @brief Mock UART registers for testing with write/read interception
 *
 * This class extends the actual UART0_Registers to add testing capabilities.
 * Since we can't override member variables in C++, we provide accessor methods
 * and a periodic sync mechanism to capture register writes.
 */
class MockUartRegisters : public UartRegisters {
   public:
    // Test state
    bool tx_ready = true;                   // TXRDY flag state
    bool rx_ready = false;                  // RXRDY flag state
    std::vector<uint8_t> transmitted_data;  // Captured writes to THR
    std::vector<uint8_t> receive_buffer;    // Data to return from RHR
    size_t receive_index = 0;
    uint32_t last_thr_value = 0;  // Track last THR write for detection

    /**
     * @brief Reset mock to initial state
     */
    void reset() {
        // Reset all register values (from base class)
        this->CR = 0;
        this->MR = 0;
        this->IER = 0;
        this->IDR = 0;
        this->IMR = 0;
        this->SR = 0;
        this->RHR = 0;
        this->THR = 0;
        this->BRGR = 0;
        this->CMPR = 0;
        this->WPMR = 0;

        tx_ready = true;
        rx_ready = false;
        transmitted_data.clear();
        receive_buffer.clear();
        receive_index = 0;
        last_thr_value = 0;
    }

    /**
     * @brief Sync mock state with actual register writes
     *
     * This must be called after UART operations to capture THR writes
     * since we can't intercept direct member access in C++.
     */
    void sync_thr() {
        if (this->THR != last_thr_value) {
            transmitted_data.push_back(static_cast<uint8_t>(this->THR & 0xFF));
            last_thr_value = this->THR;
            // Simulate TX becoming busy then ready again
            set_tx_ready(true);
        }
    }

    /**
     * @brief Sync RHR register with receive buffer
     *
     * Called after RHR is read to load next byte and update RXRDY flag
     */
    void sync_rhr() {
        if (receive_index < receive_buffer.size()) {
            // Load next byte into RHR
            this->RHR = receive_buffer[receive_index++];
            set_rx_ready(true);
        } else {
            // No more data - clear RXRDY
            set_rx_ready(false);
        }
    }

    /**
     * @brief Simulate TXRDY flag (transmitter ready)
     */
    void set_tx_ready(bool ready) {
        tx_ready = ready;
        if (ready) {
            this->SR |= (1u << 1);  // TXRDY bit
        } else {
            this->SR &= ~(1u << 1);
        }
    }

    /**
     * @brief Simulate RXRDY flag (receiver ready)
     */
    void set_rx_ready(bool ready) {
        rx_ready = ready;
        if (ready) {
            this->SR |= (1u << 0);  // RXRDY bit
        } else {
            this->SR &= ~(1u << 0);
        }
    }

    /**
     * @brief Queue data to be received
     *
     * Adds data to the receive buffer and sets up RHR with the first byte
     */
    void queue_receive_data(const uint8_t* data, size_t size) {
        receive_buffer.insert(receive_buffer.end(), data, data + size);
        receive_index = 0;
        if (!receive_buffer.empty()) {
            // Load first byte into RHR
            this->RHR = receive_buffer[0];
            receive_index = 1;
            set_rx_ready(true);
        }
    }

    /**
     * @brief Simulate reading from RHR
     */
    uint32_t read_rhr() {
        if (receive_index < receive_buffer.size()) {
            uint32_t value = receive_buffer[receive_index++];

            // If we've consumed all data, clear RXRDY
            if (receive_index >= receive_buffer.size()) {
                set_rx_ready(false);
            }

            return value;
        }
        return 0;
    }

    /**
     * @brief Simulate writing to THR
     */
    void write_thr(uint32_t value) {
        transmitted_data.push_back(static_cast<uint8_t>(value & 0xFF));

        // Simulate transmitter becoming busy, then ready again
        set_tx_ready(false);
        // In real test, we would call set_tx_ready(true) after some time
    }

    /**
     * @brief Get transmitted data as string (for verification)
     */
    std::string get_transmitted_string() const {
        return std::string(transmitted_data.begin(), transmitted_data.end());
    }

    /**
     * @brief Check if specific data was transmitted
     */
    bool transmitted_matches(const char* expected) const {
        size_t len = std::strlen(expected);
        if (transmitted_data.size() != len)
            return false;
        return std::memcmp(transmitted_data.data(), expected, len) == 0;
    }
};

/**
 * @brief Mock PMC (Power Management Controller)
 */
class MockPmc {
   public:
    uint32_t PCER0 = 0;  // Peripheral Clock Enable Register

    void reset() { PCER0 = 0; }

    bool is_clock_enabled(uint32_t peripheral_id) const {
        return (PCER0 & (1u << peripheral_id)) != 0;
    }
};

/**
 * @brief Global mock instances (replaceable for testing)
 */
inline MockUartRegisters* g_mock_uart = nullptr;
inline MockPmc* g_mock_pmc = nullptr;

/**
 * @brief RAII helper to set up and tear down mocks
 */
class MockUartFixture {
   public:
    MockUartFixture() {
        uart_mock.reset();
        pmc_mock.reset();
        g_mock_uart = &uart_mock;
        g_mock_pmc = &pmc_mock;
    }

    ~MockUartFixture() {
        g_mock_uart = nullptr;
        g_mock_pmc = nullptr;
    }

    MockUartRegisters& uart() { return uart_mock; }
    MockPmc& pmc() { return pmc_mock; }

   private:
    MockUartRegisters uart_mock;
    MockPmc pmc_mock;
};

}  // namespace alloy::hal::test
