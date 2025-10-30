# Alloy Compiler Options
# Common compiler flags for all targets

# Detect compiler
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(ALLOY_COMPILER_IS_GCC TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(ALLOY_COMPILER_IS_CLANG TRUE)
endif()

# Common warning flags
set(ALLOY_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wcast-align
    -Wunused
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
)

# GCC-specific flags
if(ALLOY_COMPILER_IS_GCC)
    list(APPEND ALLOY_WARNING_FLAGS
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
    )
endif()

# Clang-specific flags
if(ALLOY_COMPILER_IS_CLANG)
    list(APPEND ALLOY_WARNING_FLAGS
        -Wmost
    )
endif()

# Debug flags (as string to avoid CMake generator expression issues)
set(ALLOY_DEBUG_FLAGS -g -O0 -DDEBUG -DALLOY_DEBUG)

# Release flags
set(ALLOY_RELEASE_FLAGS -O3 -DNDEBUG)

# RelWithDebInfo flags
set(ALLOY_RELWITHDEBINFO_FLAGS -O2 -g -DNDEBUG)

# MinSizeRel flags
set(ALLOY_MINSIZEREL_FLAGS -Os -DNDEBUG)

# Function to apply Alloy compiler options to a target
function(alloy_target_compile_options target_name)
    target_compile_options(${target_name} PRIVATE
        ${ALLOY_WARNING_FLAGS}
    )

    # Add build-type specific flags
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${target_name} PRIVATE ${ALLOY_DEBUG_FLAGS})
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${target_name} PRIVATE ${ALLOY_RELEASE_FLAGS})
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        target_compile_options(${target_name} PRIVATE ${ALLOY_RELWITHDEBINFO_FLAGS})
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        target_compile_options(${target_name} PRIVATE ${ALLOY_MINSIZEREL_FLAGS})
    endif()
endfunction()

message(STATUS "Compiler options configured for ${CMAKE_CXX_COMPILER_ID}")
