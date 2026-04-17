include_guard(GLOBAL)

include(Catch)

function(alloy_add_catch2_test TARGET_NAME)
    set(options)
    set(one_value_args TEST_PREFIX WORKING_DIRECTORY)
    set(multi_value_args SOURCES LIBS LABELS INCLUDES COMPILE_DEFINITIONS COMPILE_OPTIONS)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "alloy_add_catch2_test(${TARGET_NAME}) requires SOURCES")
    endif()

    add_executable(${TARGET_NAME} ${ARG_SOURCES})
    target_link_libraries(${TARGET_NAME} PRIVATE
        Catch2::Catch2WithMain
        ${ARG_LIBS}
    )
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)
    target_compile_options(${TARGET_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        ${ARG_COMPILE_OPTIONS}
    )
    target_compile_definitions(${TARGET_NAME} PRIVATE ${ARG_COMPILE_DEFINITIONS})
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
        ${ARG_INCLUDES}
        ${ALLOY_DEVICE_INCLUDE_DIRS}
    )
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "tests")

    set(_discover_args)
    if(ARG_TEST_PREFIX)
        list(APPEND _discover_args TEST_PREFIX "${ARG_TEST_PREFIX}")
    endif()
    if(ARG_WORKING_DIRECTORY)
        list(APPEND _discover_args WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}")
    endif()
    if(ARG_LABELS)
        string(JOIN ";" _labels ${ARG_LABELS})
        string(REPLACE ";" "\\;" _labels "${_labels}")
        list(APPEND _discover_args PROPERTIES LABELS "${_labels}")
    endif()

    catch_discover_tests(${TARGET_NAME} ${_discover_args})
endfunction()
