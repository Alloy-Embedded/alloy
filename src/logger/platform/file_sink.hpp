#pragma once

#include <cstdio>
#include <cstring>

#include "../sink.hpp"

namespace alloy {
namespace logger {

/**
 * File output sink for logger
 *
 * Writes log messages to a file on the filesystem.
 * Supports:
 * - Host platforms (POSIX)
 * - ESP32 with SPIFFS/FAT
 * - Embedded with SD card support
 *
 * Usage:
 *   FileSink file_sink("/logs/app.log");
 *   Logger::add_sink(&file_sink);
 *
 * Note: File must be writable. No automatic rotation (see RotatingFileSink).
 */
class FileSink : public Sink {
   public:
    /**
     * Construct file sink
     *
     * @param filename Path to log file
     * @param append If true, append to existing file. If false, truncate.
     */
    explicit FileSink(const char* filename, bool append = true)
        : filename_(filename),
          file_(nullptr),
          is_open_(false) {
        open(append);
    }

    /**
     * Destructor - closes file
     */
    ~FileSink() { close(); }

    /**
     * Write log message to file
     *
     * @param data Log message
     * @param length Message length
     */
    void write(const char* data, size_t length) override {
        if (!is_open_ || file_ == nullptr) {
            return;
        }

        // Write to file
        size_t written = fwrite(data, 1, length, file_);
        (void)written;  // Ignore write errors for now
    }

    /**
     * Flush file buffer to disk
     */
    void flush() override {
        if (is_open_ && file_ != nullptr) {
            fflush(file_);
        }
    }

    /**
     * Check if file is open and ready
     */
    bool is_ready() const override { return is_open_ && file_ != nullptr; }

    /**
     * Close the file
     */
    void close() {
        if (file_ != nullptr) {
            fflush(file_);
            fclose(file_);
            file_ = nullptr;
            is_open_ = false;
        }
    }

    /**
     * Reopen the file
     *
     * @param append If true, append. If false, truncate.
     * @return true if successful
     */
    bool reopen(bool append = true) {
        close();
        return open(append);
    }

    /**
     * Get current file size
     *
     * @return File size in bytes, or 0 if file not open
     */
    size_t size() const {
        if (!is_open_ || file_ == nullptr) {
            return 0;
        }

        long current_pos = ftell(file_);
        if (current_pos < 0) {
            return 0;
        }

        // Seek to end to get size
        fseek(file_, 0, SEEK_END);
        long file_size = ftell(file_);

        // Restore position
        fseek(file_, current_pos, SEEK_SET);

        return (file_size >= 0) ? static_cast<size_t>(file_size) : 0;
    }

    /**
     * Get filename
     */
    const char* filename() const { return filename_; }

   private:
    /**
     * Open the file
     */
    bool open(bool append) {
        const char* mode = append ? "a" : "w";
        file_ = fopen(filename_, mode);
        is_open_ = (file_ != nullptr);
        return is_open_;
    }

    const char* filename_;
    FILE* file_;
    bool is_open_;
};

/**
 * Rotating file sink
 *
 * Automatically rotates log files when they reach a maximum size.
 * Creates backup files: app.log, app.log.1, app.log.2, etc.
 *
 * Usage:
 *   RotatingFileSink sink("/logs/app.log", 1024*1024, 5);  // 1MB, 5 backups
 *   Logger::add_sink(&sink);
 */
class RotatingFileSink : public Sink {
   public:
    /**
     * Construct rotating file sink
     *
     * @param filename Base log file path
     * @param max_size Maximum file size before rotation (bytes)
     * @param max_files Maximum number of backup files
     */
    RotatingFileSink(const char* filename, size_t max_size, size_t max_files = 3)
        : base_filename_(filename),
          max_size_(max_size),
          max_files_(max_files),
          current_size_(0),
          file_(nullptr) {
        open_current_file();
    }

    ~RotatingFileSink() {
        if (file_ != nullptr) {
            fclose(file_);
        }
    }

    void write(const char* data, size_t length) override {
        if (file_ == nullptr) {
            return;
        }

        // Check if rotation needed
        if (current_size_ + length > max_size_) {
            rotate();
        }

        // Write to file
        size_t written = fwrite(data, 1, length, file_);
        current_size_ += written;
    }

    void flush() override {
        if (file_ != nullptr) {
            fflush(file_);
        }
    }

    bool is_ready() const override { return file_ != nullptr; }

   private:
    void open_current_file() {
        file_ = fopen(base_filename_, "a");
        if (file_ != nullptr) {
            fseek(file_, 0, SEEK_END);
            long pos = ftell(file_);
            current_size_ = (pos >= 0) ? static_cast<size_t>(pos) : 0;
        }
    }

    void rotate() {
        // Close current file
        if (file_ != nullptr) {
            fclose(file_);
            file_ = nullptr;
        }

        // Rotate backup files: app.log.2 -> app.log.3, app.log.1 -> app.log.2, etc.
        char old_name[256];
        char new_name[256];

        for (int i = static_cast<int>(max_files_) - 1; i >= 1; --i) {
            snprintf(old_name, sizeof(old_name), "%s.%d", base_filename_, i);
            snprintf(new_name, sizeof(new_name), "%s.%d", base_filename_, i + 1);
            rename(old_name, new_name);  // Ignore errors
        }

        // Rename current file to .1
        snprintf(new_name, sizeof(new_name), "%s.1", base_filename_);
        rename(base_filename_, new_name);

        // Open new current file
        current_size_ = 0;
        open_current_file();
    }

    const char* base_filename_;
    size_t max_size_;
    size_t max_files_;
    size_t current_size_;
    FILE* file_;
};

}  // namespace logger
}  // namespace alloy
