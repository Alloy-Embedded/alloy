set(ALLOY_ELF_STM32F4_BOARD_MATCH FALSE)
if(ALLOY_BOARD STREQUAL "nucleo_f401re")
    set(ALLOY_ELF_STM32F4_BOARD_MATCH TRUE)
endif()

if(NOT ALLOY_ELF_STM32F4_BOARD_MATCH)
    message(STATUS "ELF STM32F4 scenario skipped: select ALLOY_BOARD=nucleo_f401re.")
    return()
endif()

alloy_add_elf_startup_smoke(stm32f4_elf_startup_smoke
    OUTPUT_NAME
        stm32f4_elf_startup_smoke
    BOARD_DIR
        ${CMAKE_SOURCE_DIR}/boards/nucleo_f401re
    FIRMWARE_SOURCE
        ${CMAKE_SOURCE_DIR}/tests/firmware/stm32f4/boot_smoke.cpp
    LINKER_SCRIPT
        ${CMAKE_SOURCE_DIR}/boards/nucleo_f401re/STM32F401RET6.ld
    TEST_NAME
        elf.stm32f4.startup_inspection
    TEST_LABEL
        stm32f4
)
