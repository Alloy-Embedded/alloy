if(ALLOY_BOARD STREQUAL "same70_xplained" OR ALLOY_BOARD STREQUAL "same70_xpld")
    message(STATUS "Hardware SAME70 runbook selected: tests/hardware/same70/README.md")

    set(ALLOY_SAME70_HARDWARE_TARGETS
        blink
        uart_logger
        watchdog_probe
        analog_probe
        rtc_probe
        timer_pwm_probe
        i2c_scan
        spi_probe
        dma_probe
        can_probe
    )

    set(ALLOY_SAME70_HARDWARE_AVAILABLE_TARGETS)
    foreach(_target IN LISTS ALLOY_SAME70_HARDWARE_TARGETS)
        if(TARGET ${_target})
            list(APPEND ALLOY_SAME70_HARDWARE_AVAILABLE_TARGETS ${_target})
        endif()
    endforeach()

    add_custom_target(same70_hardware_validation_bundle
        DEPENDS ${ALLOY_SAME70_HARDWARE_AVAILABLE_TARGETS}
        COMMENT "Building SAME70 hardware validation firmware bundle"
    )

    message(STATUS "Hardware SAME70 firmware bundle target: same70_hardware_validation_bundle")
    message(STATUS "  Includes: ${ALLOY_SAME70_HARDWARE_AVAILABLE_TARGETS}")
endif()
