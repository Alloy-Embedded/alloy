/**
 * @file syscalls.cpp
 * @brief Minimal syscall stubs for newlib-nano
 *
 * Provides minimal implementations of syscalls required by newlib-nano.
 * These are no-op implementations since we don't use standard I/O.
 */

#include <sys/stat.h>
#include <errno.h>

extern "C" {

/**
 * @brief Write to a file descriptor (stub)
 * @return -1 (not supported)
 */
int _write(int, char*, int) {
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Close a file descriptor (stub)
 * @return -1 (not supported)
 */
int _close(int) {
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Get file status (stub)
 * @return -1 (not supported)
 */
int _fstat(int, struct stat*) {
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Check if file descriptor is a terminal (stub)
 * @return 0 (not a terminal)
 */
int _isatty(int) {
    return 0;
}

/**
 * @brief Seek to position in file (stub)
 * @return -1 (not supported)
 */
int _lseek(int, int, int) {
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Read from file descriptor (stub)
 * @return -1 (not supported)
 */
int _read(int, char*, int) {
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Increase program data space (stub for heap)
 * @return -1 (not supported, no heap)
 */
void* _sbrk(int) {
    errno = ENOMEM;
    return (void*)-1;
}

/**
 * @brief Get process ID (stub)
 * @return 1 (fixed PID)
 */
int _getpid() {
    return 1;
}

/**
 * @brief Send signal to process (stub)
 * @return -1 (not supported)
 */
int _kill(int, int) {
    errno = ENOSYS;
    return -1;
}

} // extern "C"
