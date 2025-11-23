# ============================================================================
# MicroCore Dependency Validation
# ============================================================================
# Validates that all required tools and dependencies are installed and meet
# minimum version requirements. Provides helpful error messages with
# installation instructions.

# ============================================================================
# CMake Version Check
# ============================================================================
if(CMAKE_VERSION VERSION_LESS "3.25")
    message(FATAL_ERROR
        "\n"
        "===========================================================\n"
        "❌ ERROR: CMake version too old\n"
        "===========================================================\n"
        "\n"
        "Required: CMake 3.25 or newer\n"
        "Found:    CMake ${CMAKE_VERSION}\n"
        "\n"
        "MicroCore requires CMake 3.25+ for modern C++20/23 support.\n"
        "\n"
        "Installation:\n"
        "  macOS:   brew upgrade cmake\n"
        "  Linux:   pip3 install --upgrade cmake\n"
        "  Windows: choco upgrade cmake\n"
        "\n"
        "Or download from: https://cmake.org/download/\n"
        "\n"
        "===========================================================\n"
    )
endif()

# ============================================================================
# Python Version Check (for code generation)
# ============================================================================
function(check_python_version)
    find_package(Python3 COMPONENTS Interpreter)

    if(NOT Python3_FOUND)
        message(WARNING
            "\n"
            "===========================================================\n"
            "⚠️  WARNING: Python 3 not found\n"
            "===========================================================\n"
            "\n"
            "Python 3.10+ is required for code generation tools.\n"
            "\n"
            "Installation:\n"
            "  macOS:   brew install python3\n"
            "  Linux:   sudo apt-get install python3 python3-pip\n"
            "  Windows: choco install python3\n"
            "\n"
            "Or download from: https://www.python.org/downloads/\n"
            "\n"
            "Code generation will be unavailable until Python is installed.\n"
            "===========================================================\n"
        )
        return()
    endif()

    if(Python3_VERSION VERSION_LESS "3.10")
        message(WARNING
            "\n"
            "===========================================================\n"
            "⚠️  WARNING: Python version too old\n"
            "===========================================================\n"
            "\n"
            "Required: Python 3.10 or newer\n"
            "Found:    Python ${Python3_VERSION}\n"
            "\n"
            "Code generation requires Python 3.10+ for modern type hints\n"
            "and pattern matching features.\n"
            "\n"
            "Upgrade:\n"
            "  macOS:   brew upgrade python3\n"
            "  Linux:   Use pyenv or system package manager\n"
            "  Windows: Download from python.org\n"
            "\n"
            "===========================================================\n"
        )
    else()
        message(STATUS "Python ${Python3_VERSION} found: ${Python3_EXECUTABLE}")
    endif()

    # Check for required Python packages
    if(Python3_FOUND)
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import yaml"
            RESULT_VARIABLE YAML_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
        )

        if(NOT YAML_RESULT EQUAL 0)
            message(WARNING
                "\n"
                "⚠️  WARNING: Python package 'pyyaml' not found\n"
                "\n"
                "Install with: ${Python3_EXECUTABLE} -m pip install pyyaml\n"
            )
        endif()
    endif()
endfunction()

# ============================================================================
# ARM Toolchain Check (for embedded targets)
# ============================================================================
function(check_arm_toolchain)
    # Skip check for host/simulator platforms
    if(MICROCORE_PLATFORM STREQUAL "linux" OR
       MICROCORE_PLATFORM STREQUAL "host" OR
       MICROCORE_PLATFORM STREQUAL "esp32")
        return()
    endif()

    # Check if cross-compiling for ARM
    if(NOT CMAKE_CROSSCOMPILING)
        message(WARNING
            "\n"
            "===========================================================\n"
            "⚠️  WARNING: Not cross-compiling for ARM\n"
            "===========================================================\n"
            "\n"
            "Board:    ${MICROCORE_BOARD}\n"
            "Platform: ${MICROCORE_PLATFORM}\n"
            "\n"
            "This board requires ARM cross-compilation.\n"
            "\n"
            "Make sure to use the ARM toolchain:\n"
            "  cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake ..\n"
            "\n"
            "Or use the ucore CLI:\n"
            "  ./ucore build ${MICROCORE_BOARD} <example>\n"
            "\n"
            "===========================================================\n"
        )
    endif()

    # Verify compiler is actually ARM GCC
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} --version
            OUTPUT_VARIABLE COMPILER_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(NOT COMPILER_VERSION MATCHES "arm-none-eabi")
            message(WARNING
                "\n"
                "⚠️  WARNING: Compiler does not appear to be ARM GCC\n"
                "\n"
                "Expected: arm-none-eabi-gcc\n"
                "Found:    ${CMAKE_CXX_COMPILER}\n"
                "\n"
                "Install ARM toolchain:\n"
                "  macOS:   brew install --cask gcc-arm-embedded\n"
                "  Linux:   sudo apt-get install gcc-arm-none-eabi\n"
                "  Windows: choco install gcc-arm-embedded\n"
                "\n"
                "Or download from: https://developer.arm.com/downloads/-/gnu-rm\n"
            )
        else()
            # Extract version number
            string(REGEX MATCH "([0-9]+\\.[0-9]+\\.[0-9]+)" ARM_GCC_VERSION "${COMPILER_VERSION}")
            message(STATUS "ARM GCC ${ARM_GCC_VERSION} found: ${CMAKE_CXX_COMPILER}")

            # Warn if version is too old
            if(ARM_GCC_VERSION VERSION_LESS "10.0.0")
                message(WARNING
                    "\n"
                    "⚠️  WARNING: ARM GCC version is old\n"
                    "\n"
                    "Found:       ${ARM_GCC_VERSION}\n"
                    "Recommended: 10.0.0 or newer\n"
                    "\n"
                    "Older versions may have limited C++20/23 support.\n"
                    "Consider upgrading for best results.\n"
                )
            endif()
        endif()
    endif()
