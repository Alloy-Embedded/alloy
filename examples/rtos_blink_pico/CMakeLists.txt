# RTOS Blink Example for Raspberry Pi Pico

add_executable(rtos_blink_pico
    main.cpp
    ../../src/hal/vendors/raspberrypi/rp2040/boot2_w25q080.S
    ../../src/hal/vendors/raspberrypi/rp2040/startup.cpp
    ../../src/rtos/scheduler.cpp
    ../../src/rtos/platform/arm_context_m0.cpp  # M0+ specific context switching
    ../../src/rtos/platform/arm_systick_integration.cpp
    ../../src/hal/rp2040/systick.cpp
)

target_include_directories(rtos_blink_pico PRIVATE
    ${CMAKE_SOURCE_DIR}/boards
    ${CMAKE_SOURCE_DIR}/src
)

# Apply board-specific settings
alloy_target_board_settings(rtos_blink_pico)
