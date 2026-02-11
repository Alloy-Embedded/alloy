# MicroCore API Tier Examples
#
# Demonstrates the three-tier API system:
# - Simple tier: factory methods (gpio_simple.hpp, uart_simple.hpp, ...)
# - Fluent tier: builder pattern (gpio_fluent.hpp, uart_fluent.hpp, ...)
# - Expert tier: full control (gpio_expert.hpp, uart_expert.hpp, ...)

set(API_TIER_TARGETS
    simple_gpio_blink
    simple_gpio_button
    fluent_gpio_blink
    expert_gpio_blink
    simple_uart_echo
    fluent_uart_config
)

# expert_uart_lowpower is STM32G0-specific.
if(MICROCORE_PLATFORM STREQUAL "stm32g0")
    list(APPEND API_TIER_TARGETS expert_uart_lowpower)
endif()

set(BOARD_SOURCES)

if(EXISTS "${CMAKE_SOURCE_DIR}/boards/${MICROCORE_BOARD}/board.cpp")
    list(APPEND BOARD_SOURCES "${CMAKE_SOURCE_DIR}/boards/${MICROCORE_BOARD}/board.cpp")
endif()

if(CMAKE_CROSSCOMPILING AND EXISTS "${CMAKE_SOURCE_DIR}/boards/${MICROCORE_BOARD}/syscalls.cpp")
    list(APPEND BOARD_SOURCES "${CMAKE_SOURCE_DIR}/boards/${MICROCORE_BOARD}/syscalls.cpp")
endif()

if(CMAKE_CROSSCOMPILING AND DEFINED STARTUP_SOURCE)
    list(APPEND BOARD_SOURCES ${STARTUP_SOURCE})
endif()

function(configure_api_tier_target target_name)
    add_executable(${target_name}
        ${target_name}.cpp
        ${BOARD_SOURCES}
    )

    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/boards
    )

    if(TARGET microcore::hal)
        target_link_libraries(${target_name} PRIVATE
            microcore::hal
        )
    elseif(DEFINED MICROCORE_HAL_LIBRARY)
        target_link_libraries(${target_name} PRIVATE
            ${MICROCORE_HAL_LIBRARY}
        )
    elseif(DEFINED ALLOY_HAL_LIBRARY)
        # Legacy compatibility
        target_link_libraries(${target_name} PRIVATE
            ${ALLOY_HAL_LIBRARY}
        )
    endif()

    target_compile_options(${target_name} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<CONFIG:Debug>:-O0 -g3>
        $<$<CONFIG:Release>:-O2 -g>
        $<$<CONFIG:MinSizeRel>:-Os -g>
    )

    if(CMAKE_CROSSCOMPILING)
        target_compile_options(${target_name} PRIVATE
            -ffunction-sections
            -fdata-sections
            -fno-exceptions
            -fno-rtti
            -fno-threadsafe-statics
        )

        target_link_options(${target_name} PRIVATE
            -Wl,--gc-sections
            -Wl,--print-memory-usage
            --specs=nano.specs
            --specs=nosys.specs
        )

        if(DEFINED LINKER_SCRIPT AND EXISTS "${LINKER_SCRIPT}")
            target_link_options(${target_name} PRIVATE
                -T${LINKER_SCRIPT}
            )
        endif()

        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${target_name}> ${target_name}.hex
            COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target_name}> ${target_name}.bin
            COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target_name}>
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating HEX and BIN files for ${target_name}"
        )
    endif()
endfunction()

foreach(target_name ${API_TIER_TARGETS})
    configure_api_tier_target(${target_name})
endforeach()
