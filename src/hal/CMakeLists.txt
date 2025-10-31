# Alloy Hardware Abstraction Layer (HAL)
# Automatic vendor/family detection and configuration

# Function to detect vendor and family from MCU name
function(detect_mcu_info MCU_NAME OUT_VENDOR OUT_FAMILY OUT_MCU_LOWER)
    string(TOLOWER "${MCU_NAME}" mcu_lower)

    # STM32 Detection
    if(mcu_lower MATCHES "^stm32")
        set(${OUT_VENDOR} "st" PARENT_SCOPE)
        # Extract family: STM32F103 -> stm32f1, STM32L476 -> stm32l4
        string(REGEX MATCH "stm32([a-z][0-9])" _ "${mcu_lower}")
        set(${OUT_FAMILY} "stm32${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${OUT_MCU_LOWER} "${mcu_lower}" PARENT_SCOPE)
        return()
    endif()

    # Nordic nRF Detection
    if(mcu_lower MATCHES "^nrf")
        set(${OUT_VENDOR} "nordic" PARENT_SCOPE)
        # Extract family: nRF52840 -> nrf52
        string(REGEX MATCH "nrf([0-9]+)" _ "${mcu_lower}")
        set(${OUT_FAMILY} "nrf${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${OUT_MCU_LOWER} "${mcu_lower}" PARENT_SCOPE)
        return()
    endif()

    # ESP32 Detection
    if(mcu_lower MATCHES "^esp32")
        set(${OUT_VENDOR} "espressif" PARENT_SCOPE)
        # Extract family: ESP32, ESP32-C3, ESP32-S3 -> esp32, esp32c3, esp32s3
        string(REPLACE "-" "" family_clean "${mcu_lower}")
        string(REGEX MATCH "esp32[a-z]*[0-9]*" family "${family_clean}")
        set(${OUT_FAMILY} "${family}" PARENT_SCOPE)
        set(${OUT_MCU_LOWER} "${family_clean}" PARENT_SCOPE)
        return()
    endif()

    # Raspberry Pi RP2040/RP2350
    if(mcu_lower MATCHES "^rp2[03]")
        set(${OUT_VENDOR} "raspberrypi" PARENT_SCOPE)
        string(REGEX MATCH "rp2[0-9]+" family "${mcu_lower}")
        set(${OUT_FAMILY} "${family}" PARENT_SCOPE)
        set(${OUT_MCU_LOWER} "${mcu_lower}" PARENT_SCOPE)
        return()
    endif()

    # Renesas RL78
    if(mcu_lower MATCHES "^rl78")
        set(${OUT_VENDOR} "renesas" PARENT_SCOPE)
        set(${OUT_FAMILY} "rl78" PARENT_SCOPE)
        set(${OUT_MCU_LOWER} "${mcu_lower}" PARENT_SCOPE)
        return()
    endif()

    # Default: Unknown
    message(FATAL_ERROR "Unknown MCU vendor/family for: ${MCU_NAME}\nSupported: STM32, nRF, ESP32, RP2040, RL78")
endfunction()

# Platform-agnostic interfaces (always included)
add_subdirectory(interface)

# Handle host platform specially (for testing)
if(ALLOY_PLATFORM STREQUAL "host")
    add_subdirectory(host)
    message(STATUS "HAL configured for platform: host")
    return()
endif()

# Main HAL Configuration for embedded platforms
if(NOT DEFINED ALLOY_MCU)
    message(FATAL_ERROR "ALLOY_MCU must be defined (e.g., set(ALLOY_MCU \"STM32F103C8\"))")
endif()

# Detect MCU information
detect_mcu_info(${ALLOY_MCU} MCU_VENDOR MCU_FAMILY MCU_LOWER)

message(STATUS "=== Alloy HAL Configuration ===")
message(STATUS "  MCU:    ${ALLOY_MCU}")
message(STATUS "  Vendor: ${MCU_VENDOR}")
message(STATUS "  Family: ${MCU_FAMILY}")
message(STATUS "  Lower:  ${MCU_LOWER}")

# Set paths
set(ALLOY_HAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${MCU_VENDOR}/${MCU_FAMILY}")
set(ALLOY_GENERATED_DIR "${CMAKE_SOURCE_DIR}/src/generated/${MCU_VENDOR}/${MCU_FAMILY}/${MCU_LOWER}")

message(STATUS "  HAL dir:       ${ALLOY_HAL_DIR}")
message(STATUS "  Generated dir: ${ALLOY_GENERATED_DIR}")

# Check if HAL implementation exists
if(NOT EXISTS "${ALLOY_HAL_DIR}")
    message(WARNING "")
    message(WARNING "HAL implementation not found for ${MCU_VENDOR}/${MCU_FAMILY}")
    message(WARNING "  Expected: ${ALLOY_HAL_DIR}")
    message(WARNING "  Using generated code only (no HAL abstraction)")
    message(WARNING "")
    set(HAL_EXISTS FALSE)
else()
    set(HAL_EXISTS TRUE)
    message(STATUS "  ✓ HAL implementation found")
endif()

# Check if generated code exists
if(NOT EXISTS "${ALLOY_GENERATED_DIR}/peripherals.hpp")
    message(FATAL_ERROR "Generated code not found for ${MCU_LOWER}\n  Expected: ${ALLOY_GENERATED_DIR}/peripherals.hpp\n  Run: cd tools/codegen && python3 update_all_vendors.py --vendor ${MCU_VENDOR}")
endif()
message(STATUS "  ✓ Generated code found")

# Create HAL library
add_library(alloy_hal STATIC)

# Add interface headers (always included)
target_include_directories(alloy_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add generated code
target_include_directories(alloy_hal PUBLIC
    ${ALLOY_GENERATED_DIR}  # For <peripherals.hpp>
)

target_sources(alloy_hal PRIVATE
    ${ALLOY_GENERATED_DIR}/startup.cpp
)

# Define the generated namespace for HAL code to use
# Format: alloy::generated::<mcu_lower>
# Example: alloy::generated::stm32f103c8
target_compile_definitions(alloy_hal PUBLIC
    ALLOY_GENERATED_NAMESPACE=alloy::generated::${MCU_LOWER}
)

# Add HAL implementation if it exists
if(HAL_EXISTS)
    # Check if HAL has CMakeLists.txt
    if(EXISTS "${ALLOY_HAL_DIR}/CMakeLists.txt")
        add_subdirectory(${MCU_VENDOR}/${MCU_FAMILY})
    else()
        # Fallback: add all source files from HAL directory
        file(GLOB HAL_SOURCES
            "${ALLOY_HAL_DIR}/*.cpp"
        )
        if(HAL_SOURCES)
            target_sources(alloy_hal PRIVATE ${HAL_SOURCES})
            list(LENGTH HAL_SOURCES source_count)
            message(STATUS "  ✓ Added ${source_count} HAL source files")
        endif()
    endif()
endif()

# Add core library dependency
target_link_libraries(alloy_hal PUBLIC alloy_core)

message(STATUS "=== HAL Configuration Complete ===")
message(STATUS "")
