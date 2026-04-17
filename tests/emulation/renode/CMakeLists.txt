option(ALLOY_ENABLE_RENODE_TESTS "Enable Renode-backed emulation scenarios" OFF)
set(ALLOY_RENODE_PYTHON_EXECUTABLE
    ""
    CACHE FILEPATH
    "Python interpreter used for Renode Robot scenarios. Leave empty to auto-select an interpreter with the robot module."
)

include(${CMAKE_CURRENT_SOURCE_DIR}/common/renode_test_helpers.cmake)

if(ALLOY_ENABLE_RENODE_TESTS)
    set(ALLOY_RENODE_APP_ROOT_HINTS)

    if(CMAKE_HOST_APPLE)
        list(APPEND ALLOY_RENODE_APP_ROOT_HINTS
            "/Applications/Renode.app/Contents/MacOS"
        )

        file(GLOB ALLOY_RENODE_VOLUME_APPS
            LIST_DIRECTORIES true
            "/Volumes/Renode*/Renode.app"
        )
        foreach(ALLOY_RENODE_VOLUME_APP IN LISTS ALLOY_RENODE_VOLUME_APPS)
            list(APPEND ALLOY_RENODE_APP_ROOT_HINTS
                "${ALLOY_RENODE_VOLUME_APP}/Contents/MacOS"
            )
        endforeach()
    endif()

    if(NOT ALLOY_RENODE_EXECUTABLE AND NOT ALLOY_RENODE_TEST_EXECUTABLE)
        foreach(ALLOY_RENODE_APP_ROOT IN LISTS ALLOY_RENODE_APP_ROOT_HINTS)
            if(EXISTS "${ALLOY_RENODE_APP_ROOT}/renode" AND EXISTS "${ALLOY_RENODE_APP_ROOT}/renode-test")
                set(ALLOY_RENODE_EXECUTABLE "${ALLOY_RENODE_APP_ROOT}/renode")
                set(ALLOY_RENODE_TEST_EXECUTABLE "${ALLOY_RENODE_APP_ROOT}/renode-test")
                break()
            endif()
        endforeach()
    endif()

    if(NOT ALLOY_RENODE_EXECUTABLE)
        find_program(ALLOY_RENODE_EXECUTABLE
            NAMES renode
            HINTS ${ALLOY_RENODE_APP_ROOT_HINTS}
        )
    endif()
    if(NOT ALLOY_RENODE_TEST_EXECUTABLE)
        find_program(ALLOY_RENODE_TEST_EXECUTABLE
            NAMES renode-test
            HINTS ${ALLOY_RENODE_APP_ROOT_HINTS}
        )
    endif()
endif()

if(ALLOY_ENABLE_RENODE_TESTS)
    if(ALLOY_RENODE_EXECUTABLE)
        message(STATUS "Renode executable found at ${ALLOY_RENODE_EXECUTABLE}")
    else()
        message(WARNING "ALLOY_ENABLE_RENODE_TESTS=ON but no renode executable was found in PATH.")
    endif()

    if(ALLOY_RENODE_TEST_EXECUTABLE)
        message(STATUS "renode-test executable found at ${ALLOY_RENODE_TEST_EXECUTABLE}")
    else()
        message(WARNING "ALLOY_ENABLE_RENODE_TESTS=ON but no renode-test executable was found in PATH.")
    endif()
else()
    message(STATUS "Renode validation scaffold present; set ALLOY_ENABLE_RENODE_TESTS=ON to enable emulation scenarios.")
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
