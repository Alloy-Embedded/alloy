# ==============================================================================
# ATSAME70 Xplained Board Configuration
# ==============================================================================
#
# Board: Atmel SAME70 Xplained (SAME70-XPLD)
# MCU: ATSAME70Q21B (ARM Cortex-M7 @ 300 MHz)
# Platform: same70
#
# This file provides board-specific CMake configuration:
#   - Linker script location
#   - Board-specific compile definitions
#   - Memory layout
#
# @see boards/atmel_same70_xpld/board.hpp for peripheral definitions
# ==============================================================================

message(STATUS "Configuring ATSAME70 Xplained board")

# ------------------------------------------------------------------------------
# MCU Configuration
# ------------------------------------------------------------------------------

set(ALLOY_MCU "ATSAME70Q21B" CACHE STRING "Target MCU" FORCE)
set(ALLOY_ARCH "cortex-m7" CACHE STRING "Target architecture" FORCE)

# MCU-specific defines
add_compile_definitions(
    __SAME70Q21B__               # MCU variant
    ATSAME70Q21B                 # Alternative naming
    BOARD_SAME70_XPLAINED        # Board identifier
)

# ------------------------------------------------------------------------------
# Linker Script
# ------------------------------------------------------------------------------

# Use board-specific linker script if not already set
if(NOT DEFINED LINKER_SCRIPT)
    set(LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/ATSAME70Q21.ld")
    message(STATUS "  Using board linker script: ${LINKER_SCRIPT}")
endif()

# ------------------------------------------------------------------------------
# Memory Layout
# ------------------------------------------------------------------------------

# SAME70Q21B memory configuration (for reference, defined in linker script)
set(FLASH_ORIGIN 0x00400000)
set(FLASH_SIZE   2097152)      # 2 MB = 2048 KB
set(RAM_ORIGIN   0x20400000)
set(RAM_SIZE     393216)       # 384 KB

message(STATUS "  MCU: ${ALLOY_MCU}")
message(STATUS "  Flash: ${FLASH_SIZE} bytes @ ${FLASH_ORIGIN}")
message(STATUS "  RAM: ${RAM_SIZE} bytes @ ${RAM_ORIGIN}")

# ------------------------------------------------------------------------------
# Clock Configuration
# ------------------------------------------------------------------------------

set(BOARD_XTAL_FREQ_HZ 12000000)       # 12 MHz external crystal
set(BOARD_CPU_FREQ_HZ  300000000)      # 300 MHz target (with PLL)

add_compile_definitions(
    F_CPU=${BOARD_CPU_FREQ_HZ}
    XTAL_FREQUENCY_HZ=${BOARD_XTAL_FREQ_HZ}
)

message(STATUS "  Crystal: ${BOARD_XTAL_FREQ_HZ} Hz")
message(STATUS "  Target CPU: ${BOARD_CPU_FREQ_HZ} Hz")

# ------------------------------------------------------------------------------
# Board-Specific Features
# ------------------------------------------------------------------------------

# SAME70 Xplained has:
#   - 1x User LED (PC8, active LOW)
#   - 1x User Button (PA9, active LOW)
#   - UART0 connected to EDBG (USB debug interface)
#   - Ethernet PHY (KSZ8081RNA)
#   - microSD card slot
#   - External SDRAM (IS42S16100E - 1MB)

# Enable features
add_compile_definitions(
    HAS_USER_LED                 # User LED available
    HAS_USER_BUTTON              # User button available
    HAS_DEBUG_UART               # Debug UART (UART0 via EDBG)
    HAS_ETHERNET                 # Ethernet PHY
    HAS_SDRAM                    # External SDRAM
    HAS_MICROSD                  # microSD card slot
)

# ------------------------------------------------------------------------------
# Debug Configuration
# ------------------------------------------------------------------------------

# SAME70 Xplained uses Atmel Embedded Debugger (EDBG) for debugging
# Supports SWD (Serial Wire Debug) and JTAG

message(STATUS "  Debugger: Atmel EDBG (SWD/JTAG)")
message(STATUS "  Debug UART: UART0 (via EDBG USB)")

# ------------------------------------------------------------------------------
# Board Summary
# ------------------------------------------------------------------------------

message(STATUS "------------------------------------------------------------------------------")
message(STATUS "ATSAME70 Xplained Board Configuration Complete")
message(STATUS "------------------------------------------------------------------------------")
