/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 CTCaer
 * Copyright (c) 2018 Atmosphère-NX
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

#include <string.h>

#include "pkg2.h"
#include "pkg2_ini_kippatch.h"

#include "../config/config.h"
#include "../libs/fatfs/ff.h"
#include "../utils/aarch64_util.h"
#include "../mem/heap.h"
#include "../sec/se.h"
#include "../storage/emummc.h"
#include "../libs/compr/blz.h"

#include "../gfx/gfx.h"

extern hekate_config h_cfg;
extern void *sd_file_read(const char *path, u32 *fsize);

#ifdef KIP1_PATCH_DEBUG
	#include "../utils/util.h"
	#define DPRINTF(...) gfx_printf(__VA_ARGS__)
	#define DEBUG_PRINTING
#else
	#define DPRINTF(...)
#endif

//TODO: Replace hardcoded AArch64 instructions with instruction macros.
//TODO: Reduce hardcoded values without searching kernel for patterns?
// The process ID send/receive kernel patches were taken from Atmosphère's kernel patches.
// They should only be used when running Atmosphère.
#define FREE_CODE_OFF_1ST_100 0x4797C
#define FREE_CODE_OFF_1ST_200 0x6486C
#define FREE_CODE_OFF_1ST_300 0x494A4
#define FREE_CODE_OFF_1ST_302 0x494BC
#define FREE_CODE_OFF_1ST_400 0x52890
#define FREE_CODE_OFF_1ST_500 0x5C020
#define FREE_CODE_OFF_1ST_600 0x5EE00
#define FREE_CODE_OFF_1ST_700 0x5FEC0
#define FREE_CODE_OFF_1ST_800 0x607F0

#define ID_SND_OFF_100 0x23CC0
#define ID_SND_OFF_200 0x3F134
#define ID_SND_OFF_300 0x26080
#define ID_SND_OFF_302 0x26080
#define ID_SND_OFF_400 0x2AF64
#define ID_SND_OFF_500 0x2AD34
#define ID_SND_OFF_600 0x2BB8C
#define ID_SND_OFF_700 0x2D044
#define ID_SND_OFF_800 0x2F1FC

#define ID_RCV_OFF_100 0x219F0
#define ID_RCV_OFF_200 0x3D1A8
#define ID_RCV_OFF_300 0x240F0
#define ID_RCV_OFF_302 0x240F0
#define ID_RCV_OFF_400 0x28F6C
#define ID_RCV_OFF_500 0x28DAC
#define ID_RCV_OFF_600 0x29B6C
#define ID_RCV_OFF_700 0x2B23C
#define ID_RCV_OFF_800 0x2D424

static u32 PRC_ID_SND_100[] =
{
	0xA9BF2FEA, 0x2A0E03EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B,
	0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9412948,	0xA8C12FEA
};
#define FREE_CODE_OFF_2ND_100 (FREE_CODE_OFF_1ST_100 + sizeof(PRC_ID_SND_100) + sizeof(u32))
static u32 PRC_ID_RCV_100[] =
{
	0xA9BF2FEA, 0x2A1C03EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A,
	0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9412968, 0xA8C12FEA
};

static u32 PRC_ID_SND_200[] =
{
	0xA9BF2FEA, 0x2A1803EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B,
	0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9413148, 0xA8C12FEA
};
#define FREE_CODE_OFF_2ND_200 (FREE_CODE_OFF_1ST_200 + sizeof(PRC_ID_SND_200) + sizeof(u32))
static u32 PRC_ID_RCV_200[] =
{
	0xA9BF2FEA, 0x2A0F03EA, 0xD37EF54A, 0xF9405FEB, 0xF86A696A, 0xF9407BEB, 0x92FFFFE9, 0x8A090148,
	0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9413168, 0xA8C12FEA
};

static u32 PRC_ID_SND_300[] =
{
	0xA9BF2FEA, 0x2A1803EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B,
	0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9415548, 0xA8C12FEA
};
#define FREE_CODE_OFF_2ND_300 (FREE_CODE_OFF_1ST_300 + sizeof(PRC_ID_SND_300) + sizeof(u32))
static u32 PRC_ID_RCV_300[] =
{
	0xA9BF2FEA, 0x2A0F03EA, 0xD37EF54A, 0xF9405FEB, 0xF86A696A, 0xF9407BEB, 0x92FFFFE9, 0x8A090148,
	0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415568, 0xA8C12FEA
};

#define FREE_CODE_OFF_2ND_302 (FREE_CODE_OFF_1ST_302 + sizeof(PRC_ID_SND_300) + sizeof(u32))

static u32 PRC_ID_SND_400[] =
{
	0x2A1703EA, 0xD37EF54A, 0xF86A6B8A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9,
	0xEB09015F, 0x54000060, 0xF94053EA, 0xF9415948, 0xF94053EA
};
#define FREE_CODE_OFF_2ND_400 (FREE_CODE_OFF_1ST_400 + sizeof(PRC_ID_SND_400) + sizeof(u32))
static u32 PRC_ID_RCV_400[] =
{
	0xF9403BED, 0x2A0E03EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A,
	0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415B28, 0xD503201F
};

static u32 PRC_ID_SND_500[] =
{
	0x2A1703EA, 0xD37EF54A, 0xF86A6B6A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9,
	0xEB09015F, 0x54000060, 0xF94043EA, 0xF9415948, 0xF94043EA
};
#define FREE_CODE_OFF_2ND_500 (FREE_CODE_OFF_1ST_500 + sizeof(PRC_ID_SND_500) + sizeof(u32))
static u32 PRC_ID_RCV_500[] =
{
	0xF9403BED, 0x2A1503EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A,
	0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415B08, 0xF9406FEA
};

static u32 PRC_ID_SND_600[] =
{
	0xA9BF2FEA, 0xF94037EB, 0x2A1503EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400308, 0xF9401D08, 0xAA1803E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};
#define FREE_CODE_OFF_2ND_600 (FREE_CODE_OFF_1ST_600 + sizeof(PRC_ID_SND_600) + sizeof(u32))
static u32 PRC_ID_RCV_600[] =
{
	0xA9BF2FEA, 0xF94043EB, 0x2A1503EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400308, 0xF9401D08, 0xAA1803E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};

static u32 PRC_ID_SND_700[] =
{
	0xA9BF2FEA, 0xF9403BEB, 0x2A1903EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF94002A8, 0xF9401D08, 0xAA1503E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};
#define FREE_CODE_OFF_2ND_700 (FREE_CODE_OFF_1ST_700 + sizeof(PRC_ID_SND_700) + sizeof(u32))
static u32 PRC_ID_RCV_700[] =
{
	0xA9BF2FEA, 0xF9404FEB, 0x2A1603EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400368, 0xF9401D08, 0xAA1B03E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};

