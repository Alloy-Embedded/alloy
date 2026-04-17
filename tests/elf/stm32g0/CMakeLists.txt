set(ALLOY_ELF_STM32G0_BOARD_MATCH FALSE)
if(ALLOY_BOARD STREQUAL "nucleo_g071rb")
    set(ALLOY_ELF_STM32G0_BOARD_MATCH TRUE)
endif()

if(NOT ALLOY_ELF_STM32G0_BOARD_MATCH)
    message(STATUS "ELF STM32G0 scenario skipped: select ALLOY_BOARD=nucleo_g071rb.")
    return()
endif()

alloy_add_elf_startup_smoke(stm32g0_elf_startup_smoke
    OUTPUT_NAME
        stm32g0_elf_startup_smoke
    BOARD_DIR
        ${CMAKE_SOURCE_DIR}/boards/nucleo_g071rb
    FIRMWARE_SOURCE
        ${CMAKE_SOURCE_DIR}/tests/firmware/stm32g0/boot_smoke.cpp
    LINKER_SCRIPT
        ${CMAKE_SOURCE_DIR}/boards/nucleo_g071rb/STM32G071RBT6.ld
    TEST_NAME
        elf.stm32g0.startup_inspection
    TEST_LABEL
        stm32g0
)
