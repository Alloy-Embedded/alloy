# Resolve the alloy runtime root from this file's location so the helper include
# works whether alloy is the top-level project or consumed via add_subdirectory().
# Layout: drivers/protocol/modbus/tests/CMakeLists.txt -> alloy/ is four levels up.
get_filename_component(_alloy_runtime_root "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
include(${_alloy_runtime_root}/tests/alloy_test_helpers.cmake)

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
