if(NOT CMAKE_CROSSCOMPILING)
    message(STATUS "ELF validation skipped: cross toolchain is required for startup inspection scenarios.")
    return()
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/common/elf_test_helpers.cmake)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/same70/CMakeLists.txt)
    add_subdirectory(same70)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/stm32g0/CMakeLists.txt)
    add_subdirectory(stm32g0)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4/CMakeLists.txt)
    add_subdirectory(stm32f4)
endif()
