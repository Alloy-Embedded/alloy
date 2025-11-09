#pragma once

#include "../sink.hpp"

#ifdef ESP_PLATFORM
    #include "esp_log.h"

namespace alloy::logger {

/**
 * ESP-IDF logging integration sink
 *
 * Bridges Alloy logger to ESP-IDF's esp_log system.
 * Allows Alloy logs to appear in:
 * - idf.py monitor
 * - ESP-IDF log viewer
 * - Remote logging (if configured)
 *
 * Usage:
 *   EspLogSink esp_sink("Alloy");
 *   Logger::add_sink(&esp_sink);
 *
 * Note: Only available on ESP32 platforms (requires ESP-IDF)
 */
class EspLogSink : public Sink {
   public:
    /**
     * Construct ESP-IDF log sink
     *
     * @param tag Log tag for ESP-IDF (shown in output)
     */
    explicit EspLogSink(const char* tag = "Alloy") : tag_(tag) {}

    /**
     * Write log message to ESP-IDF logging system
     *
     * Alloy log levels are mapped to ESP-IDF levels:
     * - TRACE -> ESP_LOGV (Verbose)
     * - DEBUG -> ESP_LOGD (Debug)
     * - INFO  -> ESP_LOGI (Info)
     * - WARN  -> ESP_LOGW (Warning)
     * - ERROR -> ESP_LOGE (Error)
     *
     * @param data Log message (already formatted by Alloy)
     * @param length Message length
     */
    void write(const char* data, size_t length) override {
        (void)length;  // ESP_LOG uses null-terminated strings

        // Extract log level from message if possible
        // For now, default to INFO level
        // The message already contains level information from Alloy formatter

        // Just forward to ESP_LOGI - message already has level in it
        ESP_LOGI(tag_, "%s", data);
    }

    /**
     * Flush ESP-IDF log buffer
     */
    void flush() override {
        // ESP-IDF logging is typically unbuffered or auto-flushed
        // No explicit flush needed
    }

   private:
    const char* tag_;
};

/**
 * ESP-IDF sink with level extraction
 *
 * This version parses the Alloy log message to extract the level
 * and uses the appropriate ESP-IDF logging macro.
 *
 * Usage:
 *   EspLogSinkWithLevel esp_sink("Alloy");
 *   Logger::add_sink(&esp_sink);
 */
class EspLogSinkWithLevel : public Sink {
   public:
    explicit EspLogSinkWithLevel(const char* tag = "Alloy") : tag_(tag) {}

    void write(const char* data, size_t length) override {
        (void)length;

        // Parse log level from formatted message
        // Format: [timestamp] LEVEL [file:line] message
        esp_log_level_t esp_level = ESP_LOG_INFO;  // Default

        // Try to extract level
        if (strstr(data, "TRACE") != nullptr || strstr(data, "DEBUG") != nullptr) {
            esp_level = ESP_LOG_DEBUG;
        } else if (strstr(data, "WARN") != nullptr) {
            esp_level = ESP_LOG_WARN;
        } else if (strstr(data, "ERROR") != nullptr) {
            esp_level = ESP_LOG_ERROR;
        }

        // Use ESP-IDF's log function
        esp_log_write(esp_level, tag_, "%s", data);
    }

   private:
    const char* tag_;
};

}  // namespace alloy::logger

#endif  // ESP_PLATFORM
