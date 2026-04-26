#pragma once

// FatFS backend for alloy::hal::filesystem::Filesystem<>.
//
// Wraps Elm-Chan FatFS (http://elm-chan.org/fsw/ff/, BSD license) behind the
// Backend interface expected by Filesystem<Backend> and File<Backend>.
//
// Compile-time configuration (set by CMakeLists):
//   FF_FS_TINY   = 1  — tiny footprint mode (sector buffer in file object)
//   FF_FS_REENTRANT = 0  — no OS sync primitives (single-task bare-metal)
//   FF_USE_LFN   = 2  — LFN on stack
//   FF_VOLUMES   = 1  — one physical drive
//
// Disk I/O registration:
//   FatFS requires diskio.h callback implementations. This backend stores a
//   static pointer to itself and the diskio.cpp translation unit (compiled
//   from drivers/filesystem/fatfs/fatfs_diskio.cpp in the alloy-fatfs CMake
//   target) routes all calls through it. Only one FatfsBackend instance may
//   exist at a time; a second instantiation overwrites the pointer.
//
// Usage:
//   SdCard<SpiHandle> sd{spi};
//   FatfsBackend<decltype(sd)> fatfs_b{sd};
//   Filesystem<FatfsBackend<decltype(sd)>> fs{std::move(fatfs_b)};
//   fs.mount().unwrap();
//
// Requires:
//   CMake target alloy-fatfs linked (compiles ff.c, ffunicode.c,
//   and fatfs_diskio.cpp from the fetched source + this directory).

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

// FatFS headers — available after alloy-fatfs FetchContent.
#include <ff.h>
#include <diskio.h>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/filesystem/block_device.hpp"
#include "hal/filesystem/filesystem.hpp"

namespace alloy::drivers::filesystem::fatfs {

namespace detail {

inline auto fresult_to_error_code(FRESULT fr) -> alloy::core::ErrorCode {
    switch (fr) {
        case FR_OK:                  return alloy::core::ErrorCode::Ok;
        case FR_DISK_ERR:            return alloy::core::ErrorCode::CommunicationError;
        case FR_INT_ERR:             return alloy::core::ErrorCode::HardwareError;
        case FR_NOT_READY:           return alloy::core::ErrorCode::HardwareError;
        case FR_NO_FILE:             return alloy::core::ErrorCode::InvalidParameter;
        case FR_NO_PATH:             return alloy::core::ErrorCode::InvalidParameter;
        case FR_INVALID_NAME:        return alloy::core::ErrorCode::InvalidParameter;
        case FR_DENIED:              return alloy::core::ErrorCode::InvalidParameter;
        case FR_EXIST:               return alloy::core::ErrorCode::InvalidParameter;
        case FR_INVALID_OBJECT:      return alloy::core::ErrorCode::InvalidParameter;
        case FR_WRITE_PROTECTED:     return alloy::core::ErrorCode::NotSupported;
        case FR_INVALID_DRIVE:       return alloy::core::ErrorCode::InvalidParameter;
        case FR_NOT_ENABLED:         return alloy::core::ErrorCode::HardwareError;
        case FR_NO_FILESYSTEM:       return alloy::core::ErrorCode::HardwareError;
        case FR_MKFS_ABORTED:        return alloy::core::ErrorCode::HardwareError;
        case FR_TIMEOUT:             return alloy::core::ErrorCode::Timeout;
        case FR_LOCKED:              return alloy::core::ErrorCode::HardwareError;
        case FR_NOT_ENOUGH_CORE:     return alloy::core::ErrorCode::HardwareError;
        case FR_TOO_MANY_OPEN_FILES: return alloy::core::ErrorCode::HardwareError;
        default:                     return alloy::core::ErrorCode::HardwareError;
    }
}

inline auto open_mode_to_fatfs(alloy::hal::filesystem::OpenMode mode) -> BYTE {
    using OpenMode = alloy::hal::filesystem::OpenMode;
    switch (mode) {
        case OpenMode::ReadOnly:  return FA_READ;
        case OpenMode::WriteOnly: return FA_WRITE | FA_OPEN_ALWAYS;
        case OpenMode::ReadWrite: return FA_READ | FA_WRITE | FA_OPEN_ALWAYS;
        case OpenMode::Append:    return FA_WRITE | FA_OPEN_ALWAYS;
        case OpenMode::Truncate:  return FA_WRITE | FA_CREATE_ALWAYS;
        default:                  return FA_READ;
    }
}

// Global diskio interface. The diskio.cpp translation unit calls these.
// Defined in fatfs_diskio.cpp (compiled as part of alloy-fatfs CMake target).
struct DiskIO {
    DSTATUS (*initialize)() = nullptr;
    DSTATUS (*status)()     = nullptr;
    DRESULT (*read)(BYTE* buf, LBA_t sector, UINT count)  = nullptr;
    DRESULT (*write)(const BYTE* buf, LBA_t sector, UINT count) = nullptr;
    DRESULT (*ioctl)(BYTE cmd, void* buf) = nullptr;
};

// Defined as an inline variable (C++17); the diskio.cpp extern-declares it.
inline DiskIO g_disk_io{};

}  // namespace detail

// ── FatfsBackend<Device> ─────────────────────────────────────────────────────

template <hal::filesystem::BlockDevice Device>
class FatfsBackend {
public:
    struct FileHandle {
        FIL  fil{};
        bool open{false};
        hal::filesystem::OpenMode mode{};
    };

