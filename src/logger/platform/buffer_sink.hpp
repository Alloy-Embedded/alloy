#pragma once

#include "../sink.hpp"
#include <cstring>

namespace alloy {
namespace logger {

/**
 * Memory buffer sink for logger
 *
 * Stores log messages in a fixed-size memory buffer.
 * Useful for testing and deferred transmission.
 *
 * Usage:
 *   BufferSink buffer_sink(1024);  // 1KB buffer
 *   Logger::add_sink(&buffer_sink);
 *   LOG_INFO("Test message");
 *   // Later: read buffer_sink.data()
 */
class BufferSink : public Sink {
public:
    /**
     * Construct buffer sink with static buffer
     *
     * @param buffer Pointer to buffer
     * @param capacity Buffer size in bytes
     */
    BufferSink(char* buffer, size_t capacity)
        : buffer_(buffer), capacity_(capacity), size_(0) {
        if (buffer_ && capacity_ > 0) {
            buffer_[0] = '\0';
        }
    }

    /**
     * Write log message to buffer
     *
     * If buffer is full, message is truncated.
     *
     * @param data Log message
     * @param length Message length
     */
    void write(const char* data, size_t length) override {
        if (buffer_ == nullptr || capacity_ == 0) {
            return;
        }

        // Calculate available space (leave 1 byte for null terminator)
        size_t available = capacity_ - size_ - 1;

        if (available == 0) {
            return;  // Buffer full
        }

        // Copy as much as possible
        size_t to_copy = (length < available) ? length : available;
        memcpy(buffer_ + size_, data, to_copy);
        size_ += to_copy;
        buffer_[size_] = '\0';
    }

    /**
     * Get buffer contents
     *
     * @return Pointer to buffer (null-terminated)
     */
    const char* data() const {
        return buffer_;
    }

    /**
     * Get current buffer size
     *
     * @return Number of bytes written to buffer
     */
    size_t size() const {
        return size_;
    }

    /**
     * Get buffer capacity
     *
     * @return Maximum buffer size
     */
    size_t capacity() const {
        return capacity_;
    }

    /**
     * Clear buffer contents
     */
    void clear() {
        size_ = 0;
        if (buffer_ && capacity_ > 0) {
            buffer_[0] = '\0';
        }
    }

    /**
     * Check if buffer is full
     *
     * @return true if no more space available
     */
    bool is_full() const {
        return size_ >= capacity_ - 1;
    }

private:
    char* buffer_;
    size_t capacity_;
    size_t size_;
};

} // namespace logger
} // namespace alloy
