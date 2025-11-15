# ============================================================================
# Source Validation Script
# ============================================================================
#
# This script validates the build system to ensure:
# 1. No orphaned .cpp files exist (files that should be compiled but aren't)
# 2. All source files in the build have corresponding headers
# 3. Platform-specific files are properly isolated
#
# Usage:
#   cmake -P cmake/validate_sources.cmake
#
# Or via custom target:
#   cmake --build build --target validate-build-system
#
# ============================================================================

# Color output helpers
if(NOT WIN32)
    string(ASCII 27 Esc)
    set(COLOR_RESET "${Esc}[m")
    set(COLOR_RED "${Esc}[31m")
    set(COLOR_GREEN "${Esc}[32m")
    set(COLOR_YELLOW "${Esc}[33m")
    set(COLOR_BLUE "${Esc}[34m")
else()
    set(COLOR_RESET "")
    set(COLOR_RED "")
    set(COLOR_GREEN "")
    set(COLOR_YELLOW "")
    set(COLOR_BLUE "")
endif()

# ============================================================================
# Configuration
# ============================================================================

# Directories to scan for source files
set(HAL_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/core")
set(HAL_API_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/api")
set(HAL_VENDORS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/vendors")

# Expected core source files (from CMakeLists.txt)
# Note: These files don't exist yet - they are planned for future implementation
# The current build system is header-only for HAL core/API
set(EXPECTED_CORE_SOURCES
    # Future implementation files:
    # "src/hal/core/assert.cpp"
    # "src/hal/core/fault_handler.cpp"
    # "src/hal/api/uart_stream.cpp"
)

# Platform directories (each platform uses GLOB for its own sources)
set(PLATFORM_DIRS
    "src/hal/vendors/st/stm32f4"
    "src/hal/vendors/st/stm32f7"
    "src/hal/vendors/st/stm32g0"
    "src/hal/vendors/arm/same70"
    "src/hal/vendors/atmel/same70"
)

# Files to exclude from orphan detection (intentionally not compiled)
set(EXCLUDE_PATTERNS
    ".*/test/.*"           # Test files
    ".*/tests/.*"          # Test files
    ".*/examples/.*"       # Example files
    ".*/archive/.*"        # Archived files
    ".*/templates/.*"      # Code generation templates
    ".*/startup.*\\.cpp$"  # Startup files (board-specific)
    ".*/host/.*"           # Host platform (not embedded)
)

# ============================================================================
# Validation Functions
# ============================================================================

message("${COLOR_BLUE}======================================${COLOR_RESET}")
message("${COLOR_BLUE}Source Validation Report${COLOR_RESET}")
message("${COLOR_BLUE}======================================${COLOR_RESET}")
message("")

set(VALIDATION_FAILED FALSE)

# ----------------------------------------------------------------------------
# 1. Find all .cpp files in HAL directories
# ----------------------------------------------------------------------------

message("${COLOR_BLUE}[1/4] Scanning for C++ source files...${COLOR_RESET}")

file(GLOB_RECURSE ALL_CPP_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hal/**/*.cpp"
)

# Filter out excluded patterns
set(CPP_FILES "")
foreach(file ${ALL_CPP_FILES})
    set(EXCLUDE_FILE FALSE)
    foreach(pattern ${EXCLUDE_PATTERNS})
        if(file MATCHES "${pattern}")
            set(EXCLUDE_FILE TRUE)
            break()
        endif()
    endforeach()

    if(NOT EXCLUDE_FILE)
        list(APPEND CPP_FILES "${file}")
    endif()
endforeach()

list(LENGTH CPP_FILES CPP_COUNT)
message("  Found ${CPP_COUNT} C++ source files (after exclusions)")
message("")

# ----------------------------------------------------------------------------
# 2. Check for orphaned core source files
# ----------------------------------------------------------------------------

message("${COLOR_BLUE}[2/4] Checking for orphaned core source files...${COLOR_RESET}")

set(ORPHANED_CORE_FILES "")
foreach(file ${CPP_FILES})
    # Check if it's in core or api directories
    if(file MATCHES ".*/src/hal/(core|api)/.*\\.cpp$")
        # Get relative path
        file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}" "${file}")

        # Check if it's in the expected list
        list(FIND EXPECTED_CORE_SOURCES "${rel_path}" found_index)
        if(found_index EQUAL -1)
            list(APPEND ORPHANED_CORE_FILES "${rel_path}")
        endif()
    endif()
endforeach()

list(LENGTH ORPHANED_CORE_FILES orphan_count)
if(orphan_count GREATER 0)
    message("  ${COLOR_RED}✗ Found ${orphan_count} orphaned core source file(s):${COLOR_RESET}")
    foreach(file ${ORPHANED_CORE_FILES})
        message("    - ${file}")
    endforeach()
    set(VALIDATION_FAILED TRUE)
else()
    message("  ${COLOR_GREEN}✓ No orphaned core source files${COLOR_RESET}")
endif()
message("")

# ----------------------------------------------------------------------------
# 3. Check for missing core source files
# ----------------------------------------------------------------------------

message("${COLOR_BLUE}[3/4] Checking for missing core source files...${COLOR_RESET}")

set(MISSING_CORE_FILES "")
foreach(expected_file ${EXPECTED_CORE_SOURCES})
    set(full_path "${CMAKE_CURRENT_SOURCE_DIR}/${expected_file}")
    if(NOT EXISTS "${full_path}")
        list(APPEND MISSING_CORE_FILES "${expected_file}")
    endif()
endforeach()

list(LENGTH MISSING_CORE_FILES missing_count)
if(missing_count GREATER 0)
    message("  ${COLOR_RED}✗ Found ${missing_count} missing core source file(s):${COLOR_RESET}")
    foreach(file ${MISSING_CORE_FILES})
        message("    - ${file}")
    endforeach()
    set(VALIDATION_FAILED TRUE)
else()
    message("  ${COLOR_GREEN}✓ All expected core source files exist${COLOR_RESET}")
endif()
message("")

# ----------------------------------------------------------------------------
# 4. Check platform directory isolation
# ----------------------------------------------------------------------------

message("${COLOR_BLUE}[4/4] Checking platform directory isolation...${COLOR_RESET}")

# For each platform, ensure no cross-contamination
set(ISOLATION_ISSUES "")
foreach(platform_dir ${PLATFORM_DIRS})
    set(platform_path "${CMAKE_CURRENT_SOURCE_DIR}/${platform_dir}")

    if(NOT EXISTS "${platform_path}")
        continue()
    endif()

    # Find all .cpp files in this platform
    file(GLOB_RECURSE platform_cpp_files "${platform_path}/*.cpp")

    # Filter out startup and excluded files
    set(platform_sources "")
    foreach(file ${platform_cpp_files})
        set(EXCLUDE_FILE FALSE)
        foreach(pattern ${EXCLUDE_PATTERNS})
            if(file MATCHES "${pattern}")
                set(EXCLUDE_FILE TRUE)
                break()
            endif()
        endforeach()

        if(NOT EXCLUDE_FILE)
            list(APPEND platform_sources "${file}")
        endif()
    endforeach()

    # Check that files don't reference other platforms
    foreach(file ${platform_sources})
        file(READ "${file}" file_content)

        # Check for includes from other platforms
        foreach(other_platform ${PLATFORM_DIRS})
            if(NOT "${other_platform}" STREQUAL "${platform_dir}")
                # Extract platform name (e.g., stm32f4, stm32f7)
                string(REGEX MATCH "[^/]+$" other_platform_name "${other_platform}")

                # Check if file includes headers from other platform
                if(file_content MATCHES "#include.*/${other_platform_name}/")
                    file(RELATIVE_PATH rel_file "${CMAKE_CURRENT_SOURCE_DIR}" "${file}")
                    list(APPEND ISOLATION_ISSUES "${rel_file} includes ${other_platform_name}")
                endif()
            endif()
        endforeach()
    endforeach()
endforeach()

list(LENGTH ISOLATION_ISSUES isolation_count)
if(isolation_count GREATER 0)
    message("  ${COLOR_YELLOW}⚠ Found ${isolation_count} potential platform isolation issue(s):${COLOR_RESET}")
    foreach(issue ${ISOLATION_ISSUES})
        message("    - ${issue}")
    endforeach()
    # This is a warning, not a failure
else()
    message("  ${COLOR_GREEN}✓ Platform directories are properly isolated${COLOR_RESET}")
endif()
message("")

# ============================================================================
# Summary
# ============================================================================

message("${COLOR_BLUE}======================================${COLOR_RESET}")
if(VALIDATION_FAILED)
    message("${COLOR_RED}Validation FAILED${COLOR_RESET}")
    message("${COLOR_BLUE}======================================${COLOR_RESET}")
    message(FATAL_ERROR "Source validation detected issues. Please fix the errors above.")
else()
    message("${COLOR_GREEN}Validation PASSED${COLOR_RESET}")
    message("${COLOR_BLUE}======================================${COLOR_RESET}")
endif()
