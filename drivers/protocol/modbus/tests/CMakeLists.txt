include(${CMAKE_SOURCE_DIR}/tests/alloy_test_helpers.cmake)

alloy_add_catch2_test(test_modbus_pdu
    SOURCES test_pdu.cpp
    LIBS alloy::modbus
    LABELS unit modbus
)

alloy_add_catch2_test(test_modbus_rtu_frame
    SOURCES test_rtu_frame.cpp
    LIBS alloy::modbus
    LABELS unit modbus
)
