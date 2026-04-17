set(ALLOY_RENODE_SAME70_BOARD_MATCH FALSE)
if(ALLOY_BOARD STREQUAL "same70_xplained" OR ALLOY_BOARD STREQUAL "same70_xpld")
    set(ALLOY_RENODE_SAME70_BOARD_MATCH TRUE)
endif()

if(NOT ALLOY_RENODE_SAME70_BOARD_MATCH)
    message(STATUS "Renode SAME70 scenario skipped: select ALLOY_BOARD=same70_xplained (or same70_xpld).")
    return()
endif()

if(NOT CMAKE_CROSSCOMPILING)
    message(STATUS "Renode SAME70 scenario skipped: cross toolchain is required.")
    return()
endif()

set(ALLOY_RENODE_SAME70_TARGET same70_renode_boot_smoke)
set(ALLOY_RENODE_SAME70_BOARD_DIR "${CMAKE_SOURCE_DIR}/boards/same70_xplained")
set(ALLOY_RENODE_SAME70_PLATFORM "${CMAKE_CURRENT_BINARY_DIR}/same70_boot_smoke.repl")

set(ALLOY_RENODE_SAME70_FLIPFLOP_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/flipflop.py"
)

alloy_add_renode_boot_smoke(${ALLOY_RENODE_SAME70_TARGET}
    OUTPUT_NAME
        same70_renode_boot_smoke
    BOARD_DIR
        ${ALLOY_RENODE_SAME70_BOARD_DIR}
    FIRMWARE_SOURCE
        ${CMAKE_SOURCE_DIR}/tests/firmware/same70/boot_smoke.cpp
    LINKER_SCRIPT
        ${CMAKE_SOURCE_DIR}/boards/same70_xplained/ATSAME70Q21.ld
    PLATFORM_TEMPLATE
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/same70_boot_smoke.repl.in
    PLATFORM_OUTPUT
        ${ALLOY_RENODE_SAME70_PLATFORM}
    ROBOT_SCRIPT
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/boot_smoke.robot
    TEST_NAME
        renode.same70.boot_smoke
    TEST_LABELS
        emulation
        renode
        same70
    SYMBOL_VARS
        BOOT_STAGE_ADDR=alloy_renode_boot_stage
        BOOT_MARKER_ADDR=alloy_renode_boot_marker
        BOOT_UART_BYTES_ADDR=alloy_renode_uart_bytes_written
    FILE_VARS
        COMMON_BOOT_RESOURCE=${CMAKE_SOURCE_DIR}/tests/emulation/renode/common/robot/boot_assertions.resource
)
