# Alloy Toolchain: RISC-V ESP32-C3 (bare-metal via ESP-IDF toolchain)
# Requires: ESP-IDF riscv32-esp-elf-gcc or the standalone Espressif RISC-V GCC.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)

# ESP-IDF sets IDF_TOOLS_PATH; fall back to alloy-managed and common install locations
file(GLOB _ALLOY_TC_HINTS
    "$ENV{HOME}/.alloy/toolchains/riscv32-esp-elf-gcc/*/bin"
)
set(_ESP_TOOLCHAIN_HINTS
    ${_ALLOY_TC_HINTS}
    "$ENV{IDF_TOOLS_PATH}/tools/riscv32-esp-elf"
    "$ENV{HOME}/.espressif/tools/riscv32-esp-elf"
    "/opt/espressif/tools/riscv32-esp-elf"
)

find_program(RISCV32_ESP_GCC riscv32-esp-elf-gcc
    HINTS ${_ESP_TOOLCHAIN_HINTS}
    PATH_SUFFIXES bin
    NO_DEFAULT_PATH
)

if(RISCV32_ESP_GCC)
    get_filename_component(TOOLCHAIN_BIN "${RISCV32_ESP_GCC}" DIRECTORY)
    message(STATUS "Using ESP32-C3 RISC-V toolchain from: ${TOOLCHAIN_BIN}")
else()
    set(TOOLCHAIN_BIN "")
    message(WARNING "riscv32-esp-elf-gcc not found — install via alloyctl or ESP-IDF")
endif()

function(_esp_tool varname toolname)
    if(TOOLCHAIN_BIN)
        set(${varname} "${TOOLCHAIN_BIN}/${toolname}" PARENT_SCOPE)
    else()
        set(${varname} "${toolname}" PARENT_SCOPE)
    endif()
endfunction()

_esp_tool(CMAKE_C_COMPILER   riscv32-esp-elf-gcc)
_esp_tool(CMAKE_CXX_COMPILER riscv32-esp-elf-g++)
_esp_tool(CMAKE_ASM_COMPILER riscv32-esp-elf-gcc)
_esp_tool(CMAKE_AR           riscv32-esp-elf-ar)
_esp_tool(CMAKE_OBJCOPY      riscv32-esp-elf-objcopy)
_esp_tool(CMAKE_SIZE         riscv32-esp-elf-size)
_esp_tool(CMAKE_RANLIB       riscv32-esp-elf-ranlib)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_INIT   "-march=rv32imc -mabi=ilp32 -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "-march=rv32imc -mabi=ilp32 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections")

set(CMAKE_C_FLAGS_DEBUG_INIT   "-g -Og")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -Og")
set(CMAKE_C_FLAGS_RELEASE_INIT   "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Os -DNDEBUG")

message(STATUS "Toolchain: ESP32-C3 RISC-V (riscv32-esp-elf-gcc)")
