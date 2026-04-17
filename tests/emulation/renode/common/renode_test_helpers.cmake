include_guard(GLOBAL)

function(alloy_resolve_renode_python OUT_EXECUTABLE OUT_HAS_ROBOT OUT_DISCOVERY_MESSAGE)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    set(_candidate_executables)
    if(ALLOY_RENODE_PYTHON_EXECUTABLE)
        list(APPEND _candidate_executables "${ALLOY_RENODE_PYTHON_EXECUTABLE}")
    endif()
    if(Python3_EXECUTABLE)
        list(APPEND _candidate_executables "${Python3_EXECUTABLE}")
    endif()

    find_program(_alloy_python3_from_path NAMES python3)
    if(_alloy_python3_from_path)
        list(APPEND _candidate_executables "${_alloy_python3_from_path}")
    endif()

    find_program(_alloy_python_from_path NAMES python)
    if(_alloy_python_from_path)
        list(APPEND _candidate_executables "${_alloy_python_from_path}")
    endif()

    list(REMOVE_DUPLICATES _candidate_executables)

    set(_selected_python "${Python3_EXECUTABLE}")
    set(_selected_has_robot FALSE)
    set(_discovery_message "using CMake Python interpreter ${Python3_EXECUTABLE}")

    foreach(_candidate IN LISTS _candidate_executables)
        if(NOT _candidate)
            continue()
        endif()

        execute_process(
            COMMAND "${_candidate}" -c "import robot, sys; print(sys.executable)"
            RESULT_VARIABLE _candidate_robot_status
            OUTPUT_VARIABLE _candidate_robot_output
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(_candidate_robot_status EQUAL 0)
            set(_selected_python "${_candidate}")
            set(_selected_has_robot TRUE)
            if(_candidate STREQUAL Python3_EXECUTABLE)
                set(_discovery_message
                    "using CMake Python interpreter ${_candidate_robot_output} with robot")
            elseif(ALLOY_RENODE_PYTHON_EXECUTABLE AND _candidate STREQUAL ALLOY_RENODE_PYTHON_EXECUTABLE)
                set(_discovery_message
                    "using ALLOY_RENODE_PYTHON_EXECUTABLE=${_candidate_robot_output}")
            else()
                set(_discovery_message
                    "using fallback Python interpreter ${_candidate_robot_output} with robot")
            endif()
            break()
        endif()
    endforeach()

    set(${OUT_EXECUTABLE} "${_selected_python}" PARENT_SCOPE)
    set(${OUT_HAS_ROBOT} "${_selected_has_robot}" PARENT_SCOPE)
    set(${OUT_DISCOVERY_MESSAGE} "${_discovery_message}" PARENT_SCOPE)
endfunction()

function(alloy_add_renode_boot_smoke TARGET_NAME)
    set(options)
    set(one_value_args
        OUTPUT_NAME
        BOARD_DIR
        FIRMWARE_SOURCE
        LINKER_SCRIPT
        PLATFORM_TEMPLATE
        PLATFORM_OUTPUT
        ROBOT_SCRIPT
        TEST_NAME
    )
    set(multi_value_args
        EXTRA_SOURCES
        TEST_LABELS
        SYMBOL_VARS
        FILE_VARS
    )
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    foreach(required_arg IN ITEMS
            OUTPUT_NAME
            BOARD_DIR
            FIRMWARE_SOURCE
            LINKER_SCRIPT
            PLATFORM_TEMPLATE
            PLATFORM_OUTPUT
            ROBOT_SCRIPT
            TEST_NAME)
        if(NOT ARG_${required_arg})
            message(FATAL_ERROR "alloy_add_renode_boot_smoke(${TARGET_NAME}) requires ${required_arg}")
        endif()
    endforeach()

    set(_renode_sources
        "${ARG_FIRMWARE_SOURCE}"
        "${ARG_BOARD_DIR}/board.cpp"
        ${ARG_EXTRA_SOURCES}
    )
    if(EXISTS "${ARG_BOARD_DIR}/syscalls.cpp")
        list(APPEND _renode_sources "${ARG_BOARD_DIR}/syscalls.cpp")
    endif()

    add_executable(${TARGET_NAME} ${_renode_sources})

    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/boards
    )
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${ALLOY_HAL_LIBRARY}
    )
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)
    target_compile_options(${TARGET_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -ffunction-sections
        -fdata-sections
        -fno-exceptions
        -fno-rtti
        -fno-threadsafe-statics
    )
    target_link_options(${TARGET_NAME} PRIVATE
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs
        --specs=nosys.specs
        -T${ARG_LINKER_SCRIPT}
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
        OUTPUT_NAME "${ARG_OUTPUT_NAME}"
        FOLDER "tests/emulation/renode"
    )

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET_NAME}> ${ARG_OUTPUT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> ${ARG_OUTPUT_NAME}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating HEX and BIN files for ${TARGET_NAME}"
    )

    configure_file(
        "${ARG_PLATFORM_TEMPLATE}"
        "${ARG_PLATFORM_OUTPUT}"
        @ONLY
    )

    message(STATUS "Renode firmware configured:")
    message(STATUS "  - Target: ${TARGET_NAME}")
    message(STATUS "  - REPL:   ${ARG_PLATFORM_OUTPUT}")

    if(ALLOY_ENABLE_RENODE_TESTS AND ALLOY_RENODE_EXECUTABLE AND ALLOY_RENODE_TEST_EXECUTABLE)
        alloy_resolve_renode_python(
            _alloy_renode_python_executable
            _alloy_robot_import_available
            _alloy_python_discovery_message
        )
        set(_renode_bundle_extract_dir
            "${CMAKE_CURRENT_BINARY_DIR}/dotnet_bundle_extract/${TARGET_NAME}")
        set(_renode_home_dir "${CMAKE_CURRENT_BINARY_DIR}/renode_home/${TARGET_NAME}")
        set(_renode_xdg_config_dir "${_renode_home_dir}/.config")
        set(_renode_xdg_cache_dir "${_renode_home_dir}/.cache")
        set(_renode_macos_config_dir "${_renode_home_dir}/Library/Application Support/renode")
        set(_renode_config_file "${_renode_home_dir}/renode.config")
        set(_renode_python_shim_dir
            "${CMAKE_CURRENT_BINARY_DIR}/renode_python_shims/${TARGET_NAME}")
        file(MAKE_DIRECTORY "${_renode_bundle_extract_dir}")
        file(MAKE_DIRECTORY "${_renode_xdg_config_dir}")
        file(MAKE_DIRECTORY "${_renode_xdg_cache_dir}")
        file(MAKE_DIRECTORY "${_renode_macos_config_dir}")
        file(MAKE_DIRECTORY "${_renode_python_shim_dir}")
        file(WRITE "${_renode_config_file}" "")

        if(NOT _alloy_robot_import_available)
            message(WARNING
                "Renode runtime test ${ARG_TEST_NAME} not registered: Python module 'robot' is not available. Checked ${_alloy_renode_python_executable}. Install Renode test requirements first or set ALLOY_RENODE_PYTHON_EXECUTABLE."
            )
            add_test(
                NAME ${ARG_TEST_NAME}
                COMMAND ${CMAKE_COMMAND} -E echo
                    "Skipped ${ARG_TEST_NAME}: Python module 'robot' is not available in ${_alloy_renode_python_executable}."
            )
            set_tests_properties(${ARG_TEST_NAME} PROPERTIES DISABLED TRUE)
            if(ARG_TEST_LABELS)
                string(JOIN ";" _renode_labels ${ARG_TEST_LABELS})
                set_tests_properties(${ARG_TEST_NAME} PROPERTIES
                    LABELS "${_renode_labels}"
                )
            endif()
        else()
            message(STATUS
                "Renode runtime test ${ARG_TEST_NAME}: ${_alloy_python_discovery_message}")
            execute_process(
                COMMAND "${_alloy_renode_python_executable}" -c "import site; print(site.getusersitepackages())"
                RESULT_VARIABLE _alloy_user_site_status
                OUTPUT_VARIABLE _alloy_user_site_output
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            file(CREATE_LINK
                "${_alloy_renode_python_executable}"
                "${_renode_python_shim_dir}/python3"
                SYMBOLIC
                COPY_ON_ERROR
            )
            file(CREATE_LINK
                "${_alloy_renode_python_executable}"
                "${_renode_python_shim_dir}/python"
                SYMBOLIC
                COPY_ON_ERROR
            )
            set(_renode_test_command
                ${_alloy_renode_python_executable}
                ${CMAKE_SOURCE_DIR}/tests/emulation/renode/common/scripts/run_renode_robot.py
                --renode-test ${ALLOY_RENODE_TEST_EXECUTABLE}
                --renode ${ALLOY_RENODE_EXECUTABLE}
                --robot ${ARG_ROBOT_SCRIPT}
                --elf $<TARGET_FILE:${TARGET_NAME}>
                --platform ${ARG_PLATFORM_OUTPUT}
                --renode-config ${_renode_config_file}
                --nm ${CMAKE_NM}
            )
            foreach(symbol_var IN LISTS ARG_SYMBOL_VARS)
                list(APPEND _renode_test_command --symbol-var ${symbol_var})
            endforeach()
            foreach(file_var IN LISTS ARG_FILE_VARS)
                list(APPEND _renode_test_command --file-var ${file_var})
            endforeach()

            add_test(
                NAME ${ARG_TEST_NAME}
                COMMAND ${_renode_test_command}
            )
            set(_renode_environment_entries
                "PATH=${_renode_python_shim_dir}:$ENV{PATH}"
                "DOTNET_BUNDLE_EXTRACT_BASE_DIR=${_renode_bundle_extract_dir}"
                "HOME=${_renode_home_dir}"
                "XDG_CONFIG_HOME=${_renode_xdg_config_dir}"
                "XDG_CACHE_HOME=${_renode_xdg_cache_dir}"
            )
            if(_alloy_user_site_status EQUAL 0 AND NOT "${_alloy_user_site_output}" STREQUAL "")
                if(DEFINED ENV{PYTHONPATH} AND NOT "$ENV{PYTHONPATH}" STREQUAL "")
                    set(_renode_pythonpath "${_alloy_user_site_output}:$ENV{PYTHONPATH}")
                else()
                    set(_renode_pythonpath "${_alloy_user_site_output}")
                endif()
                list(APPEND _renode_environment_entries
                    "PYTHONPATH=${_renode_pythonpath}"
                )
            endif()
            string(JOIN ";" _renode_environment ${_renode_environment_entries})
            set_tests_properties(${ARG_TEST_NAME} PROPERTIES
                ENVIRONMENT "${_renode_environment}"
            )
            if(ARG_TEST_LABELS)
                string(JOIN ";" _renode_labels ${ARG_TEST_LABELS})
                set_tests_properties(${ARG_TEST_NAME} PROPERTIES
                    LABELS "${_renode_labels}"
                )
            endif()

            add_custom_target(run_${TARGET_NAME}
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R ^${ARG_TEST_NAME}$
                DEPENDS ${TARGET_NAME}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running ${ARG_TEST_NAME}"
            )
        endif()
    else()
        message(STATUS
            "Renode runtime test ${ARG_TEST_NAME} not registered: enable ALLOY_ENABLE_RENODE_TESTS and install renode + renode-test."
        )
    endif()
endfunction()