    explicit FatfsBackend(Device& device) : device_(&device) {
        register_diskio();
    }

    // ── Backend interface ────────────────────────────────────────────────────

    [[nodiscard]] auto mount() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (FRESULT fr = f_mount(&fatfs_, "", 1); fr != FR_OK) {
            return alloy::core::Err(detail::fresult_to_error_code(fr));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto format() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        alignas(4) std::array<std::uint8_t, FF_MAX_SS> work{};
        MKFS_PARM opt{.fmt = FM_FAT32, .n_fat = 1, .align = 0, .n_root = 0, .au_size = 0};
        if (FRESULT fr = f_mkfs("", &opt, work.data(), work.size()); fr != FR_OK) {
            return alloy::core::Err(detail::fresult_to_error_code(fr));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto open(std::string_view path,
                            hal::filesystem::OpenMode mode)
        -> alloy::core::Result<FileHandle, alloy::core::ErrorCode> {
        char buf[hal::filesystem::DirEntry::kMaxNameLen + 1]{};
        const std::size_t n = std::min(path.size(), sizeof(buf) - 1);
        std::memcpy(buf, path.data(), n);
        buf[n] = '\0';

        FileHandle fh{};
        fh.mode = mode;
        if (FRESULT fr = f_open(&fh.fil, buf, detail::open_mode_to_fatfs(mode));
            fr != FR_OK) {
            return alloy::core::Err(detail::fresult_to_error_code(fr));
        }
        // Seek to end for Append mode.
        if (mode == hal::filesystem::OpenMode::Append) {
            f_lseek(&fh.fil, f_size(&fh.fil));
        }
        fh.open = true;
        return alloy::core::Ok(std::move(fh));
    }

    [[nodiscard]] auto read(FileHandle& fh, std::span<std::byte> buf)
        -> alloy::core::Result<std::size_t, alloy::core::ErrorCode> {
        UINT br = 0;
        if (FRESULT fr = f_read(&fh.fil, buf.data(),
                                static_cast<UINT>(buf.size()), &br);
            fr != FR_OK) {
            return alloy::core::Err(detail::fresult_to_error_code(fr));
        }
        return alloy::core::Ok(static_cast<std::size_t>(br));
    }

    [[nodiscard]] auto write(FileHandle& fh, std::span<const std::byte> data)
        -> alloy::core::Result<std::size_t, alloy::core::ErrorCode> {
        UINT bw = 0;
        if (FRESULT fr = f_write(&fh.fil, data.data(),
                                  static_cast<UINT>(data.size()), &bw);
            fr != FR_OK) {
            return alloy::core::Err(detail::fresult_to_error_code(fr));
        }
        return alloy::core::Ok(static_cast<std::size_t>(bw));
    }

    [[nodiscard]] auto seek(FileHandle& fh,
                            std::int64_t offset,
                            hal::filesystem::SeekOrigin origin)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        FSIZE_t target{};
        switch (origin) {
            case hal::filesystem::SeekOrigin::Begin:
                target = static_cast<FSIZE_t>(offset);
                break;
            case hal::filesystem::SeekOrigin::Current:
                target = f_tell(&fh.fil) + static_cast<FSIZE_t>(offset);
                break;
            case hal::filesystem::SeekOrigin::End:
                target = f_size(&fh.fil) + static_cast<FSIZE_t>(offset);
                break;
        }
        if (FRESULT fr = f_lseek(&fh.fil, target); fr != FR_OK) {
            return alloy::core::Err(detail::fresult_to_error_code(fr));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto tell(const FileHandle& fh) const
        -> alloy::core::Result<std::int64_t, alloy::core::ErrorCode> {
        return alloy::core::Ok(static_cast<std::int64_t>(f_tell(&fh.fil)));
    }

    [[nodiscard]] auto close(FileHandle& fh)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        if (!fh.open) return alloy::core::Ok();
        fh.open = false;
        if (FRESULT fr = f_close(&fh.fil); fr != FR_OK) {
            return alloy::core::Err(detail::fresult_to_error_code(fr));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto remove(std::string_view path)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        char buf[hal::filesystem::DirEntry::kMaxNameLen + 1]{};
        std::memcpy(buf, path.data(), std::min(path.size(), sizeof(buf) - 1));
        if (FRESULT fr = f_unlink(buf); fr != FR_OK) {
            return alloy::core::Err(detail::fresult_to_error_code(fr));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto rename(std::string_view old_path, std::string_view new_path)
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        char obuf[hal::filesystem::DirEntry::kMaxNameLen + 1]{};
        char nbuf[hal::filesystem::DirEntry::kMaxNameLen + 1]{};
        std::memcpy(obuf, old_path.data(), std::min(old_path.size(), sizeof(obuf) - 1));
        std::memcpy(nbuf, new_path.data(), std::min(new_path.size(), sizeof(nbuf) - 1));
        if (FRESULT fr = f_rename(obuf, nbuf); fr != FR_OK) {
            return alloy::core::Err(detail::fresult_to_error_code(fr));
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto stat(std::string_view path)
        -> alloy::core::Result<hal::filesystem::DirEntry, alloy::core::ErrorCode> {
        char buf[hal::filesystem::DirEntry::kMaxNameLen + 1]{};
        std::memcpy(buf, path.data(), std::min(path.size(), sizeof(buf) - 1));
        FILINFO fno{};
        if (FRESULT fr = f_stat(buf, &fno); fr != FR_OK) {
            return alloy::core::Err(detail::fresult_to_error_code(fr));
        }
        hal::filesystem::DirEntry entry{};
        std::memcpy(entry.name, fno.fname, sizeof(entry.name) - 1);
        entry.size   = static_cast<std::size_t>(fno.fsize);
        entry.is_dir = (fno.fattrib & AM_DIR) != 0;
        return alloy::core::Ok(std::move(entry));
    }

private:
    void register_diskio() {
        detail::g_disk_io.initialize = []() -> DSTATUS {
            return (s_device_ != nullptr) ? 0 : STA_NOINIT;
        };
        detail::g_disk_io.status = []() -> DSTATUS {
            return (s_device_ != nullptr) ? 0 : STA_NOINIT;
        };
        detail::g_disk_io.read = [](BYTE* buf, LBA_t sector, UINT count) -> DRESULT {
            if (s_device_ == nullptr) return RES_NOTRDY;
            const std::size_t blk_sz = s_device_->block_size();
            for (UINT i = 0; i < count; ++i) {
                auto span = std::span<std::byte>{
                    reinterpret_cast<std::byte*>(buf) + i * blk_sz, blk_sz};
                if (auto r = s_device_->read(static_cast<std::size_t>(sector + i), span);
                    r.is_err()) {
                    return RES_ERROR;
                }
            }
            return RES_OK;
        };
        detail::g_disk_io.write = [](const BYTE* buf, LBA_t sector, UINT count) -> DRESULT {
            if (s_device_ == nullptr) return RES_NOTRDY;
            const std::size_t blk_sz = s_device_->block_size();
            for (UINT i = 0; i < count; ++i) {
                auto span = std::span<const std::byte>{
                    reinterpret_cast<const std::byte*>(buf) + i * blk_sz, blk_sz};
                if (auto r = s_device_->write(static_cast<std::size_t>(sector + i), span);
                    r.is_err()) {
                    return RES_ERROR;
                }
            }
            return RES_OK;
        };
        detail::g_disk_io.ioctl = [](BYTE cmd, void* buf) -> DRESULT {
            if (s_device_ == nullptr) return RES_NOTRDY;
            switch (cmd) {
                case CTRL_SYNC:
                    return RES_OK;
                case GET_SECTOR_COUNT: {
                    *reinterpret_cast<LBA_t*>(buf) =
                        static_cast<LBA_t>(s_device_->block_count());
                    return RES_OK;
                }
                case GET_SECTOR_SIZE: {
                    *reinterpret_cast<WORD*>(buf) =
                        static_cast<WORD>(s_device_->block_size());
                    return RES_OK;
                }
                case GET_BLOCK_SIZE:
                    *reinterpret_cast<DWORD*>(buf) = 1;
                    return RES_OK;
                default:
                    return RES_PARERR;
            }
        };
        s_device_ = device_;
    }

    Device*  device_;
    FATFS    fatfs_{};

    // One device at a time (diskio is a global C interface).
    inline static Device* s_device_{nullptr};
};

}  // namespace alloy::drivers::filesystem::fatfs