#define FREE_CODE_OFF_2ND_800 (FREE_CODE_OFF_1ST_800 + sizeof(PRC_ID_SND_700) + sizeof(u32))

// Include kernel patches here, so we can utilize pkg1 id
KERNEL_PATCHSET_DEF(_kernel_1_patchset,
	{ SVC_VERIFY_DS, 0x3764C, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x44074, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_100, _B(ID_SND_OFF_100, FREE_CODE_OFF_1ST_100), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_100, sizeof(PRC_ID_SND_100) >> 2, PRC_ID_SND_100}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_100 + sizeof(PRC_ID_SND_100),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_1ST_100 + sizeof(PRC_ID_SND_100), ID_SND_OFF_100 + sizeof(u32)), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_100, _B(ID_RCV_OFF_100, FREE_CODE_OFF_2ND_100), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_100, sizeof(PRC_ID_RCV_100) >> 2, PRC_ID_RCV_100}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_100 + sizeof(PRC_ID_RCV_100),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_2ND_100 + sizeof(PRC_ID_RCV_100), ID_RCV_OFF_100 + sizeof(u32)), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_2_patchset,
	{ SVC_VERIFY_DS, 0x54834, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x6086C, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_200, _B(ID_SND_OFF_200, FREE_CODE_OFF_1ST_200), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_200, sizeof(PRC_ID_SND_200) >> 2, PRC_ID_SND_200}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_200 + sizeof(PRC_ID_SND_200),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_1ST_200 + sizeof(PRC_ID_SND_200), ID_SND_OFF_200 + sizeof(u32)), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_200, _B(ID_RCV_OFF_200, FREE_CODE_OFF_2ND_200), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_200, sizeof(PRC_ID_RCV_200) >> 2, PRC_ID_RCV_200}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_200 + sizeof(PRC_ID_RCV_200),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_2ND_200 + sizeof(PRC_ID_RCV_200), ID_RCV_OFF_200 + sizeof(u32)), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_3_patchset,
	{ SVC_VERIFY_DS, 0x3BD24, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x483FC, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_300, _B(ID_SND_OFF_300, FREE_CODE_OFF_1ST_300), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_300, sizeof(PRC_ID_SND_300) >> 2, PRC_ID_SND_300}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_300 + sizeof(PRC_ID_SND_300),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_1ST_300 + sizeof(PRC_ID_SND_300), ID_SND_OFF_300 + sizeof(u32)), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_300, _B(ID_RCV_OFF_300, FREE_CODE_OFF_2ND_300), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_300, sizeof(PRC_ID_RCV_300) >> 2, PRC_ID_RCV_300}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_300 + sizeof(PRC_ID_RCV_300),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_2ND_300 + sizeof(PRC_ID_RCV_300), ID_RCV_OFF_300 + sizeof(u32)), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_302_patchset,
	{ SVC_VERIFY_DS, 0x3BD24, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x48414, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_302, _B(ID_SND_OFF_302, FREE_CODE_OFF_1ST_302), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_302, sizeof(PRC_ID_SND_300) >> 2, PRC_ID_SND_300}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_302 + sizeof(PRC_ID_SND_300),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_1ST_302 + sizeof(PRC_ID_SND_300), ID_SND_OFF_302 + sizeof(u32)), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_302, _B(ID_RCV_OFF_302, FREE_CODE_OFF_2ND_302), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_302, sizeof(PRC_ID_RCV_300) >> 2, PRC_ID_RCV_300}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_302 + sizeof(PRC_ID_RCV_300),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_2ND_302 + sizeof(PRC_ID_RCV_300), ID_RCV_OFF_302 + sizeof(u32)), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_4_patchset,
	{ SVC_VERIFY_DS, 0x41EB4, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x4EBFC, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_400, _B(ID_SND_OFF_400, FREE_CODE_OFF_1ST_400), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_400, sizeof(PRC_ID_SND_400) >> 2, PRC_ID_SND_400}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_400 + sizeof(PRC_ID_SND_400),                      // Branch back and skip 2 instructions.
		_B(FREE_CODE_OFF_1ST_400 + sizeof(PRC_ID_SND_400), ID_SND_OFF_400 + sizeof(u32) * 2), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_400, _B(ID_RCV_OFF_400, FREE_CODE_OFF_2ND_400), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_400, sizeof(PRC_ID_RCV_400) >> 2, PRC_ID_RCV_400}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_400 + sizeof(PRC_ID_RCV_400),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_2ND_400 + sizeof(PRC_ID_RCV_400), ID_RCV_OFF_400 + sizeof(u32)), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_5_patchset,
	{ SVC_GENERIC,   0x38C2C, _NOP(), NULL },          // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x45E6C, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x5513C, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_500, _B(ID_SND_OFF_500, FREE_CODE_OFF_1ST_500), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_500, sizeof(PRC_ID_SND_500) >> 2, PRC_ID_SND_500}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_500 + sizeof(PRC_ID_SND_500),                      // Branch back and skip 2 instructions.
		_B(FREE_CODE_OFF_1ST_500 + sizeof(PRC_ID_SND_500), ID_SND_OFF_500 + sizeof(u32) * 2), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_500, _B(ID_RCV_OFF_500, FREE_CODE_OFF_2ND_500), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_500, sizeof(PRC_ID_RCV_500) >> 2, PRC_ID_RCV_500}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_500 + sizeof(PRC_ID_RCV_500),                      // Branch back and skip 2 instructions.
		_B(FREE_CODE_OFF_2ND_500 + sizeof(PRC_ID_RCV_500), ID_RCV_OFF_500 + sizeof(u32) * 2), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_6_patchset,
	{ SVC_GENERIC,   0x3A8CC, _NOP(), NULL },          // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x47EA0, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x57548, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_600, _B(ID_SND_OFF_600, FREE_CODE_OFF_1ST_600), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_600, sizeof(PRC_ID_SND_600) >> 2, PRC_ID_SND_600}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_600 + sizeof(PRC_ID_SND_600),                      // Branch back and skip 4 instructions.
		_B(FREE_CODE_OFF_1ST_600 + sizeof(PRC_ID_SND_600), ID_SND_OFF_600 + sizeof(u32) * 4), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_600, _B(ID_RCV_OFF_600, FREE_CODE_OFF_2ND_600), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_600, sizeof(PRC_ID_RCV_600) >> 2, PRC_ID_RCV_600}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_600 + sizeof(PRC_ID_RCV_600),                      // Branch back and skip 4 instructions.
		_B(FREE_CODE_OFF_2ND_600 + sizeof(PRC_ID_RCV_600), ID_RCV_OFF_600 + sizeof(u32) * 4), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_7_patchset,
	{ SVC_GENERIC,   0x3C6E0, _NOP(), NULL },          // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x49E5C, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x581B0, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_700, _B(ID_SND_OFF_700, FREE_CODE_OFF_1ST_700), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_700, sizeof(PRC_ID_SND_700) >> 2, PRC_ID_SND_700}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_700 + sizeof(PRC_ID_SND_700),                      // Branch back and skip 4 instructions.
		_B(FREE_CODE_OFF_1ST_700 + sizeof(PRC_ID_SND_700), ID_SND_OFF_700 + sizeof(u32) * 4), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_700, _B(ID_RCV_OFF_700, FREE_CODE_OFF_2ND_700), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_700, sizeof(PRC_ID_RCV_700) >> 2, PRC_ID_RCV_700}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_700 + sizeof(PRC_ID_RCV_700),                      // Branch back and skip 4 instructions.
		_B(FREE_CODE_OFF_2ND_700 + sizeof(PRC_ID_RCV_700), ID_RCV_OFF_700 + sizeof(u32) * 4), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_8_patchset,
	{ SVC_GENERIC,   0x3FAD0, _NOP(), NULL },          // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x4D15C, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x5BFAC, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_800, _B(ID_SND_OFF_800, FREE_CODE_OFF_1ST_800), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_800, sizeof(PRC_ID_SND_700) >> 2, PRC_ID_SND_700}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_800 + sizeof(PRC_ID_SND_700),                      // Branch back and skip 4 instructions.
		_B(FREE_CODE_OFF_1ST_800 + sizeof(PRC_ID_SND_700), ID_SND_OFF_800 + sizeof(u32) * 4), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_800, _B(ID_RCV_OFF_800, FREE_CODE_OFF_2ND_800), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_800, sizeof(PRC_ID_RCV_700) >> 2, PRC_ID_RCV_700}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_800 + sizeof(PRC_ID_RCV_700),                      // Branch back and skip 4 instructions.
		_B(FREE_CODE_OFF_2ND_800 + sizeof(PRC_ID_RCV_700), ID_RCV_OFF_800 + sizeof(u32) * 4), NULL}
);

