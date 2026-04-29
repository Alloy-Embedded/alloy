# boards/nucleo_f401re/mcuboot/memory_map.cmake
#
# MCUboot partition layout for Nucleo-F401RE (STM32F401RET6)
# Internal flash: 512 KB at 0x08000000, variable-size sectors.
#
# STM32F401 sector map:
#   Sector 0:  16 KB  (0x08000000)
#   Sector 1:  16 KB  (0x08004000)
#   Sector 2:  16 KB  (0x08008000)
#   Sector 3:  16 KB  (0x0800C000)
#   Sector 4:  64 KB  (0x08010000)
#   Sector 5: 128 KB  (0x08020000)   ← primary slot
#   Sector 6: 128 KB  (0x08040000)   ← secondary slot
#   Sector 7: 128 KB  (0x08060000)   ← scratch
#
# MCUboot swap mode requires equal-size slot sectors. Sectors 5-7 (128 KB each)
# satisfy this constraint. The bootloader occupies sectors 0-4 (128 KB total).
#
# Layout (total 512 KB):
#   Bootloader : 0x08000000  128 KB  (sectors 0-4)
#   Primary    : 0x08020000  128 KB  (sector 5)
#   Secondary  : 0x08040000  128 KB  (sector 6)
#   Scratch    : 0x08060000  128 KB  (sector 7)

include_guard(GLOBAL)

# ── Base address ──────────────────────────────────────────────────────────────
set(ALLOY_FLASH_BASE          "0x08000000" CACHE STRING "Internal flash base address")

# ── Bootloader region ─────────────────────────────────────────────────────────
set(ALLOY_BOOT_OFFSET         "0x08000000" CACHE STRING "MCUboot bootloader start address")
set(ALLOY_BOOT_SIZE           "0x20000"    CACHE STRING "MCUboot bootloader region size (128 KB)")

# ── Primary application slot ──────────────────────────────────────────────────
set(ALLOY_PRIMARY_OFFSET      "0x08020000" CACHE STRING "Primary slot start address")
set(ALLOY_PRIMARY_SIZE        "0x20000"    CACHE STRING "Primary slot size (128 KB)")

# ── Secondary (upgrade) slot ──────────────────────────────────────────────────
set(ALLOY_SECONDARY_OFFSET    "0x08040000" CACHE STRING "Secondary slot start address")
set(ALLOY_SECONDARY_SIZE      "0x20000"    CACHE STRING "Secondary slot size (128 KB)")

# ── Swap scratch area ─────────────────────────────────────────────────────────
set(ALLOY_SCRATCH_OFFSET      "0x08060000" CACHE STRING "Scratch area start address")
set(ALLOY_SCRATCH_SIZE        "0x20000"    CACHE STRING "Scratch area size (128 KB)")

# ── imgtool / MCUboot build parameters ───────────────────────────────────────
set(ALLOY_MCUBOOT_HEADER_SIZE "0x200"      CACHE STRING "MCUboot image header size")
set(ALLOY_MCUBOOT_ALIGN       "8"          CACHE STRING "MCUboot write alignment")
set(ALLOY_MCUBOOT_SLOT_SIZE   "${ALLOY_PRIMARY_SIZE}" CACHE STRING "Slot size passed to imgtool")
