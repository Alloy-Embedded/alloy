# boards/nrf52840_dk/mcuboot/memory_map.cmake — PLACEHOLDER
#
# MCUboot partition layout for nRF52840 DK.
# Status: deferred until nRF52840 bring-up is complete (add-nrf52840-bringup spec).
#
# nRF52840 flash: 1 MB at 0x00000000, 4 KB pages.
# Preliminary layout (not yet validated in hardware):
#
#   Bootloader : 0x00000000   48 KB  (12 pages — MCUboot on Cortex-M4)
#   Primary    : 0x0000C000  456 KB  (114 pages)
#   Secondary  : 0x0007E000  456 KB  (114 pages)
#   Scratch    : 0x000F0000   64 KB  (16 pages)
#
# These values will be confirmed and activated when nRF52840 bring-up lands.
# Do NOT enable ALLOY_MCUBOOT=ON for nrf52840_dk until that spec is merged.

include_guard(GLOBAL)

message(STATUS
    "alloy: MCUboot memory map for nrf52840_dk is a placeholder. "
    "Do not use ALLOY_MCUBOOT=ON until nRF52840 bring-up is complete."
)

set(ALLOY_FLASH_BASE          "0x00000000" CACHE STRING "Internal flash base address (nRF52840)")
set(ALLOY_BOOT_OFFSET         "0x00000000" CACHE STRING "MCUboot bootloader start (placeholder)")
set(ALLOY_BOOT_SIZE           "0xC000"     CACHE STRING "Bootloader size (48 KB, placeholder)")
set(ALLOY_PRIMARY_OFFSET      "0x0000C000" CACHE STRING "Primary slot start (placeholder)")
set(ALLOY_PRIMARY_SIZE        "0x72000"    CACHE STRING "Primary slot size (456 KB, placeholder)")
set(ALLOY_SECONDARY_OFFSET    "0x0007E000" CACHE STRING "Secondary slot start (placeholder)")
set(ALLOY_SECONDARY_SIZE      "0x72000"    CACHE STRING "Secondary slot size (456 KB, placeholder)")
set(ALLOY_SCRATCH_OFFSET      "0x000F0000" CACHE STRING "Scratch area start (placeholder)")
set(ALLOY_SCRATCH_SIZE        "0x10000"    CACHE STRING "Scratch size (64 KB, placeholder)")
set(ALLOY_MCUBOOT_HEADER_SIZE "0x200"      CACHE STRING "MCUboot image header size")
set(ALLOY_MCUBOOT_ALIGN       "4"          CACHE STRING "MCUboot write alignment (nRF52840)")
set(ALLOY_MCUBOOT_SLOT_SIZE   "${ALLOY_PRIMARY_SIZE}" CACHE STRING "Slot size for imgtool")
