# Alloy Hardware Abstraction Layer (HAL)

# Platform-agnostic interfaces
add_subdirectory(interface)

# Platform-specific implementations
if(ALLOY_PLATFORM STREQUAL "host")
    add_subdirectory(host)
elseif(ALLOY_PLATFORM STREQUAL "arm")
    # Determine which ARM MCU family
    if(ALLOY_ARCH MATCHES "cortex-m3" AND ALLOY_MCU MATCHES "STM32F1")
        add_subdirectory(stm32f1)
    elseif(ALLOY_ARCH MATCHES "cortex-m4" AND ALLOY_MCU MATCHES "STM32F4")
        add_subdirectory(stm32f4)
    elseif(ALLOY_MCU MATCHES "RP2040")
        add_subdirectory(rp2040)
    else()
        message(WARNING "Unknown ARM MCU: ${ALLOY_MCU}")
    endif()
elseif(ALLOY_PLATFORM STREQUAL "rl78")
    add_subdirectory(rl78)
elseif(ALLOY_PLATFORM STREQUAL "esp32")
    add_subdirectory(esp32)
else()
    message(WARNING "Unknown platform: ${ALLOY_PLATFORM}")
endif()

message(STATUS "HAL configured for platform: ${ALLOY_PLATFORM}")
