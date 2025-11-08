#pragma once

#include <cstdio>
#include <cstring>

#include "../sink.hpp"

// Platform detection for terminal features
#ifdef _WIN32
    #include <io.h>
    #define ISATTY _isatty
    #define FILENO _fileno
#else
    #include <unistd.h>
    #define ISATTY isatty
    #define FILENO fileno
#endif

namespace alloy {
namespace logger {

/**
 * Console output sink with ANSI color support
 *
 * Writes log messages to stdout/stderr with optional color coding.
 * Automatically detects if output is a TTY and disables colors if piped.
 *
 * Color scheme:
 * - TRACE: Gray
 * - DEBUG: Cyan
 * - INFO:  Green
 * - WARN:  Yellow
 * - ERROR: Red (to stderr)
 *
 * Usage (Host only):
 *   ConsoleSink console(true);  // Enable colors
 *   Logger::add_sink(&console);
 */
class ConsoleSink : public Sink {
   public:
    /**
     * Construct console sink
     *
     * @param enable_colors Enable ANSI color codes (auto-disabled if not TTY)
     * @param use_stderr Send ERROR logs to stderr, others to stdout
     */
    explicit ConsoleSink(bool enable_colors = true, bool use_stderr = true)
        : enable_colors_(enable_colors && is_terminal()),
          use_stderr_(use_stderr) {}

    /**
     * Write log message to console
     *
     * @param data Log message
     * @param length Message length
     */
    void write(const char* data, size_t length) override {
        // Determine output stream based on log level
        FILE* stream = stdout;

        if (use_stderr_ && is_error_message(data)) {
            stream = stderr;
        }

        // Apply color if enabled and not already colored
        if (enable_colors_ && !has_ansi_codes(data)) {
            const char* color = get_color_for_message(data);
            fprintf(stream, "%s%.*s%s", color, static_cast<int>(length), data, ANSI_RESET);
        } else {
            fwrite(data, 1, length, stream);
        }

        // Ensure newline (message should already have it)
        // Just flush to ensure output appears immediately
        fflush(stream);
    }

    /**
     * Flush console buffers
     */
    void flush() override {
        fflush(stdout);
        fflush(stderr);
    }

    /**
     * Check if console is available
     */
    bool is_ready() const override {
        return true;  // Console always available
    }

    /**
     * Enable or disable colors dynamically
     */
    void set_colors_enabled(bool enabled) { enable_colors_ = enabled && is_terminal(); }

   private:
    // ANSI color codes
    static constexpr const char* ANSI_RESET = "\033[0m";
    static constexpr const char* ANSI_GRAY = "\033[90m";
    static constexpr const char* ANSI_CYAN = "\033[36m";
    static constexpr const char* ANSI_GREEN = "\033[32m";
    static constexpr const char* ANSI_YELLOW = "\033[33m";
    static constexpr const char* ANSI_RED = "\033[31m";

    /**
     * Check if stdout is a terminal (TTY)
     */
    static bool is_terminal() { return ISATTY(FILENO(stdout)) != 0; }

    /**
     * Check if message already contains ANSI codes
     */
    static bool has_ansi_codes(const char* data) { return strstr(data, "\033[") != nullptr; }

    /**
     * Check if message is an error
     */
    static bool is_error_message(const char* data) { return strstr(data, "ERROR") != nullptr; }

    /**
     * Get color code based on message content
     */
    const char* get_color_for_message(const char* data) const {
        if (strstr(data, "TRACE") != nullptr)
            return ANSI_GRAY;
        if (strstr(data, "DEBUG") != nullptr)
            return ANSI_CYAN;
        if (strstr(data, "INFO") != nullptr)
            return ANSI_GREEN;
        if (strstr(data, "WARN") != nullptr)
            return ANSI_YELLOW;
        if (strstr(data, "ERROR") != nullptr)
            return ANSI_RED;
        return "";  // No color
    }

    bool enable_colors_;
    bool use_stderr_;
};

/**
 * Simple console sink without color support
 *
 * Lighter version that just writes to stdout.
 * Use this if you don't need colors or stderr routing.
 */
class SimpleConsoleSink : public Sink {
   public:
    void write(const char* data, size_t length) override {
        fwrite(data, 1, length, stdout);
        fflush(stdout);
    }

    void flush() override { fflush(stdout); }
};

}  // namespace logger
}  // namespace alloy
