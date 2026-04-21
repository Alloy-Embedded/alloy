if(ALLOY_BOARD STREQUAL "nucleo_f401re")
    message(STATUS "Hardware STM32F4 runbook selected: tests/hardware/stm32f4/README.md")

    set(ALLOY_STM32F4_HARDWARE_TARGETS
        blink
        uart_logger
        watchdog_probe
        analog_probe
        rtc_probe
        timer_pwm_probe
        dma_probe
    )

    set(ALLOY_STM32F4_HARDWARE_AVAILABLE_TARGETS)
    foreach(_target IN LISTS ALLOY_STM32F4_HARDWARE_TARGETS)
        if(TARGET ${_target})
            list(APPEND ALLOY_STM32F4_HARDWARE_AVAILABLE_TARGETS ${_target})
        endif()
    endforeach()

    add_custom_target(stm32f4_hardware_validation_bundle
        DEPENDS ${ALLOY_STM32F4_HARDWARE_AVAILABLE_TARGETS}
        COMMENT "Building STM32F4 hardware validation firmware bundle"
    )

    message(STATUS "Hardware STM32F4 firmware bundle target: stm32f4_hardware_validation_bundle")
    message(STATUS "  Includes: ${ALLOY_STM32F4_HARDWARE_AVAILABLE_TARGETS}")
endif()
