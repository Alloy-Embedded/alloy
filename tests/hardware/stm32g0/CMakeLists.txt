if(ALLOY_BOARD STREQUAL "nucleo_g071rb" OR ALLOY_BOARD STREQUAL "nucleo_g0b1re")
    message(STATUS "Hardware STM32G0 runbook selected: tests/hardware/stm32g0/README.md")

    set(ALLOY_STM32G0_HARDWARE_TARGETS
        blink
        time_probe
        uart_logger
        watchdog_probe
        analog_probe
        rtc_probe
        timer_pwm_probe
    )

    set(ALLOY_STM32G0_HARDWARE_AVAILABLE_TARGETS)
    foreach(_target IN LISTS ALLOY_STM32G0_HARDWARE_TARGETS)
        if(TARGET ${_target})
            list(APPEND ALLOY_STM32G0_HARDWARE_AVAILABLE_TARGETS ${_target})
        endif()
    endforeach()

    add_custom_target(stm32g0_hardware_validation_bundle
        DEPENDS ${ALLOY_STM32G0_HARDWARE_AVAILABLE_TARGETS}
        COMMENT "Building STM32G0 hardware validation firmware bundle"
    )

    message(STATUS "Hardware STM32G0 firmware bundle target: stm32g0_hardware_validation_bundle")
    message(STATUS "  Includes: ${ALLOY_STM32G0_HARDWARE_AVAILABLE_TARGETS}")
endif()
