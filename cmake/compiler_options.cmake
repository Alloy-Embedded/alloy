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

# Debug flags
set(ALLOY_DEBUG_FLAGS
    -g
    -O0
    -DDEBUG
    -DALLOY_DEBUG
)

# Release flags
set(ALLOY_RELEASE_FLAGS
    -O3
    -DNDEBUG
)

# RelWithDebInfo flags
set(ALLOY_RELWITHDEBINFO_FLAGS
    -O2
    -g
    -DNDEBUG
)

# MinSizeRel flags
set(ALLOY_MINSIZEREL_FLAGS
    -Os
    -DNDEBUG
)

# Function to apply Alloy compiler options to a target
function(alloy_target_compile_options target_name)
    target_compile_options(${target_name} PRIVATE
        ${ALLOY_WARNING_FLAGS}
        $<$<CONFIG:Debug>:${ALLOY_DEBUG_FLAGS}>
        $<$<CONFIG:Release>:${ALLOY_RELEASE_FLAGS}>
        $<$<CONFIG:RelWithDebInfo>:${ALLOY_RELWITHDEBINFO_FLAGS}>
        $<$<CONFIG:MinSizeRel>:${ALLOY_MINSIZEREL_FLAGS}>
    )
endfunction()

# Global compile options (applied to all targets)
add_compile_options(
    ${ALLOY_WARNING_FLAGS}
    $<$<CONFIG:Debug>:${ALLOY_DEBUG_FLAGS}>
    $<$<CONFIG:Release>:${ALLOY_RELEASE_FLAGS}>
    $<$<CONFIG:RelWithDebInfo>:${ALLOY_RELWITHDEBINFO_FLAGS}>
    $<$<CONFIG:MinSizeRel>:${ALLOY_MINSIZEREL_FLAGS}>
)

message(STATUS "Compiler options configured for ${CMAKE_CXX_COMPILER_ID}")
