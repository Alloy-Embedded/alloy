/**
 * @file esp_error.hpp
 * @brief ESP-IDF error integration for Alloy
 *
 * Extends Alloy's Result<T> and ErrorCode types with ESP-IDF esp_err_t
 * integration. Provides conversion functions and factory methods for working
 * with ESP-IDF errors in Alloy code.
 *
 * Example:
 * @code
 * #include "core/error.hpp"
 * #include "core/esp_error.hpp"
 *
 * Result<IPAddress> connect_wifi() {
 *     esp_err_t err = esp_wifi_connect();
 *     if (err != ESP_OK) {
 *         return esp_result_error<IPAddress>(err);
 *     }
 *     return Result<IPAddress>::ok(get_ip_address());
 * }
 * @endcode
 */

#pragma once

#include "error.hpp"

#ifdef ESP_PLATFORM
#include "esp_err.h"
#include "esp_log.h"
#endif

namespace alloy::core {

#ifdef ESP_PLATFORM

/**
 * @brief Convert ESP-IDF esp_err_t to Alloy ErrorCode
 *
 * Maps common ESP-IDF error codes to appropriate Alloy ErrorCode values.
 * Unknown ESP-IDF errors map to ErrorCode::Unknown.
 *
 * @param esp_error ESP-IDF error code
 * @return Corresponding Alloy ErrorCode
 */
inline ErrorCode esp_to_error_code(esp_err_t esp_error) noexcept {
    switch (esp_error) {
        case ESP_OK:
            return ErrorCode::Ok;

        case ESP_ERR_NO_MEM:
            return ErrorCode::HardwareError; // Out of memory

        case ESP_ERR_INVALID_ARG:
        case ESP_ERR_INVALID_SIZE:
            return ErrorCode::InvalidParameter;

        case ESP_ERR_INVALID_STATE:
            return ErrorCode::NotInitialized;

        case ESP_ERR_NOT_FOUND:
            return ErrorCode::NotSupported;

        case ESP_ERR_NOT_SUPPORTED:
            return ErrorCode::NotSupported;

        case ESP_ERR_TIMEOUT:
            return ErrorCode::Timeout;

        case ESP_FAIL:
            return ErrorCode::HardwareError;

        default:
            // WiFi, NVS, HTTP and other component-specific errors
            // are handled by checking error code ranges
            if (esp_error >= 0x3000 && esp_error < 0x4000) {
                // WiFi errors (ESP_ERR_WIFI_BASE = 0x3001)
                return ErrorCode::CommunicationError;
            } else if (esp_error >= 0x1100 && esp_error < 0x1200) {
                // NVS errors (ESP_ERR_NVS_BASE = 0x1100)
                return ErrorCode::HardwareError;
            } else if (esp_error >= 0x7000 && esp_error < 0x8000) {
                // HTTP errors (ESP_ERR_HTTP_BASE = 0x7000)
                return ErrorCode::CommunicationError;
            }
            return ErrorCode::Unknown;
    }
}

/**
 * @brief Create an error Result from ESP-IDF error
 *
 * Helper function to create a Result<T>::error() from an esp_err_t code.
 * Automatically converts the ESP-IDF error to Alloy ErrorCode.
 *
 * @tparam T The value type of the Result
 * @param esp_error ESP-IDF error code
 * @return Result<T> in error state
 *
 * Example:
 * @code
 * esp_err_t err = esp_wifi_init(&cfg);
 * if (err != ESP_OK) {
 *     return esp_result_error<bool>(err);
 * }
 * @endcode
 */
template<typename T>
inline Result<T> esp_result_error(esp_err_t esp_error) {
    return Result<T>::error(esp_to_error_code(esp_error));
}

/**
 * @brief Create a void Result from ESP-IDF error
 *
 * Specialization for operations that return Result<void>.
 *
 * @param esp_error ESP-IDF error code
 * @return Result<void> in error state
 */
inline Result<void> esp_result_error_void(esp_err_t esp_error) {
    return Result<void>::error(esp_to_error_code(esp_error));
}

/**
 * @brief Check ESP-IDF error and convert to Result<void>
 *
 * Convenience function that checks if an ESP-IDF operation succeeded
 * and returns an appropriate Result<void>.
 *
 * @param esp_error ESP-IDF error code to check
 * @return Result<void>::ok() if ESP_OK, otherwise Result<void>::error()
 *
 * Example:
 * @code
 * Result<void> init_wifi() {
 *     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
 *     return esp_check(esp_wifi_init(&cfg));
 * }
 * @endcode
 */
inline Result<void> esp_check(esp_err_t esp_error) {
    if (esp_error == ESP_OK) {
        return Result<void>::ok();
    }
    return esp_result_error_void(esp_error);
}

/**
 * @brief Macro to check ESP-IDF result and return error if failed
 *
 * This macro simplifies error handling by checking an ESP-IDF function call
 * and returning early if it fails. Similar to ESP-IDF's ESP_ERROR_CHECK but
 * returns a Result instead of aborting.
 *
 * Example:
 * @code
 * Result<void> init_components() {
 *     ESP_TRY(esp_wifi_init(&cfg));
 *     ESP_TRY(esp_wifi_start());
 *     return Result<void>::ok();
 * }
 * @endcode
 */
#define ESP_TRY(expr) do { \
    esp_err_t __err = (expr); \
    if (__err != ESP_OK) { \
        return ::alloy::core::esp_result_error_void(__err); \
    } \
} while(0)

/**
 * @brief Macro to check ESP-IDF result and return typed error if failed
 *
 * Similar to ESP_TRY but for functions returning Result<T>.
 *
 * @param T The value type of the Result to return
 * @param expr Expression that returns esp_err_t
 *
 * Example:
 * @code
 * Result<IPAddress> get_ip() {
 *     ESP_TRY_T(IPAddress, esp_wifi_connect());
 *     return Result<IPAddress>::ok(read_ip_address());
 * }
 * @endcode
 */
#define ESP_TRY_T(T, expr) do { \
    esp_err_t __err = (expr); \
    if (__err != ESP_OK) { \
        return ::alloy::core::esp_result_error<T>(__err); \
    } \
} while(0)

/**
 * @brief Get error name string from ESP-IDF error
 *
 * Returns a human-readable string describing the ESP-IDF error.
 *
 * @param esp_error ESP-IDF error code
 * @return String describing the error
 */
inline const char* esp_error_name(esp_err_t esp_error) {
    return esp_err_to_name(esp_error);
}

/**
 * @brief Log ESP-IDF error with tag
 *
 * Convenience function to log an ESP-IDF error with a custom tag.
 *
 * @param tag Log tag (component name)
 * @param esp_error ESP-IDF error code
 * @param message Optional additional message
 */
inline void esp_log_error(const char* tag, esp_err_t esp_error, const char* message = nullptr) {
    if (message) {
        ESP_LOGE(tag, "%s: %s (0x%x)", message, esp_err_to_name(esp_error), esp_error);
    } else {
        ESP_LOGE(tag, "%s (0x%x)", esp_err_to_name(esp_error), esp_error);
    }
}

#endif // ESP_PLATFORM

} // namespace alloy::core
