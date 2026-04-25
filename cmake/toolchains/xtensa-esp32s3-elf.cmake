# Alloy Toolchain: Xtensa LX7 ESP32-S3 (bare-metal via ESP-IDF toolchain)
# Requires: ESP-IDF xtensa-esp32s3-elf-gcc or the standalone Espressif Xtensa GCC.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR xtensa)

set(_ESP_TOOLCHAIN_HINTS
    "$ENV{IDF_TOOLS_PATH}/tools/xtensa-esp32s3-elf"
    "$ENV{HOME}/.espressif/tools/xtensa-esp32s3-elf"
    "/opt/espressif/tools/xtensa-esp32s3-elf"
)

find_program(XTENSA_ESP32S3_GCC xtensa-esp32s3-elf-gcc
    HINTS ${_ESP_TOOLCHAIN_HINTS}
    PATH_SUFFIXES bin
)

if(XTENSA_ESP32S3_GCC)
    get_filename_component(TOOLCHAIN_BIN "${XTENSA_ESP32S3_GCC}" DIRECTORY)
    set(TOOLCHAIN_PREFIX "${TOOLCHAIN_BIN}/")
    message(STATUS "Using ESP32-S3 Xtensa toolchain from: ${TOOLCHAIN_BIN}")
else()
    set(TOOLCHAIN_PREFIX "xtensa-esp32s3-elf-")
    message(WARNING "xtensa-esp32s3-elf-gcc not found — install ESP-IDF or the standalone toolchain")
endif()

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}xtensa-esp32s3-elf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}xtensa-esp32s3-elf-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}xtensa-esp32s3-elf-gcc)
set(CMAKE_AR           ${TOOLCHAIN_PREFIX}xtensa-esp32s3-elf-ar)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}xtensa-esp32s3-elf-objcopy)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}xtensa-esp32s3-elf-size)
set(CMAKE_RANLIB       ${TOOLCHAIN_PREFIX}xtensa-esp32s3-elf-ranlib)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_INIT   "-mlongcalls -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "-mlongcalls -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections")

set(CMAKE_C_FLAGS_DEBUG_INIT   "-g -Og")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -Og")
set(CMAKE_C_FLAGS_RELEASE_INIT   "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Os -DNDEBUG")

message(STATUS "Toolchain: ESP32-S3 Xtensa (xtensa-esp32s3-elf-gcc)")
