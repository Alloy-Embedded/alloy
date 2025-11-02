/// Alloy RTOS - Platform-Agnostic Critical Sections
///
/// Provides interrupt disable/enable primitives for all platforms.
/// On embedded: Uses CPSID/CPSIE (ARM) or RSIL (Xtensa)
/// On host: Uses std::mutex for thread synchronization

#ifndef ALLOY_RTOS_PLATFORM_CRITICAL_SECTION_HPP
#define ALLOY_RTOS_PLATFORM_CRITICAL_SECTION_HPP

#if defined(__x86_64__) || defined(__aarch64__) || defined(_WIN64) || defined(__APPLE__)
    // Host platform - use mutex for critical sections
    #include <mutex>

    namespace alloy::rtos::platform {
        // Global mutex for critical sections on host
        extern std::mutex g_critical_section_mutex;

        inline void disable_interrupts() {
            g_critical_section_mutex.lock();
        }

        inline void enable_interrupts() {
            g_critical_section_mutex.unlock();
        }
    }

#elif defined(__ARM_ARCH)
    // ARM Cortex-M - use CPSID/CPSIE
    namespace alloy::rtos::platform {
        inline void disable_interrupts() {
            __asm volatile("cpsid i" ::: "memory");
        }

        inline void enable_interrupts() {
            __asm volatile("cpsie i" ::: "memory");
        }
    }

#elif defined(ESP32) || defined(ESP_PLATFORM)
    // Xtensa (ESP32) - use RSIL
    namespace alloy::rtos::platform {
        inline void disable_interrupts() {
            __asm volatile("rsil a15, 15" ::: "memory");
        }

        inline void enable_interrupts() {
            __asm volatile("rsil a15, 0" ::: "memory");
        }
    }

#else
    #error "Unsupported platform for critical sections"
#endif

#endif // ALLOY_RTOS_PLATFORM_CRITICAL_SECTION_HPP
