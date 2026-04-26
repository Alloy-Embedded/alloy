/* ffconf.h — FatFS configuration for alloy (bare-metal, single-task) */
#pragma once

/* Revision ID — must match the FatFS source revision */
#define FFCONF_DEF 80286

/* Filesystem type: FAT12/16/32 only (no exFAT for code-size) */
#define FF_FS_EXFAT 0

/* Tiny mode: file data buffer not inside FIL (saves RAM when file count > 1) */
#define FF_FS_TINY 1

/* No OS-level re-entrancy (single-task bare-metal) */
#define FF_FS_REENTRANT 0

/* Long filename support via stack buffer (no heap) */
#define FF_USE_LFN 2
#define FF_MAX_LFN 255

/* Code page: US ASCII + Latin-1 (most common for embedded) */
#define FF_CODE_PAGE 437

/* Sector size: 512 bytes (SD card native) */
#define FF_MIN_SS 512
#define FF_MAX_SS 512

/* Number of volumes (physical drives) */
#define FF_VOLUMES 1

/* String functions */
#define FF_USE_STRFUNC 1
#define FF_PRINT_LLI   1
#define FF_PRINT_FLOAT 0

/* mkfs support (needed for format()) */
#define FF_USE_MKFS 1

/* Directory operations */
#define FF_USE_FIND   0
#define FF_USE_CHMOD  0
#define FF_USE_EXPAND 0
#define FF_USE_LABEL  0

/* Forward function (not needed) */
#define FF_USE_FORWARD 0

/* Timestamp: no RTC — always returns 0 */
#define FF_FS_NORTC    1
#define FF_NORTC_MON   1
#define FF_NORTC_MDAY  1
#define FF_NORTC_YEAR  2024

/* Lock: no OS sync */
#define FF_FS_LOCK 0
