set(ALLOY_ELF_SAME70_BOARD_MATCH FALSE)
if(ALLOY_BOARD STREQUAL "same70_xplained" OR ALLOY_BOARD STREQUAL "same70_xpld")
    set(ALLOY_ELF_SAME70_BOARD_MATCH TRUE)
endif()

if(NOT ALLOY_ELF_SAME70_BOARD_MATCH)
    message(STATUS "ELF SAME70 scenario skipped: select ALLOY_BOARD=same70_xplained (or same70_xpld).")
    return()
endif()

alloy_add_elf_startup_smoke(same70_elf_startup_smoke
    OUTPUT_NAME
        same70_elf_startup_smoke
    BOARD_DIR
        ${CMAKE_SOURCE_DIR}/boards/same70_xplained
    FIRMWARE_SOURCE
        ${CMAKE_SOURCE_DIR}/tests/firmware/same70/boot_smoke.cpp
    LINKER_SCRIPT
        ${CMAKE_SOURCE_DIR}/boards/same70_xplained/ATSAME70Q21.ld
    TEST_NAME
        elf.same70.startup_inspection
    TEST_LABEL
        same70
)