// Kernel sha256 hashes.
static const pkg2_kernel_id_t _pkg2_kernel_ids[] =
{
	{ "\xb8\xc5\x0c\x68\x25\xa9\xb9\x5b", _kernel_1_patchset },   //1.0.0
	{ "\x64\x0b\x51\xff\x28\x01\xb8\x30", _kernel_2_patchset },   //2.0.0 - 2.3.0
	{ "\x50\x84\x23\xac\x6f\xa1\x5d\x3b", _kernel_3_patchset },   //3.0.0 - 3.0.1
	{ "\x81\x9d\x08\xbe\xe4\x5e\x1f\xbb", _kernel_302_patchset }, //3.0.2
	{ "\xe6\xc0\xb7\xe3\x2f\xf9\x44\x51", _kernel_4_patchset },   //4.0.0 - 4.1.0
	{ "\xb2\x38\x61\xa8\xe1\xe2\xe4\xe4", _kernel_5_patchset },   //5.0.0 - 5.1.0
	{ "\x85\x97\x40\xf6\xc0\x3e\x3d\x44", _kernel_6_patchset },   //6.0.0 - 6.2.0
	{ "\xa2\x5e\x47\x0c\x8e\x6d\x2f\xd7", _kernel_7_patchset },   //7.0.0
	{ "\xf1\x5e\xc8\x34\xfd\x68\xf0\xf0", _kernel_8_patchset }    //8.0.0. Kernel only.
};

enum kip_offset_section
{
	KIP_TEXT    = 0,
	KIP_RODATA  = 1,
	KIP_DATA    = 2,
	KIP_BSS     = 3,
	KIP_UNKSEC1 = 4,
	KIP_UNKSEC2 = 5
};

#define KIP_PATCH_SECTION_SHIFT  (29)
#define KIP_PATCH_SECTION_MASK   (7 << KIP_PATCH_SECTION_SHIFT)
#define KIP_PATCH_OFFSET_MASK    (~KIP_PATCH_SECTION_MASK)
#define GET_KIP_PATCH_SECTION(x) ((x >> KIP_PATCH_SECTION_SHIFT) & 7)
#define GET_KIP_PATCH_OFFSET(x)  (x & KIP_PATCH_OFFSET_MASK)
#define KPS(x) ((u32)(x) << KIP_PATCH_SECTION_SHIFT)

static kip1_patch_t _fs_emummc[] =
{
	{ KPS(KIP_TEXT) | 1, 0, "", "" },
	{ 0, 0, NULL, NULL }
};

