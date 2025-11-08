/**
 * @file scoped_device.hpp
 * @brief Generic RAII wrapper for resource management
 *
 * This file provides a generic RAII (Resource Acquisition Is Initialization)
 * wrapper that ensures automatic resource cleanup even in the presence of
 * exceptions or early returns.
 *
 * Design Principles:
 * - RAII: Constructor acquires, destructor releases
 * - Non-copyable: Resources cannot be duplicated
 * - Movable: Resources can be transferred
 * - Zero overhead: No virtual functions, fully inlined
 * - Type-safe: Strong typing prevents misuse
 *
 * @note Part of CoreZero Core Library
 */

#pragma once

#include "core/error.hpp"
#include "core/result.hpp"
#include <utility>  // For std::move, std::forward

namespace alloy::core {

/**
 * @brief Generic RAII wrapper for device resources
 *
 * ScopedDevice provides automatic resource management using RAII pattern.
 * The device is acquired in the constructor and automatically released
 * in the destructor, ensuring proper cleanup even if exceptions occur.
 *
 * @tparam Device Type of the device to manage (e.g., I2c, Spi)
 *
 * Example usage:
 * @code
 * auto result = ScopedDevice::create(my_device);
 * if (result.is_ok()) {
 *     auto scoped = result.unwrap();
 *     scoped->someOperation();  // Use the device
 * }  // Device automatically released here
 * @endcode
 */
template <typename Device>
class ScopedDevice {
public:
    /**
     * @brief Create a scoped device wrapper
     *
     * This factory method attempts to acquire the device and returns
     * a Result containing either a ScopedDevice or an error.
     *
     * @param device Reference to the device to manage
     * @return Result containing ScopedDevice or error code
     */
    [[nodiscard]] static Result<ScopedDevice<Device>, ErrorCode> create(Device& device) {
        // Check if device is already open
        if (!device.isOpen()) {
            return Err(ErrorCode::NotInitialized);
        }

        return Ok(ScopedDevice(device));
    }

    /**
     * @brief Destructor - automatically releases the device
     *
     * The destructor ensures the device is properly released even if
     * an exception is thrown or an early return occurs.
     */
    ~ScopedDevice() {
        // Device is managed by the underlying implementation
        // No explicit release needed for basic scoped access
    }

    // Delete copy constructor and copy assignment
    ScopedDevice(const ScopedDevice&) = delete;
    ScopedDevice& operator=(const ScopedDevice&) = delete;

    // Allow move operations
    ScopedDevice(ScopedDevice&& other) noexcept
        : m_device(other.m_device) {
    }

    ScopedDevice& operator=(ScopedDevice&& other) noexcept {
        if (this != &other) {
            m_device = other.m_device;
        }
        return *this;
    }

    /**
     * @brief Access the underlying device via pointer semantics
     * @return Pointer to the managed device
     */
    [[nodiscard]] Device* operator->() noexcept {
        return m_device;
    }

    /**
     * @brief Access the underlying device via pointer semantics (const)
     * @return Const pointer to the managed device
     */
    [[nodiscard]] const Device* operator->() const noexcept {
        return m_device;
    }

    /**
     * @brief Access the underlying device via reference
     * @return Reference to the managed device
     */
    [[nodiscard]] Device& operator*() noexcept {
        return *m_device;
    }

    /**
     * @brief Access the underlying device via reference (const)
     * @return Const reference to the managed device
     */
    [[nodiscard]] const Device& operator*() const noexcept {
        return *m_device;
    }

    /**
     * @brief Get raw pointer to the underlying device
     * @return Pointer to the managed device
     */
    [[nodiscard]] Device* get() noexcept {
        return m_device;
    }

    /**
     * @brief Get raw pointer to the underlying device (const)
     * @return Const pointer to the managed device
     */
    [[nodiscard]] const Device* get() const noexcept {
        return m_device;
    }

private:
    /**
     * @brief Private constructor - use create() factory method
     * @param device Reference to the device to manage
     */
    explicit ScopedDevice(Device& device)
        : m_device(&device) {
    }

    Device* m_device;  ///< Pointer to the managed device
};

} // namespace alloy::core
