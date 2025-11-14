/**
 * @file syscalls.cpp
 * @brief Minimal system calls for newlib
 *
 * Provides stubs for newlib system calls. These are required for
 * linking but not fully implemented in bare-metal environment.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

extern "C" {

// Increase program data space (not implemented)
void* _sbrk(int incr) {
    extern char _end;  // Defined by linker
    static char* heap_end = nullptr;
    char* prev_heap_end;

    if (heap_end == nullptr) {
        heap_end = &_end;
    }

    prev_heap_end = heap_end;
    heap_end += incr;

    return prev_heap_end;
}

// Close file (not implemented)
int _close(int file) {
    (void)file;
    return -1;
}

// Status of file (not implemented)
int _fstat(int file, struct stat* st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

// Query whether output stream is a terminal (not implemented)
int _isatty(int file) {
    (void)file;
    return 1;
}

// Set position in file (not implemented)
int _lseek(int file, int ptr, int dir) {
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

// Read from file (not implemented)
int _read(int file, char* ptr, int len) {
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

// Write to file (not implemented - could be redirected to UART)
int _write(int file, char* ptr, int len) {
    (void)file;
    (void)ptr;
    return len;  // Pretend we wrote everything
}

// Exit (not implemented - infinite loop)
void _exit(int status) {
    (void)status;
    while (1) {
        // Hang forever
    }
}

// Kill process (not implemented)
int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

// Get process ID (not implemented)
int _getpid() {
    return 1;
}

} // extern "C"
