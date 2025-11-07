/**
 * @file uart.hpp
 * @brief Template-based UART implementation for Linux using POSIX termios
 *
 * This file implements the UART peripheral for Linux using POSIX serial I/O
 * with the SAME interface as embedded platforms (SAME70, ESP32, etc.).
 *
 * Design Principles:
 * - Template-based: Device path resolved at compile-time
 * - Zero virtual functions: Same template approach as embedded platforms
 * - POSIX termios: Uses standard Linux serial I/O
 * - Compatible API: Same interface as SAME70 UART
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 * @see openspec/changes/platform-abstraction/specs/platform-interface-layer/spec.md
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// POSIX headers for serial I/O
#include <fcntl.h>      // open()
#include <unistd.h>     // close(), read(), write()
#include <termios.h>    // termios, tcgetattr(), tcsetattr()
#include <sys/ioctl.h>  // ioctl()
#include <cstring>      // memset

// macOS compatibility (doesn't have CRTSCTS in termios.h)
#ifndef CRTSCTS
#define CRTSCTS 0  // Hardware flow control not available on macOS via POSIX API
#endif

// macOS uses numeric baudrate values directly
#ifndef B57600
#define B57600 57600
#endif
#ifndef B115200
#define B115200 115200
#endif
#ifndef B230400
#define B230400 230400
#endif

namespace alloy::hal::linux {

// Import types from core and hal namespaces
using alloy::core::Result;
using alloy::core::ErrorCode;
using alloy::hal::Baudrate;

/**
 * @brief Template-based UART peripheral for Linux
 *
 * This class provides a template-based UART implementation that satisfies
 * the same interface as embedded UARTs, using POSIX termios for serial I/O.
 *
 * Template Parameters:
 * - DEVICE_PATH: Serial device path (e.g., "/dev/ttyUSB0")
 *
 * Example usage:
 * @code
 * // Define UART instance
 * constexpr const char* USB0_PATH = "/dev/ttyUSB0";
 * using UartUsb0 = Uart<USB0_PATH>;
 *
 * // Use it
 * auto uart = UartUsb0{};
 * uart.open();
 * uart.setBaudrate(Baudrate::e115200);
 * uart.write(data, size);
 * uart.close();
 * @endcode
 *
 * @tparam DEVICE_PATH Serial device path (compile-time string)
 */
template <const char* DEVICE_PATH>
class Uart {
public:
    // Compile-time constants
    static constexpr const char* device_path = DEVICE_PATH;

    /**
     * @brief Constructor
     *
     * Note: Does NOT open device - call open() first
     */
    constexpr Uart() : m_fd(-1), m_opened(false) {}

    /**
     * @brief Destructor - ensures device is closed
     */
    ~Uart() {
        if (m_opened) {
            close();
        }
    }

    // Delete copy/move to match embedded UART behavior
    Uart(const Uart&) = delete;
    Uart& operator=(const Uart&) = delete;
    Uart(Uart&&) = delete;
    Uart& operator=(Uart&&) = delete;

    /**
     * @brief Open UART device
     *
     * This method:
     * 1. Opens the serial device (e.g., /dev/ttyUSB0)
     * 2. Configures termios for raw mode (8N1)
     * 3. Sets default baudrate (115200)
     * 4. Disables hardware/software flow control
     *
     * @return Result<void> Ok() if successful, Err() if already open or device not found
     */
    Result<void> open() {
        if (m_opened) {
            return Result<void>::error(ErrorCode::AlreadyInitialized);
        }

        // Open device (read/write, no controlling terminal, non-blocking initially)
        m_fd = ::open(DEVICE_PATH, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (m_fd < 0) {
            return Result<void>::error(ErrorCode::HardwareError);
        }

        // Get current termios configuration
        struct termios tty;
        if (tcgetattr(m_fd, &tty) != 0) {
            ::close(m_fd);
            m_fd = -1;
            return Result<void>::error(ErrorCode::HardwareError);
        }

        // Configure for raw mode (8N1, no flow control)
        // Input flags - disable all processing
        tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);

        // Output flags - disable all processing
        tty.c_oflag &= ~OPOST;

        // Control flags - 8N1, enable receiver, ignore modem control
        tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CSIZE | CRTSCTS);
        tty.c_cflag |= (CS8 | CREAD | CLOCAL);

        // Local flags - disable canonical mode, echo, signals
        tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

        // Read configuration - non-blocking with small timeout
        tty.c_cc[VMIN] = 0;   // Return immediately with available data
        tty.c_cc[VTIME] = 1;  // 0.1 second timeout