endfunction()

# ============================================================================
# Flash Tool Check
# ============================================================================
function(check_flash_tools)
    # Skip check for host platform
    if(MICROCORE_PLATFORM STREQUAL "linux" OR MICROCORE_PLATFORM STREQUAL "host")
        return()
    endif()

    # Check for st-flash (STM32 boards)
    if(MICROCORE_PLATFORM MATCHES "^stm32")
        find_program(ST_FLASH st-flash)

        if(ST_FLASH)
            execute_process(
                COMMAND ${ST_FLASH} --version
                OUTPUT_VARIABLE ST_FLASH_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            message(STATUS "st-flash found: ${ST_FLASH}")
        else()
            message(STATUS
                "\n"
                "ℹ️  INFO: st-flash not found (optional for STM32 boards)\n"
                "\n"
                "Install for STM32 flashing support:\n"
                "  macOS:   brew install stlink\n"
                "  Linux:   sudo apt-get install stlink-tools\n"
                "  Windows: choco install stlink\n"
                "\n"
                "Alternatively, use OpenOCD or other flash tools.\n"
            )
        endif()
    endif()

    # Check for OpenOCD (universal)
    find_program(OPENOCD openocd)

    if(OPENOCD)
        execute_process(
            COMMAND ${OPENOCD} --version
            OUTPUT_VARIABLE OPENOCD_VERSION
            ERROR_VARIABLE OPENOCD_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(STATUS "OpenOCD found: ${OPENOCD}")
    else()
        message(STATUS
            "\n"
            "ℹ️  INFO: OpenOCD not found (optional for debugging)\n"
            "\n"
            "Install for debugging support:\n"
            "  macOS:   brew install openocd\n"
            "  Linux:   sudo apt-get install openocd\n"
            "  Windows: choco install openocd\n"
        )
    endif()

    # Check for JLink (optional)
    find_program(JLINK JLinkExe)
    if(JLINK)
        message(STATUS "JLink found: ${JLINK}")
    endif()
endfunction()

# ============================================================================
# Clang Tools Check (optional static analysis)
# ============================================================================
function(check_clang_tools)
    find_program(CLANG_FORMAT clang-format)
    if(CLANG_FORMAT)
        execute_process(
            COMMAND ${CLANG_FORMAT} --version
            OUTPUT_VARIABLE CLANG_FORMAT_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(STATUS "clang-format found: ${CLANG_FORMAT}")
    endif()

    find_program(CLANG_TIDY clang-tidy)
    if(CLANG_TIDY)
        execute_process(
            COMMAND ${CLANG_TIDY} --version
            OUTPUT_VARIABLE CLANG_TIDY_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(STATUS "clang-tidy found: ${CLANG_TIDY}")
    endif()
endfunction()

# ============================================================================
# Main Dependency Validation
# ============================================================================
function(validate_dependencies)
    message(STATUS "")
    message(STATUS "========================================")
    message(STATUS "Validating MicroCore Dependencies")
    message(STATUS "========================================")

    # Core dependencies (always required)
    check_python_version()

    # Embedded-specific dependencies
    check_arm_toolchain()
    check_flash_tools()

    # Optional development tools
    check_clang_tools()

    message(STATUS "========================================")
    message(STATUS "Dependency validation complete")
    message(STATUS "========================================")
    message(STATUS "")
endfunction()

# ============================================================================
# Summary Function (for ucore doctor command)
# ============================================================================
function(print_dependency_summary)
    message("")
    message("===========================================================")
    message("MicroCore Dependency Summary")
    message("===========================================================")
    message("")

    # CMake
    message("✓ CMake: ${CMAKE_VERSION}")

    # Python
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_FOUND)
        message("✓ Python: ${Python3_VERSION}")
    else()
        message("✗ Python: Not found")
    endif()

    # ARM GCC
    if(CMAKE_CXX_COMPILER MATCHES "arm-none-eabi")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} --version
            OUTPUT_VARIABLE COMPILER_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(REGEX MATCH "([0-9]+\\.[0-9]+\\.[0-9]+)" ARM_GCC_VERSION "${COMPILER_VERSION}")
        message("✓ ARM GCC: ${ARM_GCC_VERSION}")
    else()
        message("○ ARM GCC: Not in use (host build)")
    endif()

    # Flash tools
    find_program(ST_FLASH st-flash QUIET)
    if(ST_FLASH)
        message("✓ st-flash: Found")
    else()
        message("○ st-flash: Not found (optional)")
    endif()

    find_program(OPENOCD openocd QUIET)
    if(OPENOCD)
        message("✓ OpenOCD: Found")
    else()
        message("○ OpenOCD: Not found (optional)")
    endif()

    # Clang tools
    find_program(CLANG_FORMAT clang-format QUIET)
    if(CLANG_FORMAT)
        message("✓ clang-format: Found")
    else()
        message("○ clang-format: Not found (optional)")
    endif()

    find_program(CLANG_TIDY clang-tidy QUIET)
    if(CLANG_TIDY)
        message("✓ clang-tidy: Found")
    else()
        message("○ clang-tidy: Not found (optional)")
    endif()

    message("")
    message("Legend:")
    message("  ✓ = Found and validated")
    message("  ✗ = Missing (required)")
    message("  ○ = Not found (optional)")
    message("")
    message("===========================================================")
    message("")
endfunction()
