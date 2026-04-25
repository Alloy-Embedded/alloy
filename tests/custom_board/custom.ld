/* Placeholder linker script for the ALLOY_BOARD=custom configure smoke test.
 * The test does not link a firmware image; the runtime only requires the file
 * to exist. A real consuming project supplies a target-specific linker script.
 */
ENTRY(_start)
MEMORY {
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 64K
    RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 16K
}
