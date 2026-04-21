include_guard(GLOBAL)

function(alloy_add_device_contract_smoke TARGET_NAME)
    add_library(${TARGET_NAME} OBJECT
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_device_import_layer.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_startup_contract.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_system_clock_contract.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_dma_api.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_connector_kernel.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_gpio_api.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_uart_api.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_i2c_api.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_spi_api.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_timer_api.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_pwm_api.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_adc_api.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_dac_api.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_board_led_api.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_board_uart_api.cpp
        ${CMAKE_SOURCE_DIR}/tests/compile_tests/test_board_dma_api.cpp
        ${ALLOY_ACTIVE_STARTUP_SOURCE}
        ${ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE}
    )

    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/boards
        ${ALLOY_DEVICE_INCLUDE_DIRS}
    )
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "tests/compile")
endfunction()
