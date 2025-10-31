# Blinky Example for Blue Pill (STM32F103C8T6)

cmake_minimum_required(VERSION 3.25)

# This example requires the BluePill board
if(NOT ALLOY_BOARD STREQUAL "bluepill")
    message(WARNING "blinky_bluepill requires ALLOY_BOARD=bluepill")
    return()
endif()

# Define the executable
add_executable(blinky_bluepill
    main.cpp
)

# Link with STM32F1 HAL
target_link_libraries(blinky_bluepill PRIVATE
    alloy::hal::stm32f1
)

# Use generated startup code if available
if(ALLOY_CODEGEN_AVAILABLE AND ALLOY_GENERATED_SOURCES)
    target_sources(blinky_bluepill PRIVATE
        ${ALLOY_GENERATED_SOURCES}
    )
endif()

# Set linker script (to be created)
# set_target_properties(blinky_bluepill PROPERTIES
#     LINK_FLAGS "-T${CMAKE_CURRENT_SOURCE_DIR}/STM32F103C8.ld"
# )

message(STATUS "Example: blinky_bluepill configured")
