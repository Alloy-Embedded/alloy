# Alloy Toolchain: Xtensa LX6 ESP32 (bare-metal, ESP-IDF bootloader flow)
# Requires: xtensa-esp32-elf-gcc (alloy-managed or ESP-IDF)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR xtensa)

# alloy-managed toolchain first, then IDF locations
file(GLOB _ALLOY_TC_HINTS
    "$ENV{HOME}/.alloy/toolchains/xtensa-esp-elf-gcc/*/bin"
)
set(_ESP_TOOLCHAIN_HINTS
    ${_ALLOY_TC_HINTS}
    "$ENV{IDF_TOOLS_PATH}/tools/xtensa-esp32-elf"
    "$ENV{HOME}/.espressif/tools/xtensa-esp32-elf"
    "/opt/espressif/tools/xtensa-esp32-elf"
)

find_program(XTENSA_ESP32_GCC xtensa-esp32-elf-gcc
    HINTS ${_ESP_TOOLCHAIN_HINTS}
    PATH_SUFFIXES bin
    NO_DEFAULT_PATH
)

if(XTENSA_ESP32_GCC)
    get_filename_component(TOOLCHAIN_BIN "${XTENSA_ESP32_GCC}" DIRECTORY)
    message(STATUS "Using ESP32 Xtensa toolchain from: ${TOOLCHAIN_BIN}")
else()
    set(TOOLCHAIN_BIN "")
    message(WARNING "xtensa-esp32-elf-gcc not found in alloy-managed or IDF paths")
endif()

function(_esp32_tool varname toolname)
    if(TOOLCHAIN_BIN)
        set(${varname} "${TOOLCHAIN_BIN}/${toolname}" PARENT_SCOPE)
    else()
        set(${varname} "${toolname}" PARENT_SCOPE)
    endif()
endfunction()

_esp32_tool(CMAKE_C_COMPILER   xtensa-esp32-elf-gcc)
_esp32_tool(CMAKE_CXX_COMPILER xtensa-esp32-elf-g++)
_esp32_tool(CMAKE_ASM_COMPILER xtensa-esp32-elf-gcc)
_esp32_tool(CMAKE_AR           xtensa-esp32-elf-ar)
_esp32_tool(CMAKE_OBJCOPY      xtensa-esp32-elf-objcopy)
_esp32_tool(CMAKE_SIZE         xtensa-esp32-elf-size)
_esp32_tool(CMAKE_RANLIB       xtensa-esp32-elf-ranlib)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_INIT   "-mlongcalls -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "-mlongcalls -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections")

set(CMAKE_C_FLAGS_DEBUG_INIT   "-g -Og")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -Og")
set(CMAKE_C_FLAGS_RELEASE_INIT   "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Os -DNDEBUG")

message(STATUS "Toolchain: ESP32 Xtensa LX6 (xtensa-esp32-elf-gcc)")
