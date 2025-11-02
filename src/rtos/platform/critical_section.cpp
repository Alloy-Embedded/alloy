/// Alloy RTOS - Critical Section Implementation

#include "critical_section.hpp"

#if defined(__x86_64__) || defined(__aarch64__) || defined(_WIN64) || defined(__APPLE__)
// Host platform - define global mutex
namespace alloy::rtos::platform {
    std::mutex g_critical_section_mutex;
}
#endif