static kip1_patchset_t _fs_patches_100[] =
{
	{ "nogc",     NULL },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patchset_t _fs_patches_200[] =
{
	{ "nogc",     NULL },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patchset_t _fs_patches_210[] =
{
	{ "nogc",     NULL },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patchset_t _fs_patches_300[] =
{
	{ "nogc",     NULL },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patchset_t _fs_patches_30x[] =
{
	{ "nogc",     NULL },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patch_t _fs_nogc_40x[] =
{
	{ KPS(KIP_TEXT) | 0xA3458, 4, "\x14\x40\x80\x72", "\x14\x80\x80\x72" },
	{ KPS(KIP_TEXT) | 0xAAB44, 8, "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", "\xE0\x03\x1F\x2A\xC0\x03\x5F\xD6" },
	{ 0, 0, NULL, NULL }
};

static kip1_patchset_t _fs_patches_40x[] =
{
	{ "nogc",     _fs_nogc_40x },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patch_t _fs_nogc_410[] =
{
	{ KPS(KIP_TEXT) | 0xA34BC, 4, "\x14\x40\x80\x72", "\x14\x80\x80\x72" },
	{ KPS(KIP_TEXT) | 0xAABA8, 8, "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", "\xE0\x03\x1F\x2A\xC0\x03\x5F\xD6" },
	{ 0, 0, NULL, NULL }
};

static kip1_patchset_t _fs_patches_410[] =
{
	{ "nogc",     _fs_nogc_410 },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patch_t _fs_nogc_50x[] =
{
	{ KPS(KIP_TEXT) | 0xCF3C4, 4, "\x14\x40\x80\x52", "\x14\x80\x80\x52" },
	{ KPS(KIP_TEXT) | 0xD73A0, 8, "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", "\xE0\x03\x1F\x2A\xC0\x03\x5F\xD6" },
	{ 0, 0, NULL, NULL }
};

static kip1_patchset_t _fs_patches_50x[] =
{
	{ "nogc",     _fs_nogc_50x },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patch_t _fs_nogc_510[] =
{
	{ KPS(KIP_TEXT) | 0xCF794, 4, "\x14\x40\x80\x52", "\x14\x80\x80\x52" },
	{ KPS(KIP_TEXT) | 0xD7770, 8, "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", "\xE0\x03\x1F\x2A\xC0\x03\x5F\xD6" },
	{ 0, 0, NULL, NULL }
};

static kip1_patchset_t _fs_patches_510[] =
{
	{ "nogc",     _fs_nogc_510 },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patch_t _fs_nogc_600[] =
{
	{ KPS(KIP_TEXT) | 0x12CC20, 8, "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", "\xE0\x03\x1F\x2A\xC0\x03\x5F\xD6" },
	{ KPS(KIP_TEXT) | 0x1538F4, 4, "\x14\x40\x80\x52", "\x14\x80\x80\x52" },
	{ 0, 0, NULL, NULL }
};

static kip1_patch_t _fs_nogc_600_exfat[] =
{
	{ KPS(KIP_TEXT) | 0x138320, 8, "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", "\xE0\x03\x1F\x2A\xC0\x03\x5F\xD6" },
	{ KPS(KIP_TEXT) | 0x15EFF4, 4, "\x14\x40\x80\x52", "\x14\x80\x80\x52" },
	{ 0, 0, NULL, NULL }
};

static kip1_patchset_t _fs_patches_600[] =
{
	{ "nogc",     _fs_nogc_600 },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patchset_t _fs_patches_600_exfat[] =
{
	{ "nogc",     _fs_nogc_600_exfat },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patch_t _fs_nogc_700[] =
{
	{ KPS(KIP_TEXT) | 0x134160, 8, "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", "\xE0\x03\x1F\x2A\xC0\x03\x5F\xD6" },
	{ KPS(KIP_TEXT) | 0x15BF04, 4, "\x14\x40\x80\x52", "\x14\x80\x80\x52" },
	{ 0, 0, NULL, NULL }
};

static kip1_patch_t _fs_nogc_700_exfat[] =
{
	{ KPS(KIP_TEXT) | 0x13F710, 8, "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", "\xE0\x03\x1F\x2A\xC0\x03\x5F\xD6" },
	{ KPS(KIP_TEXT) | 0x1674B4, 4, "\x14\x40\x80\x52", "\x14\x80\x80\x52" },
	{ 0, 0, NULL, NULL }
};

static kip1_patchset_t _fs_patches_700[] =
{
	{ "nogc",     _fs_nogc_700 },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patchset_t _fs_patches_700_exfat[] =
{
	{ "nogc",     _fs_nogc_700_exfat },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patch_t _fs_nogc_800[] =
{
	{ KPS(KIP_TEXT) | 0x136800, 8, "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", "\xE0\x03\x1F\x2A\xC0\x03\x5F\xD6" },
	{ KPS(KIP_TEXT) | 0x15EB94, 4, "\x14\x40\x80\x52", "\x14\x80\x80\x52" },
	{ 0, 0, NULL, NULL }
};

static kip1_patch_t _fs_nogc_800_exfat[] =
{
	{ KPS(KIP_TEXT) | 0x141DB0, 8, "\xF4\x4F\xBE\xA9\xFD\x7B\x01\xA9", "\xE0\x03\x1F\x2A\xC0\x03\x5F\xD6" },
	{ KPS(KIP_TEXT) | 0x16A144, 4, "\x14\x40\x80\x52", "\x14\x80\x80\x52" },
	{ 0, 0, NULL, NULL }
};

static kip1_patchset_t _fs_patches_800[] =
{
	{ "nogc",     _fs_nogc_800 },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

static kip1_patchset_t _fs_patches_800_exfat[] =
{
	{ "nogc",     _fs_nogc_800_exfat },
	{ "emummc",   _fs_emummc },
	{ NULL, NULL }
};

// SHA256 hashes.
static kip1_id_t _kip_ids[] = 
{
	{ "FS", "\xde\x9f\xdd\xa4\x08\x5d\xd5\xfe", _fs_patches_100 },       // FS 1.0.0
	{ "FS", "\xfc\x3e\x80\x99\x1d\xca\x17\x96", _fs_patches_100 },       // FS 1.0.0 exfat
	{ "FS", "\xcd\x7b\xbe\x18\xd6\x13\x0b\x28", _fs_patches_200 },       // FS 2.0.0
	{ "FS", "\xe7\x66\x92\xdf\xaa\x04\x20\xe9", _fs_patches_200 },       // FS 2.0.0 exfat
	{ "FS", "\x0d\x70\x05\x62\x7b\x07\x76\x7c", _fs_patches_210 },       // FS 2.1.0
	{ "FS", "\xdb\xd8\x5f\xca\xcc\x19\x3d\xa8", _fs_patches_210 },       // FS 2.1.0 exfat
	{ "FS", "\xa8\x6d\xa5\xe8\x7e\xf1\x09\x7b", _fs_patches_300 },       // FS 3.0.0
	{ "FS", "\x98\x1c\x57\xe7\xf0\x2f\x70\xf7", _fs_patches_300 },       // FS 3.0.0 exfat
	{ "FS", "\x57\x39\x7c\x06\x3f\x10\xb6\x31", _fs_patches_30x },       // FS 3.0.1
	{ "FS", "\x07\x30\x99\xd7\xc6\xad\x7d\x89", _fs_patches_30x },       // FS 3.0.1 exfat
	{ "FS", "\x06\xe9\x07\x19\x59\x5a\x01\x0c", _fs_patches_40x },       // FS 4.0.1
	{ "FS", "\x54\x9b\x0f\x8d\x6f\x72\xc4\xe9", _fs_patches_40x },       // FS 4.0.1 exfat
	{ "FS", "\x80\x96\xaf\x7c\x6a\x35\xaa\x82", _fs_patches_410 },       // FS 4.1.0
	{ "FS", "\x02\xd5\xab\xaa\xfd\x20\xc8\xb0", _fs_patches_410 },       // FS 4.1.0 exfat
	{ "FS", "\xa6\xf2\x7a\xd9\xac\x7c\x73\xad", _fs_patches_50x },       // FS 5.0.0
	{ "FS", "\xce\x3e\xcb\xa2\xf2\xf0\x62\xf5", _fs_patches_50x },       // FS 5.0.0 exfat
	{ "FS", "\x76\xf8\x74\x02\xc9\x38\x7c\x0f", _fs_patches_510 },       // FS 5.1.0
	{ "FS", "\x10\xb2\xd8\x16\x05\x48\x85\x99", _fs_patches_510 },       // FS 5.1.0 exfat
	{ "FS", "\x1b\x82\xcb\x22\x18\x67\xcb\x52", _fs_patches_600 },       // FS 6.0.0-4.0
	{ "FS", "\x96\x6a\xdd\x3d\x20\xb6\x27\x13", _fs_patches_600_exfat }, // FS 6.0.0-4.0 exfat
	{ "FS", "\x3a\x57\x4d\x43\x61\x86\x19\x1d", _fs_patches_600 },       // FS 6.0.0-5.0
	{ "FS", "\x33\x05\x53\xf6\xb5\xfb\x55\xc4", _fs_patches_600_exfat }, // FS 6.0.0-5.0 exfat
	{ "FS", "\x2A\xDB\xE9\x7E\x9B\x5F\x41\x77", _fs_patches_700 },       // FS 7.0.0
	{ "FS", "\x2C\xCE\x65\x9C\xEC\x53\x6A\x8E", _fs_patches_700_exfat }, // FS 7.0.0 exfat
	{ "FS", "\xB2\xF5\x17\x6B\x35\x48\x36\x4D", _fs_patches_800 },       // FS 8.0.0
	{ "FS", "\xDB\xD9\x41\xC0\xC5\x3C\x52\xCC", _fs_patches_800_exfat }, // FS 8.0.0 exfat
	{ "FS", "\x6B\x09\xB6\x7B\x29\xC0\x20\x24", _fs_patches_800 },       // FS 8.1.0
	{ "FS", "\xB4\xCA\xE1\xF2\x49\x65\xD9\x2E", _fs_patches_800_exfat }  // FS 8.1.0 exfat
};

static void parse_external_kip_patches()
{
	u32 curr_kip_idx = 0;
	char path[64];
	strcpy(path, "bootloader/patches.ini");

	// If patches.ini not found, try to load from template.
	if (f_stat(path, NULL))
		strcpy(path, "bootloader/patches_template.ini");

	LIST_INIT(ini_kip_sections);
	if (ini_patch_parse(&ini_kip_sections, path))
	{
		// Parse patchsets and glue them together.
		LIST_FOREACH_ENTRY(ini_kip_sec_t, ini_psec, &ini_kip_sections, link)
		{
			kip1_id_t* curr_kip = &_kip_ids[curr_kip_idx];

			if (!strcmp(curr_kip->name, ini_psec->name) && !memcmp(curr_kip->hash, ini_psec->hash, 8))
			{
				kip1_patchset_t *patchsets = (kip1_patchset_t *)calloc(sizeof(kip1_patchset_t), 8);

				u32 curr_patchset_idx;
				for(curr_patchset_idx = 0; curr_kip->patchset[curr_patchset_idx].name != NULL; curr_patchset_idx++)
				{
					patchsets[curr_patchset_idx].name = curr_kip->patchset[curr_patchset_idx].name;
					patchsets[curr_patchset_idx].patches = curr_kip->patchset[curr_patchset_idx].patches;
				}

				curr_kip->patchset = patchsets;
				bool first_ext_patch = true;
				u32 curr_patch_idx = 0;

				// Parse patches and glue them together to a patchset.
				kip1_patch_t *patches = calloc(sizeof(kip1_patch_t), 16);
				LIST_FOREACH_ENTRY(ini_patchset_t, pt, &ini_psec->pts, link)
				{
					if (first_ext_patch)
					{
						first_ext_patch = false;
						patchsets[curr_patchset_idx].name = malloc(strlen(pt->name) + 1);
						strcpy(patchsets[curr_patchset_idx].name, pt->name);
						patchsets[curr_patchset_idx].patches = patches;
					}
					else
					{
						if (strcmp(pt->name, patchsets[curr_patchset_idx].name))
						{
							curr_patchset_idx++;
							curr_patch_idx = 0;
							patches = calloc(sizeof(kip1_patch_t), 16);

							patchsets[curr_patchset_idx].name = malloc(strlen(pt->name) + 1);
							strcpy(patchsets[curr_patchset_idx].name, pt->name);
							patchsets[curr_patchset_idx].patches = patches;
						}
					}

					if (pt->length)
					{
						patches[curr_patch_idx].offset = pt->offset;
						patches[curr_patch_idx].length = pt->length;

						patches[curr_patch_idx].srcData = malloc(pt->length);
						patches[curr_patch_idx].dstData = malloc(pt->length);
						memcpy(patches[curr_patch_idx].srcData, pt->srcData, pt->length);
						memcpy(patches[curr_patch_idx].dstData, pt->dstData, pt->length);
					}
					
					curr_patch_idx++;
				}
				curr_patchset_idx++;
				patchsets[curr_patchset_idx].name = NULL;
				patchsets[curr_patchset_idx].patches = NULL;
			}

			curr_kip_idx++;
			if (!(curr_kip_idx < (sizeof(_kip_ids) / sizeof(_kip_ids[0]))))
				break;
		}
	}
}

const pkg2_kernel_id_t *pkg2_identify(u8 *hash)
{
	for (u32 i = 0; i < (sizeof(_pkg2_kernel_ids) / sizeof(pkg2_kernel_id_t)); i++)
	{
		if (!memcmp(hash, _pkg2_kernel_ids[i].hash, sizeof(_pkg2_kernel_ids[0].hash)))
			return &_pkg2_kernel_ids[i];
	}
	return NULL;
}

static u32 _pkg2_calc_kip1_size(pkg2_kip1_t *kip1)
{
	u32 size = sizeof(pkg2_kip1_t);
	for (u32 j = 0; j < KIP1_NUM_SECTIONS; j++)
		size += kip1->sections[j].size_comp;
	return size;
}

void pkg2_parse_kips(link_t *info, pkg2_hdr_t *pkg2, bool *new_pkg2)
{
	u8 *ptr;
	// Check for new pkg2 type.
	if (!pkg2->sec_size[PKG2_SEC_INI1])
	{
		u32 kernel_ini1_off = *(u32 *)(pkg2->data + PKG2_NEWKERN_INI1_START);
		ptr = pkg2->data + kernel_ini1_off;
		*new_pkg2 = true;
	}
	else
		ptr = pkg2->data + pkg2->sec_size[PKG2_SEC_KERNEL];

	pkg2_ini1_t *ini1 = (pkg2_ini1_t *)ptr;
	ptr += sizeof(pkg2_ini1_t);

	for (u32 i = 0; i < ini1->num_procs; i++)
	{
		pkg2_kip1_t *kip1 = (pkg2_kip1_t *)ptr;
		pkg2_kip1_info_t *ki = (pkg2_kip1_info_t *)malloc(sizeof(pkg2_kip1_info_t));
		ki->kip1 = kip1;
		ki->size = _pkg2_calc_kip1_size(kip1);
		list_append(info, &ki->link);
		ptr += ki->size;
DPRINTF(" kip1 %d:%s @ %08X (%08X)\n", i, kip1->name, (u32)kip1, ki->size);
	}
}

int pkg2_has_kip(link_t *info, u64 tid)
{
	LIST_FOREACH_ENTRY(pkg2_kip1_info_t, ki, info, link)
		if(ki->kip1->tid == tid)
			return 1;
	return 0;
}

void pkg2_replace_kip(link_t *info, u64 tid, pkg2_kip1_t *kip1)
{
	LIST_FOREACH_ENTRY(pkg2_kip1_info_t, ki, info, link)
	{
		if (ki->kip1->tid == tid)
		{
			ki->kip1 = kip1;
			ki->size = _pkg2_calc_kip1_size(kip1);
DPRINTF("replaced kip %s (new size %08X)\n", kip1->name, ki->size);
			return;
		}
	}
}

void pkg2_add_kip(link_t *info, pkg2_kip1_t *kip1)
{
	pkg2_kip1_info_t *ki = (pkg2_kip1_info_t *)malloc(sizeof(pkg2_kip1_info_t));
	ki->kip1 = kip1;
	ki->size = _pkg2_calc_kip1_size(kip1);
DPRINTF("added kip %s (size %08X)\n", kip1->name, ki->size);
	list_append(info, &ki->link);
}

void pkg2_merge_kip(link_t *info, pkg2_kip1_t *kip1)
{
	if (pkg2_has_kip(info, kip1->tid))
		pkg2_replace_kip(info, kip1->tid, kip1);
	else
		pkg2_add_kip(info, kip1);
}

int pkg2_decompress_kip(pkg2_kip1_info_t* ki, u32 sectsToDecomp)
{
	u32 compClearMask = ~sectsToDecomp;
	if ((ki->kip1->flags & compClearMask) == ki->kip1->flags)
		return 0; // Already decompressed, nothing to do.

	pkg2_kip1_t hdr;
	memcpy(&hdr, ki->kip1, sizeof(hdr));
	
	unsigned int newKipSize = sizeof(hdr);
	for (u32 sectIdx = 0; sectIdx < KIP1_NUM_SECTIONS; sectIdx++)
	{
		u32 sectCompBit = 1u << sectIdx;
		// For compressed, cant get actual decompressed size without doing it, so use safe "output size".
		if (sectIdx < 3 && (sectsToDecomp & sectCompBit) && (hdr.flags & sectCompBit)) 
			newKipSize += hdr.sections[sectIdx].size_decomp;
		else
			newKipSize += hdr.sections[sectIdx].size_comp;
	}

	pkg2_kip1_t* newKip = malloc(newKipSize);	
	unsigned char* dstDataPtr = newKip->data;
	const unsigned char* srcDataPtr = ki->kip1->data;
	for (u32 sectIdx = 0; sectIdx < KIP1_NUM_SECTIONS; sectIdx++)
	{
		u32 sectCompBit = 1u << sectIdx;
		// Easy copy path for uncompressed or ones we dont want to uncompress.
		if (sectIdx >= 3 || !(sectsToDecomp & sectCompBit) || !(hdr.flags & sectCompBit))
		{
			unsigned int dataSize = hdr.sections[sectIdx].size_comp;
			if (dataSize == 0)
				continue;

			memcpy(dstDataPtr, srcDataPtr, dataSize);
			srcDataPtr += dataSize;
			dstDataPtr += dataSize;
			continue;
		}

		unsigned int compSize = hdr.sections[sectIdx].size_comp;
		unsigned int outputSize = hdr.sections[sectIdx].size_decomp;
		gfx_printf("Decomping %s KIP1 sect %d of size %d...\n", (const char*)hdr.name, sectIdx, compSize);
		if (blz_uncompress_srcdest(srcDataPtr, compSize, dstDataPtr, outputSize) == 0)
		{
			gfx_printf("%kERROR decomping sect %d of %s KIP!%k\n", 0xFFFF0000, sectIdx, (char*)hdr.name, 0xFFCCCCCC);			
			free(newKip);

			return 1;
		}
		else
		{
			DPRINTF("Done! Decompressed size is %d!\n", outputSize);
		}
		hdr.sections[sectIdx].size_comp = outputSize;
		srcDataPtr += compSize;
		dstDataPtr += outputSize;
	}

	hdr.flags &= compClearMask;
	memcpy(newKip, &hdr, sizeof(hdr));
	newKipSize = dstDataPtr-(unsigned char*)(newKip);

	free(ki->kip1);
	ki->kip1 = newKip;
	ki->size = newKipSize;

	return 0;
}

static int _kipm_inject(const char *kipm_path, char *target_name, pkg2_kip1_info_t* ki)
{
	if (!strcmp((const char *)ki->kip1->name, target_name))
	{
		u32 size = 0;
		u8 *kipm_data = (u8 *)sd_file_read(kipm_path, &size);
		if (!kipm_data)
			return 1;

		u32 inject_size = size - sizeof(ki->kip1->caps);
		u8 *kip_patched_data = (u8 *)malloc(ki->size + inject_size);

		// Copy headers.
		memcpy(kip_patched_data, ki->kip1, sizeof(pkg2_kip1_t));

		pkg2_kip1_t *fs_kip = ki->kip1;
		ki->kip1 = (pkg2_kip1_t *)kip_patched_data;
		ki->size = ki->size + inject_size;

		// Patch caps.
		memcpy(&ki->kip1->caps, kipm_data, sizeof(ki->kip1->caps));
		// Copy our .text data.
		memcpy(&ki->kip1->data, kipm_data + sizeof(ki->kip1->caps), inject_size);

		u32 new_offset = 0;

		for (u32 currSectIdx = 0; currSectIdx < KIP1_NUM_SECTIONS - 2; currSectIdx++)
		{
			if(!currSectIdx) // .text.
			{
				memcpy(ki->kip1->data + inject_size, fs_kip->data, fs_kip->sections[0].size_comp);
				ki->kip1->sections[0].size_decomp += inject_size;
				ki->kip1->sections[0].size_comp += inject_size;
			}
			else // Others.
			{
				if (currSectIdx < 3)
					memcpy(ki->kip1->data + new_offset + inject_size, fs_kip->data + new_offset, fs_kip->sections[currSectIdx].size_comp);
				ki->kip1->sections[currSectIdx].offset += inject_size;
			}
			new_offset += fs_kip->sections[currSectIdx].size_comp;
		}

		// Patch PMC capabilities for 1.0.0.
		if (!emu_cfg.fs_ver)
		{
			for (u32 i = 0; i < 0x20; i++)
			{
				if (ki->kip1->caps[i] == 0xFFFFFFFF)
				{
					ki->kip1->caps[i] = 0x07000E7F;
					break;
				}
			}
		}

		free(kipm_data);
		return 0;
	}

	return 1;
}

static bool ext_patches_parsed = false;

const char* pkg2_patch_kips(link_t *info, char* patchNames)
{
	if (patchNames == NULL || patchNames[0] == 0)
		return NULL;

	if (!ext_patches_parsed)
	{
		parse_external_kip_patches();
		ext_patches_parsed = true;
	}

	static const u32 MAX_NUM_PATCHES_REQUESTED = sizeof(u32) * 8;
	char* patches[MAX_NUM_PATCHES_REQUESTED];

	u32 numPatches = 1;
	patches[0] = patchNames;
	{
		for (char* p = patchNames; *p != 0; p++)
		{
			if (*p == ',')
			{
				*p = 0;
				patches[numPatches++] = p + 1;
				if (numPatches >= MAX_NUM_PATCHES_REQUESTED)
					return "too_many_patches";
			}
			else if (*p >= 'A' && *p <= 'Z')
				*p += 0x20;
		}
	}

	u32 patchesApplied = 0; // Bitset over patches.
	for (u32 i = 0; i < numPatches; i++)
	{
		// Eliminate leading spaces.
		for (const char* p = patches[i]; *p != 0; p++)
		{
			if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
				patches[i]++;
			else
				break;
		}
		int valueLen = strlen(patches[i]);
		if (valueLen == 0)
			continue;

		// Eliminate trailing spaces.
		for (int chIdx = valueLen - 1; chIdx >= 0; chIdx--)
		{
			const char* p = patches[i] + chIdx;
			if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
				valueLen = chIdx;
			else
				break;
		}
		patches[i][valueLen] = 0;

		DPRINTF("Requested patch: '%s'\n", patches[i]);
	}

	u32 shaBuf[32 / sizeof(u32)];
	LIST_FOREACH_ENTRY(pkg2_kip1_info_t, ki, info, link)
	{
		shaBuf[0] = 0; // sha256 for this kip not yet calculated.
		for (u32 currKipIdx = 0; currKipIdx < (sizeof(_kip_ids) / sizeof(_kip_ids[0])); currKipIdx++)
		{
			if (strncmp((const char*)ki->kip1->name, _kip_ids[currKipIdx].name, sizeof(ki->kip1->name)) != 0)
				continue;

			u32 bitsAffected = 0;
			kip1_patchset_t* currPatchset = _kip_ids[currKipIdx].patchset;
			while (currPatchset != NULL && currPatchset->name != NULL)
			{
				for (u32 i = 0; i < numPatches; i++)
				{
					if (strcmp(currPatchset->name, patches[i]) != 0)
					{
						bitsAffected = i + 1;
						break;
					}
				}
				currPatchset++;
			}

			// Dont bother even hashing this KIP if we dont have any patches enabled for it.
			if (bitsAffected == 0)
				continue;

			if (shaBuf[0] == 0)
			{
				if (!se_calc_sha256(shaBuf, ki->kip1, ki->size))
					memset(shaBuf, 0, sizeof(shaBuf));
			}			

			if (memcmp(shaBuf, _kip_ids[currKipIdx].hash, sizeof(_kip_ids[0].hash)) != 0)
				continue;

			// Find out which sections are affected by the enabled patches, to know which to decompress.
			bitsAffected = 0;
			currPatchset = _kip_ids[currKipIdx].patchset;
			while (currPatchset != NULL && currPatchset->name != NULL)
			{
				if (currPatchset->patches != NULL)
				{
					for (u32 currEnabIdx = 0; currEnabIdx < numPatches; currEnabIdx++)
					{
						if (strcmp(currPatchset->name, patches[currEnabIdx]))
							continue;

						if (!strcmp(currPatchset->name, "emummc"))
							bitsAffected |= 1u << GET_KIP_PATCH_SECTION(currPatchset->patches->offset);

						for (const kip1_patch_t* currPatch=currPatchset->patches; currPatch != NULL && (currPatch->length != 0); currPatch++)
							bitsAffected |= 1u << GET_KIP_PATCH_SECTION(currPatch->offset);
					}
				}		
				currPatchset++;
			}

			// Got patches to apply to this kip, have to decompress it.
#ifdef DEBUG_PRINTING
			u32 preDecompTime = get_tmr_us();
#endif
			if (pkg2_decompress_kip(ki, bitsAffected))
				return (const char*)ki->kip1->name; // Failed to decompress.

#ifdef DEBUG_PRINTING
			u32 postDecompTime = get_tmr_us();
			if (!se_calc_sha256(shaBuf, ki->kip1, ki->size))
				memset(shaBuf, 0, sizeof(shaBuf));

			DPRINTF("%dms %s KIP1 size %d hash %08X\n", (postDecompTime-preDecompTime) / 1000, ki->kip1->name, (int)ki->size, __builtin_bswap32(shaBuf[0]));
#endif

			currPatchset = _kip_ids[currKipIdx].patchset;
			bool emummc_patch_selected = false;
			while (currPatchset != NULL && currPatchset->name != NULL)
			{
				for (u32 currEnabIdx = 0; currEnabIdx < numPatches; currEnabIdx++)
				{
					if (strcmp(currPatchset->name, patches[currEnabIdx]))
						continue;

					u32 appliedMask = 1u << currEnabIdx;
					if (currPatchset->patches == NULL || !strcmp(currPatchset->name, "emummc"))
					{
						gfx_printf("Patch '%s' not necessary for %s KIP1\n", currPatchset->name, (const char*)ki->kip1->name);
						patchesApplied |= appliedMask;

						if (!strcmp(currPatchset->name, "emummc"))
							emummc_patch_selected = true;
						break;
					}

					unsigned char* kipSectData = ki->kip1->data;
					for (u32 currSectIdx = 0; currSectIdx < KIP1_NUM_SECTIONS; currSectIdx++)
					{
						if (bitsAffected & (1u << currSectIdx))
						{
							gfx_printf("Applying patch '%s' on %s KIP1 sect %d\n", currPatchset->name, (const char*)ki->kip1->name, currSectIdx);
							for (const kip1_patch_t* currPatch = currPatchset->patches; currPatch != NULL && currPatch->length != 0; currPatch++)
							{
								if (GET_KIP_PATCH_SECTION(currPatch->offset) != currSectIdx)
									continue;

								u32 currOffset = GET_KIP_PATCH_OFFSET(currPatch->offset);
								if (memcmp(&kipSectData[currOffset], currPatch->srcData, currPatch->length) != 0)
								{
									gfx_printf("%kDATA MISMATCH FOR PATCH AT OFFSET 0x%x!!!%k\n", 0xFFFF0000, currOffset, 0xFFCCCCCC);
									return currPatchset->name; // MUST stop here as kip is likely corrupt.
								}
								else
								{
									DPRINTF("Patching %d bytes at offset 0x%x\n", currPatch->length, currOffset);
									memcpy(&kipSectData[currOffset], currPatch->dstData, currPatch->length);
								}
							}
						}
						kipSectData += ki->kip1->sections[currSectIdx].size_comp;
					}
					
					patchesApplied |= appliedMask;
					break;
				}
				currPatchset++;
			}
			if (!strncmp(_kip_ids[currKipIdx].name, "FS", 2) && emummc_patch_selected)
			{
				emummc_patch_selected = false;
				emu_cfg.fs_ver = currKipIdx;
				if (currKipIdx)
					emu_cfg.fs_ver--;
				if (currKipIdx > 17)
					emu_cfg.fs_ver -= 2;

				gfx_printf("Injecting emuMMC. FS ver: %d\n", emu_cfg.fs_ver);
				if (_kipm_inject("/bootloader/sys/emummc.kipm", "FS", ki))
					return "emummc";
			}
		}
	}

	for (u32 i = 0; i < numPatches; i++)
	{
		if ((patchesApplied & (1u << i)) == 0)
			return patches[i];
	}

	return NULL;
}

pkg2_hdr_t *pkg2_decrypt(void *data)
{
	u8 *pdata = (u8 *)data;
	
	// Skip signature.
	pdata += 0x100;

	pkg2_hdr_t *hdr = (pkg2_hdr_t *)pdata;

	// Skip header.
	pdata += sizeof(pkg2_hdr_t);

	// Decrypt header.
	se_aes_crypt_ctr(8, hdr, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);
	//gfx_hexdump((u32)hdr, hdr, 0x100);

	if (hdr->magic != PKG2_MAGIC)
		return NULL;

	for (u32 i = 0; i < 4; i++)
	{
DPRINTF("sec %d has size %08X\n", i, hdr->sec_size[i]);
		if (!hdr->sec_size[i])
			continue;

		se_aes_crypt_ctr(8, pdata, hdr->sec_size[i], pdata, hdr->sec_size[i], &hdr->sec_ctr[i * 0x10]);
		//gfx_hexdump((u32)pdata, pdata, 0x100);

		pdata += hdr->sec_size[i];
	}

	return hdr;
}

static u32 _pkg2_ini1_build(u8 *pdst, pkg2_hdr_t *hdr, link_t *kips_info, bool new_pkg2)
{
	u32 ini1_size = sizeof(pkg2_ini1_t);
	pkg2_ini1_t *ini1 = (pkg2_ini1_t *)pdst;
	memset(ini1, 0, sizeof(pkg2_ini1_t));
	ini1->magic = INI1_MAGIC;
	pdst += sizeof(pkg2_ini1_t);
	LIST_FOREACH_ENTRY(pkg2_kip1_info_t, ki, kips_info, link)
	{
DPRINTF("adding kip1 '%s' @ %08X (%08X)\n", ki->kip1->name, (u32)ki->kip1, ki->size);
		memcpy(pdst, ki->kip1, ki->size);
		pdst += ki->size;
		ini1_size += ki->size;
		ini1->num_procs++;
	}
	ini1_size = ALIGN(ini1_size, 4);
	ini1->size = ini1_size;
	if (!new_pkg2)
	{
		hdr->sec_size[PKG2_SEC_INI1] = ini1_size;
		hdr->sec_off[PKG2_SEC_INI1] = 0x14080000;
		se_aes_crypt_ctr(8, ini1, ini1_size, ini1, ini1_size, &hdr->sec_ctr[PKG2_SEC_INI1 * 0x10]);
	}
	else
	{
		hdr->sec_size[PKG2_SEC_INI1] = 0;
		hdr->sec_off[PKG2_SEC_INI1] = 0;
	}

	return ini1_size;
}

void pkg2_build_encrypt(void *dst, void *kernel, u32 kernel_size, link_t *kips_info, bool new_pkg2)
{
	u8 *pdst = (u8 *)dst;

	// Signature.
	memset(pdst, 0, 0x100);
	pdst += 0x100;

	// Header.
	pkg2_hdr_t *hdr = (pkg2_hdr_t *)pdst;
	memset(hdr, 0, sizeof(pkg2_hdr_t));
	pdst += sizeof(pkg2_hdr_t);
	hdr->magic = PKG2_MAGIC;
	if (!new_pkg2)
		hdr->base = 0x10000000;
	else
		hdr->base = 0x60000;
DPRINTF("kernel @ %08X (%08X)\n", (u32)kernel, kernel_size);

	// Kernel.
	memcpy(pdst, kernel, kernel_size);
	if (!new_pkg2)
		hdr->sec_off[PKG2_SEC_KERNEL] = 0x10000000;
	else
	{
		// Set new INI1 offset to kernel.
		*(u32 *)(pdst + PKG2_NEWKERN_INI1_START) = kernel_size;
		kernel_size += _pkg2_ini1_build(pdst + kernel_size, hdr, kips_info, new_pkg2);
		hdr->sec_off[PKG2_SEC_KERNEL] = 0x60000;
	}
	hdr->sec_size[PKG2_SEC_KERNEL] = kernel_size;
	se_aes_crypt_ctr(8, pdst, kernel_size, pdst, kernel_size, &hdr->sec_ctr[PKG2_SEC_KERNEL * 0x10]);
	pdst += kernel_size;
DPRINTF("kernel encrypted\n");

	// INI1.
	u32 ini1_size = 0;
	if (!new_pkg2)
		ini1_size = _pkg2_ini1_build(pdst, hdr, kips_info, new_pkg2);
DPRINTF("INI1 encrypted\n");

	//Encrypt header.
	*(u32 *)hdr->ctr = 0x100 + sizeof(pkg2_hdr_t) + kernel_size + ini1_size;
	se_aes_crypt_ctr(8, hdr, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);
	memset(hdr->ctr, 0 , 0x10);
	*(u32 *)hdr->ctr = 0x100 + sizeof(pkg2_hdr_t) + kernel_size + ini1_size;
}
