# boards/same70_xplained/mcuboot/memory_map.cmake
#
# MCUboot partition layout for SAME70 Xplained Ultra (ATSAME70Q21B)
# Internal flash: 2 MB at 0x00400000, uniform 8 KB pages.
# External flash: W25Q128JV (16 MB) via QSPI — hosts the secondary slot.
#
# Architecture rationale:
#   Primary slot on internal flash so MCUboot can execute and SHA-256-verify
#   the image before booting, without needing the W25Q driver at boot time.
#   Secondary slot on W25Q: the OTA client writes the download there via the
#   W25QFlashBackend driver. MCUboot reads it via the external-flash HAL shim.
#   Scratch: SRAM-backed (256 KB ITCM SRAM at 0x00000000); scratch does not
#   need to be persistent across resets for overwrite-mode OTA.
#
# Layout:
#   Bootloader : 0x00400000  256 KB  (on internal flash)
#   Primary    : 0x00440000    1 MB  (on internal flash)
#   Secondary  : 0x00000000    1 MB  (offset in W25Q QSPI flash, not MCU addr)
#   Scratch    : 0x00000000  256 KB  (ITCM SRAM — set ALLOY_MCUBOOT_SCRATCH_IN_RAM=ON)

include_guard(GLOBAL)

# ── Internal flash base ───────────────────────────────────────────────────────
set(ALLOY_FLASH_BASE              "0x00400000" CACHE STRING "Internal flash base address (SAME70)")

# ── Bootloader region (internal flash) ───────────────────────────────────────
set(ALLOY_BOOT_OFFSET             "0x00400000" CACHE STRING "MCUboot bootloader start address")
set(ALLOY_BOOT_SIZE               "0x40000"    CACHE STRING "MCUboot bootloader region size (256 KB)")

# ── Primary application slot (internal flash) ─────────────────────────────────
set(ALLOY_PRIMARY_OFFSET          "0x00440000" CACHE STRING "Primary slot start address (internal)")
set(ALLOY_PRIMARY_SIZE            "0x100000"   CACHE STRING "Primary slot size (1 MB)")

# ── Secondary slot (W25Q external QSPI flash) ────────────────────────────────
# Addressed as an offset within the W25Q flash, not as a CPU address.
# The MCUboot external-flash shim reads this via the QSPI HAL.
set(ALLOY_SECONDARY_OFFSET        "0x00000000" CACHE STRING "Secondary slot offset in W25Q flash")
set(ALLOY_SECONDARY_SIZE          "0x100000"   CACHE STRING "Secondary slot size (1 MB)")
set(ALLOY_SECONDARY_ON_EXT_FLASH  ON           CACHE BOOL   "Secondary slot resides on external flash")

# ── Scratch (SRAM — not persisted across resets) ──────────────────────────────
# MCUboot overwrite mode does not need a persistent scratch region; the ITCM
# SRAM is large enough (256 KB) to hold the swap buffer for 1 MB images.
set(ALLOY_SCRATCH_IN_RAM          ON           CACHE BOOL   "Use SRAM as MCUboot scratch")
set(ALLOY_SCRATCH_SIZE            "0x40000"    CACHE STRING "Scratch size (256 KB)")

# ── imgtool / MCUboot build parameters ───────────────────────────────────────
set(ALLOY_MCUBOOT_HEADER_SIZE "0x200"      CACHE STRING "MCUboot image header size")
set(ALLOY_MCUBOOT_ALIGN       "8"          CACHE STRING "MCUboot write alignment")
set(ALLOY_MCUBOOT_SLOT_SIZE   "${ALLOY_PRIMARY_SIZE}" CACHE STRING "Slot size passed to imgtool")
