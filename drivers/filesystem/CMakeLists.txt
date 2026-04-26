include(FetchContent)

# ── littlefs v2.8.1 ───────────────────────────────────────────────────────────
FetchContent_Declare(
    littlefs
    GIT_REPOSITORY https://github.com/littlefs-project/littlefs.git
    GIT_TAG        v2.8.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(littlefs)

add_library(alloy-littlefs STATIC
    ${littlefs_SOURCE_DIR}/lfs.c
    ${littlefs_SOURCE_DIR}/lfs_util.c
)
target_include_directories(alloy-littlefs PUBLIC
    ${littlefs_SOURCE_DIR}
)
target_compile_definitions(alloy-littlefs PUBLIC
    LFS_NO_MALLOC=1
    LFS_YES_TRACE=0
)
target_compile_options(alloy-littlefs PRIVATE
    -Wno-unused-function
)
add_library(alloy::littlefs ALIAS alloy-littlefs)

# ── FatFS R0.15 ───────────────────────────────────────────────────────────────
# Fetches the community-maintained mirror with CMake support.
FetchContent_Declare(
    fatfs
    GIT_REPOSITORY https://github.com/abbrev/fatfs.git
    GIT_TAG        R0.15
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(fatfs)

# ffconf.h is provided by us — override the default with our configuration.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/fatfs/ffconf.h"
    "${CMAKE_CURRENT_BINARY_DIR}/fatfs/ffconf.h"
    COPYONLY
)

add_library(alloy-fatfs STATIC
    ${fatfs_SOURCE_DIR}/source/ff.c
    ${fatfs_SOURCE_DIR}/source/ffunicode.c
    fatfs/fatfs_diskio.cpp
)
target_include_directories(alloy-fatfs PUBLIC
    ${fatfs_SOURCE_DIR}/source
    ${CMAKE_CURRENT_BINARY_DIR}/fatfs
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)
target_compile_options(alloy-fatfs PRIVATE -Wno-unused-function)
add_library(alloy::fatfs ALIAS alloy-fatfs)
