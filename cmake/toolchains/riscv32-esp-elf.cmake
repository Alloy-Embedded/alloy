# Alloy Toolchain: RISC-V ESP32-C3 (bare-metal via ESP-IDF toolchain)
# Requires: ESP-IDF riscv32-esp-elf-gcc or the standalone Espressif RISC-V GCC.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)

# ESP-IDF sets IDF_TOOLS_PATH; fall back to common install locations
set(_ESP_TOOLCHAIN_HINTS
    "$ENV{IDF_TOOLS_PATH}/tools/riscv32-esp-elf"
    "$ENV{HOME}/.espressif/tools/riscv32-esp-elf"
    "/opt/espressif/tools/riscv32-esp-elf"
)

find_program(RISCV32_ESP_GCC riscv32-esp-elf-gcc
    HINTS ${_ESP_TOOLCHAIN_HINTS}
    PATH_SUFFIXES bin
)

if(RISCV32_ESP_GCC)
    get_filename_component(TOOLCHAIN_BIN "${RISCV32_ESP_GCC}" DIRECTORY)
    set(TOOLCHAIN_PREFIX "${TOOLCHAIN_BIN}/")
    message(STATUS "Using ESP32-C3 RISC-V toolchain from: ${TOOLCHAIN_BIN}")
else()
    set(TOOLCHAIN_PREFIX "riscv32-esp-elf-")
    message(WARNING "riscv32-esp-elf-gcc not found — install ESP-IDF or the standalone toolchain")
endif()

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}riscv32-esp-elf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}riscv32-esp-elf-g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}riscv32-esp-elf-gcc)
set(CMAKE_AR           ${TOOLCHAIN_PREFIX}riscv32-esp-elf-ar)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}riscv32-esp-elf-objcopy)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}riscv32-esp-elf-size)
set(CMAKE_RANLIB       ${TOOLCHAIN_PREFIX}riscv32-esp-elf-ranlib)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_INIT   "-march=rv32imc -mabi=ilp32 -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT "-march=rv32imc -mabi=ilp32 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--gc-sections")

set(CMAKE_C_FLAGS_DEBUG_INIT   "-g -Og")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -Og")
set(CMAKE_C_FLAGS_RELEASE_INIT   "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Os -DNDEBUG")

message(STATUS "Toolchain: ESP32-C3 RISC-V (riscv32-esp-elf-gcc)")
