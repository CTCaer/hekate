/*
 * Copyright (c) 2019 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MEMORY_MAP_H_
#define _MEMORY_MAP_H_

//#define IPL_STACK_TOP  0x4003FF00
/* --- BIT/BCT: 0x40000000 - 0x40003000 --- */
/* ---     IPL: 0x40008000 - 0x40028000 --- */
#define IPL_LOAD_ADDR     0x40008000
#define  IPL_SZ_MAX          0x20000 // 128KB.
//#define IRAM_LIB_ADDR     0x4002B000
#define SDRAM_PARAMS_ADDR 0x40030000 // SDRAM extraction buffer during sdram init.
#define CBFS_DRAM_EN_ADDR 0x4003e000 // u32.

/* --- DRAM START --- */
#define DRAM_START     0x80000000
/* Do not write anything in this area */
#define NYX_LOAD_ADDR  0x81000000
#define  NYX_SZ_MAX     0x1000000
/* Stack theoretical max: 220MB */
#define IPL_STACK_TOP  0x90010000
#define IPL_HEAP_START 0x90020000
#define  IPL_HEAP_SZ   0x24FE0000 // 592MB.
/* --- Gap: 0xB5000000 - 0xB5FFFFFF --- */

// SDMMC DMA buffers
#define SDXC_BUF_ALIGNED   0xB6000000
#define MIXD_BUF_ALIGNED   0xB7000000
#define EMMC_BUF_ALIGNED   MIXD_BUF_ALIGNED
#define  SDMMC_DMA_BUF_SZ   0x1000000 // 16MB (4MB currently used).
#define SDMMC_UPPER_BUFFER 0xB8000000
#define  SDMMC_UP_BUF_SZ    0x8000000 // 128MB.

// Virtual disk / Chainloader buffers.
#define RAM_DISK_ADDR     0xC1000000
#define  RAM_DISK_SZ      0x20000000
//#define DRAM_LIB_ADDR    0xE0000000
/* --- Chnldr: 252MB 0xC03C0000 - 0xCFFFFFFF --- */ //! Only used when chainloading.
/* ---    Gap: 464MB 0xD0000000 - 0xECFFFFFF --- */

// Nyx buffers.
#define NYX_STORAGE_ADDR 0xED000000
#define NYX_RES_ADDR     0xEE000000

// Framebuffer addresses.
#define IPL_FB_ADDRESS   0xF0000000
#define  IPL_FB_SZ         0x384000 // 720 x 1280 x 4.
#define LOG_FB_ADDRESS   0xF0400000
#define  LOG_FB_SZ         0x334000 // 1280 x 656 x 4.
#define NYX_FB_ADDRESS   0xF0800000
#define  NYX_FB_SZ         0x384000 // 1280 x 720 x 4.

// Nyx LvGL buffers.
#define NYX_LV_VDB_ADR   0xF0C00000
#define  NYX_FB_SZ         0x384000 // 1280 x 720 x 4.
#define NYX_LV_MEM_ADR   0xF1000000
#define  NYX_LV_MEM_SZ    0x8000000

// NX BIS driver sector cache.
#define NX_BIS_CACHE_ADDR 0xF9000000
#define  NX_BIS_CACHE_SZ      0x8800
/* --- Gap: 111MB 0xF9008800 - 0xFFFFFFFF --- */

// #define EXT_PAYLOAD_ADDR    0xC03C0000
// #define RCM_PAYLOAD_ADDR    (EXT_PAYLOAD_ADDR + ALIGN(PATCHED_RELOC_SZ, 0x10))
// #define COREBOOT_ADDR       (0xD0000000 - 0x100000)

// NYX
// #define EXT_PAYLOAD_ADDR    0xC0000000
// #define RCM_PAYLOAD_ADDR    (EXT_PAYLOAD_ADDR + ALIGN(PATCHED_RELOC_SZ, 0x10))
// #define COREBOOT_ADDR       (0xD0000000 - 0x100000)

#endif
