# boards/nucleo_g071rb/mcuboot/memory_map.cmake
#
# MCUboot partition layout for Nucleo-G071RB (STM32G071RBT6)
# Internal flash: 128 KB at 0x08000000, uniform 2 KB pages.
#
# Include this file when ALLOY_MCUBOOT=ON is set in the parent CMakeLists.txt:
#   if(ALLOY_MCUBOOT)
#       include(${CMAKE_CURRENT_SOURCE_DIR}/boards/nucleo_g071rb/mcuboot/memory_map.cmake)
#   endif()
#
# Layout (total 128 KB):
#   Bootloader : 0x08000000  32 KB  (16 pages, holds MCUboot image)
#   Primary    : 0x08008000  44 KB  (22 pages, active application slot)
#   Secondary  : 0x08013000  44 KB  (22 pages, upgrade staging slot)
#   Scratch    :  0x0801E000   8 KB  ( 4 pages, swap scratch area)

include_guard(GLOBAL)

# ── Base address ──────────────────────────────────────────────────────────────
set(ALLOY_FLASH_BASE          "0x08000000" CACHE STRING "Internal flash base address")

# ── Bootloader region ─────────────────────────────────────────────────────────
set(ALLOY_BOOT_OFFSET         "0x08000000" CACHE STRING "MCUboot bootloader start address")
set(ALLOY_BOOT_SIZE           "0x8000"     CACHE STRING "MCUboot bootloader region size (32 KB)")

# ── Primary application slot ──────────────────────────────────────────────────
set(ALLOY_PRIMARY_OFFSET      "0x08008000" CACHE STRING "Primary slot start address")
set(ALLOY_PRIMARY_SIZE        "0xB000"     CACHE STRING "Primary slot size (44 KB)")

# ── Secondary (upgrade) slot ──────────────────────────────────────────────────
set(ALLOY_SECONDARY_OFFSET    "0x08013000" CACHE STRING "Secondary slot start address")
set(ALLOY_SECONDARY_SIZE      "0xB000"     CACHE STRING "Secondary slot size (44 KB)")

# ── Swap scratch area ─────────────────────────────────────────────────────────
set(ALLOY_SCRATCH_OFFSET      "0x0801E000" CACHE STRING "Scratch area start address")
set(ALLOY_SCRATCH_SIZE        "0x2000"     CACHE STRING "Scratch area size (8 KB)")

# ── imgtool / MCUboot build parameters ───────────────────────────────────────
set(ALLOY_MCUBOOT_HEADER_SIZE "0x200"      CACHE STRING "MCUboot image header size")
set(ALLOY_MCUBOOT_ALIGN       "8"          CACHE STRING "MCUboot write alignment")
set(ALLOY_MCUBOOT_SLOT_SIZE   "${ALLOY_PRIMARY_SIZE}" CACHE STRING "Slot size passed to imgtool")
