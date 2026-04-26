// FatFS diskio bridge — compiled once as part of alloy-fatfs CMake target.
//
// Routes disk_initialize / disk_status / disk_read / disk_write / disk_ioctl
// to the function pointers registered by FatfsBackend<Device>::register_diskio().
// Only physical drive 0 is supported (FF_VOLUMES=1).

// ff.h must come first — it defines BYTE, DSTATUS, DRESULT, LBA_t, UINT, etc.
// diskio.h uses these types without including ff.h itself.
#include <ff.h>
#include <diskio.h>

#include "drivers/filesystem/fatfs/fatfs_backend.hpp"

extern "C" {

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0 || !alloy::drivers::filesystem::fatfs::detail::g_disk_io.initialize) {
        return STA_NOINIT;
    }
    return alloy::drivers::filesystem::fatfs::detail::g_disk_io.initialize();
}

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0 || !alloy::drivers::filesystem::fatfs::detail::g_disk_io.status) {
        return STA_NOINIT;
    }
    return alloy::drivers::filesystem::fatfs::detail::g_disk_io.status();
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || !alloy::drivers::filesystem::fatfs::detail::g_disk_io.read) {
        return RES_NOTRDY;
    }
    return alloy::drivers::filesystem::fatfs::detail::g_disk_io.read(buff, sector, count);
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || !alloy::drivers::filesystem::fatfs::detail::g_disk_io.write) {
        return RES_NOTRDY;
    }
    return alloy::drivers::filesystem::fatfs::detail::g_disk_io.write(buff, sector, count);
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    if (pdrv != 0 || !alloy::drivers::filesystem::fatfs::detail::g_disk_io.ioctl) {
        return RES_NOTRDY;
    }
    return alloy::drivers::filesystem::fatfs::detail::g_disk_io.ioctl(cmd, buff);
}

DWORD get_fattime(void) {
    // Returns 0 when no RTC is available. FatFS accepts this value.
    return 0;
}

}  // extern "C"