        // Set default baudrate (115200)
        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);

        // Apply configuration
        if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
            ::close(m_fd);
            m_fd = -1;
            return Result<void>::error(ErrorCode::HardwareError);
        }

        // Flush any stale data
        tcflush(m_fd, TCIOFLUSH);

        m_opened = true;
        return Result<void>::ok();
    }

    /**
     * @brief Close UART device
     *
     * @return Result<void> Ok() if successful, Err() if not open
     */
    Result<void> close() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        if (m_fd >= 0) {
            ::close(m_fd);
            m_fd = -1;
        }

        m_opened = false;
        return Result<void>::ok();
    }

    /**
     * @brief Write data to UART (blocking)
     *
     * Sends bytes to the serial device. This is a blocking operation.
     *
     * @param data Pointer to data buffer
     * @param size Number of bytes to write
     * @return Result<size_t> Number of bytes written, or error
     */
    Result<size_t> write(const uint8_t* data, size_t size) {
        if (!m_opened) {
            return Result<size_t>::error(ErrorCode::NotInitialized);
        }

        if (data == nullptr) {
            return Result<size_t>::error(ErrorCode::InvalidParameter);
        }

        if (size == 0) {
            return Result<size_t>::ok(0);
        }

        ssize_t bytes_written = ::write(m_fd, data, size);

        if (bytes_written < 0) {
            return Result<size_t>::error(ErrorCode::CommunicationError);
        }

        // Ensure data is transmitted (drain output buffer)
        tcdrain(m_fd);

        return Result<size_t>::ok(static_cast<size_t>(bytes_written));
    }

    /**
     * @brief Read data from UART (blocking with timeout)
     *
     * Reads available bytes from the serial device.
     * Returns when either:
     * - Requested number of bytes are read
     * - Timeout occurs (configured via termios VTIME)
     *
     * @param data Pointer to buffer for received data
     * @param size Maximum number of bytes to read
     * @return Result<size_t> Number of bytes read (may be less than size), or error
     */
    Result<size_t> read(uint8_t* data, size_t size) {
        if (!m_opened) {
            return Result<size_t>::error(ErrorCode::NotInitialized);
        }

        if (data == nullptr) {
            return Result<size_t>::error(ErrorCode::InvalidParameter);
        }

        if (size == 0) {
            return Result<size_t>::ok(0);
        }

        ssize_t bytes_read = ::read(m_fd, data, size);

        if (bytes_read < 0) {
            return Result<size_t>::error(ErrorCode::CommunicationError);
        }

        return Result<size_t>::ok(static_cast<size_t>(bytes_read));
    }

    /**
     * @brief Set UART baudrate
     *
     * Changes the baudrate of the serial device.
     *
     * @param baudrate Desired baudrate (see hal/types.hpp)
     * @return Result<void> Ok() if successful, Err() on failure
     */
    Result<void> setBaudrate(Baudrate baudrate) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        // Get current termios configuration
        struct termios tty;
        if (tcgetattr(m_fd, &tty) != 0) {
            return Result<void>::error(ErrorCode::HardwareError);
        }

        // Map Baudrate enum to termios speed_t
        speed_t speed;
        switch (baudrate) {
            case Baudrate::e9600:   speed = B9600;   break;
            case Baudrate::e19200:  speed = B19200;  break;
            case Baudrate::e38400:  speed = B38400;  break;
            case Baudrate::e57600:  speed = B57600;  break;
            case Baudrate::e115200: speed = B115200; break;
            case Baudrate::e230400: speed = B230400; break;
#ifdef B460800
            case Baudrate::e460800: speed = B460800; break;
#else
            case Baudrate::e460800:
                return Result<void>::error(ErrorCode::NotSupported);
#endif
#ifdef B921600
            case Baudrate::e921600: speed = B921600; break;
#else
            case Baudrate::e921600:
                return Result<void>::error(ErrorCode::NotSupported);
#endif
            default:
                return Result<void>::error(ErrorCode::InvalidParameter);
        }

        // Set input and output speed
        cfsetispeed(&tty, speed);
        cfsetospeed(&tty, speed);

        // Apply configuration
        if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
            return Result<void>::error(ErrorCode::HardwareError);
        }

        return Result<void>::ok();
    }

    /**
     * @brief Get number of bytes available to read
     *
     * @return Result<size_t> Number of bytes available, or error
     */
    Result<size_t> available() const {
        if (!m_opened) {
            return Result<size_t>::error(ErrorCode::NotInitialized);
        }

        int bytes_available = 0;
        if (ioctl(m_fd, FIONREAD, &bytes_available) < 0) {
            return Result<size_t>::error(ErrorCode::HardwareError);
        }

        return Result<size_t>::ok(static_cast<size_t>(bytes_available));
    }

    /**
     * @brief Flush input and output buffers
     *
     * @return Result<void> Ok() if successful
     */
    Result<void> flush() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        if (tcflush(m_fd, TCIOFLUSH) != 0) {
            return Result<void>::error(ErrorCode::HardwareError);
        }

        return Result<void>::ok();
    }

    /**
     * @brief Check if UART is open
     *
     * @return true if open, false otherwise
     */
    bool isOpen() const {
        return m_opened;
    }

private:
    int m_fd;          ///< File descriptor for serial device
    bool m_opened;     ///< Device open state
};

// ============================================================================
// Predefined UART instances for common devices
// ============================================================================

// Device path strings (must have external linkage for template parameters)
inline constexpr const char ttyUSB0_path[] = "/dev/ttyUSB0";
inline constexpr const char ttyUSB1_path[] = "/dev/ttyUSB1";
inline constexpr const char ttyACM0_path[] = "/dev/ttyACM0";
inline constexpr const char ttyACM1_path[] = "/dev/ttyACM1";
inline constexpr const char ttyS0_path[] = "/dev/ttyS0";
inline constexpr const char ttyS1_path[] = "/dev/ttyS1";

// Type aliases for common serial devices
using UartUsb0 = Uart<ttyUSB0_path>;  // USB-to-serial adapter 0
using UartUsb1 = Uart<ttyUSB1_path>;  // USB-to-serial adapter 1
using UartAcm0 = Uart<ttyACM0_path>;  // USB CDC/ACM device 0 (Arduino, etc.)
using UartAcm1 = Uart<ttyACM1_path>;  // USB CDC/ACM device 1
using UartS0 = Uart<ttyS0_path>;      // Hardware serial port 0
using UartS1 = Uart<ttyS1_path>;      // Hardware serial port 1

} // namespace alloy::hal::linux
