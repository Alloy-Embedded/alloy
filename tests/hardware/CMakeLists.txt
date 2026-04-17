message(STATUS "Hardware validation uses representative board spot-check runbooks under tests/hardware.")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/common/CMakeLists.txt)
    add_subdirectory(common)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/same70/CMakeLists.txt)
    add_subdirectory(same70)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/stm32g0/CMakeLists.txt)
    add_subdirectory(stm32g0)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4/CMakeLists.txt)
    add_subdirectory(stm32f4)
endif()
