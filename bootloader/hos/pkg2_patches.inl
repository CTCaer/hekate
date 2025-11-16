/*
 * Copyright (c) 2018-2024 CTCaer
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

//TODO: Replace hardcoded AArch64 instructions with instruction macros.
//TODO: Reduce hardcoded values without searching kernel for patterns?
// The process ID send/receive kernel patches were taken from Atmosphère's kernel patches.
// They should only be used when running Atmosphère.
#define CODE_OFF_1ST_100  0x4797C
#define CODE_OFF_1ST_200  0x6486C
#define CODE_OFF_1ST_300  0x494A4
#define CODE_OFF_1ST_302  0x494BC
#define CODE_OFF_1ST_400  0x52890
#define CODE_OFF_1ST_500  0x5C020
#define CODE_OFF_1ST_600  0x5EE00
#define CODE_OFF_1ST_700  0x5FEC0
#define CODE_OFF_1ST_800  0x607F0
#define CODE_OFF_1ST_900  0x65780
#define CODE_OFF_1ST_1000 0x67790
#define CODE_OFF_1ST_1100 0x49EE8
#define CODE_OFF_1ST_1200 0x48810

#define ID_SND_OFF_100  0x23CC0
#define ID_SND_OFF_200  0x3F134
#define ID_SND_OFF_300  0x26080
#define ID_SND_OFF_302  0x26080
#define ID_SND_OFF_400  0x2AF64
#define ID_SND_OFF_500  0x2AD34
#define ID_SND_OFF_600  0x2BB8C
#define ID_SND_OFF_700  0x2D044
#define ID_SND_OFF_800  0x2F1FC
#define ID_SND_OFF_900  0x329A0
#define ID_SND_OFF_1000 0x34404
#define ID_SND_OFF_1100 0x245B4
#define ID_SND_OFF_1101 0x245B8
#define ID_SND_OFF_1200 0x24CF4

#define ID_RCV_OFF_100  0x219F0
#define ID_RCV_OFF_200  0x3D1A8
#define ID_RCV_OFF_300  0x240F0
#define ID_RCV_OFF_302  0x240F0
#define ID_RCV_OFF_400  0x28F6C
#define ID_RCV_OFF_500  0x28DAC
#define ID_RCV_OFF_600  0x29B6C
#define ID_RCV_OFF_700  0x2B23C
#define ID_RCV_OFF_800  0x2D424
#define ID_RCV_OFF_900  0x309B4
#define ID_RCV_OFF_1000 0x322F8
#define ID_RCV_OFF_1100 0x22B24
#define ID_RCV_OFF_1101 0x22B28
#define ID_RCV_OFF_1200 0x23424

static const u32 PRC_ID_SND_100[] =
{
	0xA9BF2FEA, 0x2A0E03EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B,
	0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9412948,	0xA8C12FEA
};
#define CODE_OFF_2ND_100 (CODE_OFF_1ST_100 + sizeof(PRC_ID_SND_100) + sizeof(u32))
static const u32 PRC_ID_RCV_100[] =
{
	0xA9BF2FEA, 0x2A1C03EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A,
	0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9412968, 0xA8C12FEA
};

static const u32 PRC_ID_SND_200[] =
{
	0xA9BF2FEA, 0x2A1803EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B,
	0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9413148, 0xA8C12FEA
};
#define CODE_OFF_2ND_200 (CODE_OFF_1ST_200 + sizeof(PRC_ID_SND_200) + sizeof(u32))
static const u32 PRC_ID_RCV_200[] =
{
	0xA9BF2FEA, 0x2A0F03EA, 0xD37EF54A, 0xF9405FEB, 0xF86A696A, 0xF9407BEB, 0x92FFFFE9, 0x8A090148,
	0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9413168, 0xA8C12FEA
};

static const u32 PRC_ID_SND_300[] =
{
	0xA9BF2FEA, 0x2A1803EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B,
	0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9415548, 0xA8C12FEA
};
#define CODE_OFF_2ND_300 (CODE_OFF_1ST_300 + sizeof(PRC_ID_SND_300) + sizeof(u32))
static const u32 PRC_ID_RCV_300[] =
{
	0xA9BF2FEA, 0x2A0F03EA, 0xD37EF54A, 0xF9405FEB, 0xF86A696A, 0xF9407BEB, 0x92FFFFE9, 0x8A090148,
	0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415568, 0xA8C12FEA
};

#define CODE_OFF_2ND_302 (CODE_OFF_1ST_302 + sizeof(PRC_ID_SND_300) + sizeof(u32))

static const u32 PRC_ID_SND_400[] =
{
	0x2A1703EA, 0xD37EF54A, 0xF86A6B8A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9,
	0xEB09015F, 0x54000060, 0xF94053EA, 0xF9415948, 0xF94053EA
};
#define CODE_OFF_2ND_400 (CODE_OFF_1ST_400 + sizeof(PRC_ID_SND_400) + sizeof(u32))
static const u32 PRC_ID_RCV_400[] =
{
	0xF9403BED, 0x2A0E03EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A,
	0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415B28, 0xD503201F
};

static const u32 PRC_ID_SND_500[] =
{
	0x2A1703EA, 0xD37EF54A, 0xF86A6B6A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9,
	0xEB09015F, 0x54000060, 0xF94043EA, 0xF9415948, 0xF94043EA
};
#define CODE_OFF_2ND_500 (CODE_OFF_1ST_500 + sizeof(PRC_ID_SND_500) + sizeof(u32))
static const u32 PRC_ID_RCV_500[] =
{
	0xF9403BED, 0x2A1503EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A,
	0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415B08, 0xF9406FEA
};

static const u32 PRC_ID_SND_600[] =
{
	0xA9BF2FEA, 0xF94037EB, 0x2A1503EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400308, 0xF9401D08, 0xAA1803E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};
#define CODE_OFF_2ND_600 (CODE_OFF_1ST_600 + sizeof(PRC_ID_SND_600) + sizeof(u32))
static const u32 PRC_ID_RCV_600[] =
{
	0xA9BF2FEA, 0xF94043EB, 0x2A1503EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400308, 0xF9401D08, 0xAA1803E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};

static const u32 PRC_ID_SND_700[] =
{
	0xA9BF2FEA, 0xF9403BEB, 0x2A1903EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF94002A8, 0xF9401D08, 0xAA1503E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};
#define CODE_OFF_2ND_700 (CODE_OFF_1ST_700 + sizeof(PRC_ID_SND_700) + sizeof(u32))
static const u32 PRC_ID_RCV_700[] =
{
	0xA9BF2FEA, 0xF9404FEB, 0x2A1603EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400368, 0xF9401D08, 0xAA1B03E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};

#define CODE_OFF_2ND_800 (CODE_OFF_1ST_800 + sizeof(PRC_ID_SND_700) + sizeof(u32))

static const u32 PRC_ID_SND_900[] =
{
	0xA9BF2FEA, 0xF94037EB, 0x2A1603EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF94002E8, 0xF9401D08, 0xAA1703E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};
#define CODE_OFF_2ND_900 (CODE_OFF_1ST_900 + sizeof(PRC_ID_SND_900) + sizeof(u32))
static const u32 PRC_ID_RCV_900[] =
{
	0xA9BF2FEA, 0xF9404BEB, 0x2A1703EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400368, 0xF9401D08, 0xAA1B03E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};

static const u32 PRC_ID_SND_1000[] =
{
	0xA9BF2FEA, 0xF94063EB, 0x2A1603EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF94002E8, 0xF9401D08, 0xAA1703E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};
#define CODE_OFF_2ND_1000 (CODE_OFF_1ST_1000 + sizeof(PRC_ID_SND_1000) + sizeof(u32))
static const u32 PRC_ID_RCV_1000[] =
{
	0xA9BF2FEA, 0xF94067EB, 0x2A1A03EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400388, 0xF9401D08, 0xAA1C03E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};

static const u32 PRC_ID_SND_1100[] =
{
	0xA9BF2FEA, 0xF94043EB, 0x5280006A, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF94002A8, 0xF9401D08, 0xAA1503E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};
#define CODE_OFF_2ND_1100 (CODE_OFF_1ST_1100 + sizeof(PRC_ID_SND_1100) + sizeof(u32))
static const u32 PRC_ID_RCV_1100[] =
{
	0xA9BF2FEA, 0xF94073EB, 0x5280006A, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400308, 0xF9401D08, 0xAA1803E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};

static const u32 PRC_ID_SND_1200[] =
{
	0xA9BF2FEA, 0xF9404FEB, 0x5280006A, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF94002C8, 0xF9401D08, 0xAA1603E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};
#define CODE_OFF_2ND_1200 (CODE_OFF_1ST_1200 + sizeof(PRC_ID_SND_1200) + sizeof(u32))
static const u32 PRC_ID_RCV_1200[] =
{
	0xA9BF2FEA, 0xF94073EB, 0x5280006A, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9,
	0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400388, 0xF9401D08, 0xAA1C03E0,
	0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0
};

// Include kernel patches here, so we can utilize pkg1 id
KERNEL_PATCHSET_DEF(_kernel_1_patchset,
	{ SVC_VERIFY_DS, 0x3764C, _NOP(),          NULL }, // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x44074, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_100,   _B(ID_SND_OFF_100, CODE_OFF_1ST_100), NULL },           // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_100, sizeof(PRC_ID_SND_100) >> 2,          PRC_ID_SND_100 }, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_100 + sizeof(PRC_ID_SND_100),                                // Branch back and skip 1 instruction.
		_B(CODE_OFF_1ST_100 + sizeof(PRC_ID_SND_100), ID_SND_OFF_100 + 0x4), NULL },
	{ ATM_GEN_PATCH, ID_RCV_OFF_100,   _B(ID_RCV_OFF_100, CODE_OFF_2ND_100), NULL },           // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_100, sizeof(PRC_ID_RCV_100) >> 2,          PRC_ID_RCV_100 }, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_100 + sizeof(PRC_ID_RCV_100),                                // Branch back and skip 1 instruction.
		_B(CODE_OFF_2ND_100 + sizeof(PRC_ID_RCV_100), ID_RCV_OFF_100 + 0x4), NULL }
);

KERNEL_PATCHSET_DEF(_kernel_2_patchset,
	{ SVC_VERIFY_DS, 0x54834, _NOP(),          NULL }, // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x6086C, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_200, _B(ID_SND_OFF_200, CODE_OFF_1ST_200),   NULL },           // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_200, sizeof(PRC_ID_SND_200) >> 2,          PRC_ID_SND_200 }, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_200 + sizeof(PRC_ID_SND_200),                                // Branch back and skip 1 instruction.
		_B(CODE_OFF_1ST_200 + sizeof(PRC_ID_SND_200), ID_SND_OFF_200 + 0x4), NULL },
	{ ATM_GEN_PATCH, ID_RCV_OFF_200, _B(ID_RCV_OFF_200, CODE_OFF_2ND_200),   NULL },           // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_200, sizeof(PRC_ID_RCV_200) >> 2,          PRC_ID_RCV_200 }, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_200 + sizeof(PRC_ID_RCV_200),                                // Branch back and skip 1 instruction.
		_B(CODE_OFF_2ND_200 + sizeof(PRC_ID_RCV_200), ID_RCV_OFF_200 + 0x4), NULL }
);

KERNEL_PATCHSET_DEF(_kernel_3_patchset,
	{ SVC_VERIFY_DS, 0x3BD24, _NOP(),          NULL }, // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x483FC, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_300, _B(ID_SND_OFF_300, CODE_OFF_1ST_300),   NULL },           // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_300, sizeof(PRC_ID_SND_300) >> 2,          PRC_ID_SND_300 }, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_300 + sizeof(PRC_ID_SND_300),                                // Branch back and skip 1 instruction.
		_B(CODE_OFF_1ST_300 + sizeof(PRC_ID_SND_300), ID_SND_OFF_300 + 0x4), NULL },
	{ ATM_GEN_PATCH, ID_RCV_OFF_300, _B(ID_RCV_OFF_300, CODE_OFF_2ND_300),   NULL },           // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_300, sizeof(PRC_ID_RCV_300) >> 2,          PRC_ID_RCV_300 }, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_300 + sizeof(PRC_ID_RCV_300),                                // Branch back and skip 1 instruction.
		_B(CODE_OFF_2ND_300 + sizeof(PRC_ID_RCV_300), ID_RCV_OFF_300 + 0x4), NULL }
);

KERNEL_PATCHSET_DEF(_kernel_302_patchset,
	{ SVC_VERIFY_DS, 0x3BD24, _NOP(),          NULL }, // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x48414, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_302, _B(ID_SND_OFF_302, CODE_OFF_1ST_302),   NULL },           // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_302, sizeof(PRC_ID_SND_300) >> 2,          PRC_ID_SND_300 }, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_302 + sizeof(PRC_ID_SND_300),                                // Branch back and skip 1 instruction.
		_B(CODE_OFF_1ST_302 + sizeof(PRC_ID_SND_300), ID_SND_OFF_302 + 0x4), NULL },
	{ ATM_GEN_PATCH, ID_RCV_OFF_302, _B(ID_RCV_OFF_302, CODE_OFF_2ND_302),   NULL },           // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_302, sizeof(PRC_ID_RCV_300) >> 2,          PRC_ID_RCV_300 }, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_302 + sizeof(PRC_ID_RCV_300),                                // Branch back and skip 1 instruction.
		_B(CODE_OFF_2ND_302 + sizeof(PRC_ID_RCV_300), ID_RCV_OFF_302 + 0x4), NULL }
);

KERNEL_PATCHSET_DEF(_kernel_4_patchset,
	{ SVC_VERIFY_DS, 0x41EB4, _NOP(),          NULL }, // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x4EBFC, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_400, _B(ID_SND_OFF_400, CODE_OFF_1ST_400),       NULL },           // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_400, sizeof(PRC_ID_SND_400) >> 2,              PRC_ID_SND_400 }, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_400 + sizeof(PRC_ID_SND_400),                                    // Branch back and skip 2 instructions.
		_B(CODE_OFF_1ST_400 + sizeof(PRC_ID_SND_400), ID_SND_OFF_400 + 0x4 * 2), NULL },
	{ ATM_GEN_PATCH, ID_RCV_OFF_400, _B(ID_RCV_OFF_400, CODE_OFF_2ND_400),       NULL },           // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_400, sizeof(PRC_ID_RCV_400) >> 2,              PRC_ID_RCV_400 }, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_400 + sizeof(PRC_ID_RCV_400),                                    // Branch back and skip 1 instruction.
		_B(CODE_OFF_2ND_400 + sizeof(PRC_ID_RCV_400), ID_RCV_OFF_400 + 0x4),     NULL }
);

KERNEL_PATCHSET_DEF(_kernel_5_patchset,
	{ SVC_GENERIC,   0x38C2C, _NOP(),          NULL }, // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x45E6C, _NOP(),          NULL }, // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x5513C, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_SYSM_INCR, 0x54E30, _MOVZW(8, 0x1E00, LSL16), NULL }, // System memory pool increase.
	{ ATM_GEN_PATCH, ID_SND_OFF_500, _B(ID_SND_OFF_500, CODE_OFF_1ST_500),       NULL },           // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_500, sizeof(PRC_ID_SND_500) >> 2,              PRC_ID_SND_500 }, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_500 + sizeof(PRC_ID_SND_500),                                    // Branch back and skip 2 instructions.
		_B(CODE_OFF_1ST_500 + sizeof(PRC_ID_SND_500), ID_SND_OFF_500 + 0x4 * 2), NULL },
	{ ATM_GEN_PATCH, ID_RCV_OFF_500, _B(ID_RCV_OFF_500, CODE_OFF_2ND_500),       NULL },           // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_500, sizeof(PRC_ID_RCV_500) >> 2,              PRC_ID_RCV_500 }, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_500 + sizeof(PRC_ID_RCV_500),                                    // Branch back and skip 2 instructions.
		_B(CODE_OFF_2ND_500 + sizeof(PRC_ID_RCV_500), ID_RCV_OFF_500 + 0x4 * 2), NULL }
);

KERNEL_PATCHSET_DEF(_kernel_6_patchset,
	{ SVC_GENERIC,   0x3A8CC, _NOP(),          NULL }, // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x47EA0, _NOP(),          NULL }, // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x57548, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_SYSM_INCR, 0x57330, _MOVZW(8, 0x1D80, LSL16), NULL }, // System memory pool increase.
	{ ATM_GEN_PATCH, ID_SND_OFF_600, _B(ID_SND_OFF_600, CODE_OFF_1ST_600),       NULL },           // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_600, sizeof(PRC_ID_SND_600) >> 2,              PRC_ID_SND_600 }, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_600 + sizeof(PRC_ID_SND_600),                                    // Branch back and skip 4 instructions.
		_B(CODE_OFF_1ST_600 + sizeof(PRC_ID_SND_600), ID_SND_OFF_600 + 0x4 * 4), NULL },
	{ ATM_GEN_PATCH, ID_RCV_OFF_600, _B(ID_RCV_OFF_600, CODE_OFF_2ND_600),       NULL },           // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_600, sizeof(PRC_ID_RCV_600) >> 2,              PRC_ID_RCV_600 }, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_600 + sizeof(PRC_ID_RCV_600),                                    // Branch back and skip 4 instructions.
		_B(CODE_OFF_2ND_600 + sizeof(PRC_ID_RCV_600), ID_RCV_OFF_600 + 0x4 * 4), NULL }
);

KERNEL_PATCHSET_DEF(_kernel_7_patchset,
	{ SVC_GENERIC,   0x3C6E0, _NOP(),          NULL }, // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x49E5C, _NOP(),          NULL }, // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x581B0, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_SYSM_INCR, 0x57F98, _MOVZW(8, 0x1D80, LSL16), NULL }, // System memory pool increase.
	{ ATM_GEN_PATCH, ID_SND_OFF_700, _B(ID_SND_OFF_700, CODE_OFF_1ST_700),       NULL },           // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_700, sizeof(PRC_ID_SND_700) >> 2,              PRC_ID_SND_700 }, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_700 + sizeof(PRC_ID_SND_700),                                    // Branch back and skip 4 instructions.
		_B(CODE_OFF_1ST_700 + sizeof(PRC_ID_SND_700), ID_SND_OFF_700 + 0x4 * 4), NULL },
	{ ATM_GEN_PATCH, ID_RCV_OFF_700, _B(ID_RCV_OFF_700, CODE_OFF_2ND_700),       NULL },           // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_700, sizeof(PRC_ID_RCV_700) >> 2,              PRC_ID_RCV_700 }, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_700 + sizeof(PRC_ID_RCV_700),                                    // Branch back and skip 4 instructions.
		_B(CODE_OFF_2ND_700 + sizeof(PRC_ID_RCV_700), ID_RCV_OFF_700 + 0x4 * 4), NULL }
);

KERNEL_PATCHSET_DEF(_kernel_8_patchset,
	{ SVC_GENERIC,   0x3FAD0, _NOP(),          NULL }, // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x4D15C, _NOP(),          NULL }, // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x5BFAC, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_SYSM_INCR, 0x5F9A4, _MOVZW(19, 0x1D80, LSL16), NULL }, // System memory pool increase.
	{ ATM_GEN_PATCH, ID_SND_OFF_800, _B(ID_SND_OFF_800, CODE_OFF_1ST_800),       NULL },           // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_800, sizeof(PRC_ID_SND_700) >> 2,              PRC_ID_SND_700 }, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_800 + sizeof(PRC_ID_SND_700),                                    // Branch back and skip 4 instructions.
		_B(CODE_OFF_1ST_800 + sizeof(PRC_ID_SND_700), ID_SND_OFF_800 + 0x4 * 4), NULL },
	{ ATM_GEN_PATCH, ID_RCV_OFF_800, _B(ID_RCV_OFF_800, CODE_OFF_2ND_800),       NULL },           // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_800, sizeof(PRC_ID_RCV_700) >> 2,              PRC_ID_RCV_700 }, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_800 + sizeof(PRC_ID_RCV_700),                                    // Branch back and skip 4 instructions.
		_B(CODE_OFF_2ND_800 + sizeof(PRC_ID_RCV_700), ID_RCV_OFF_800 + 0x4 * 4), NULL }
);

KERNEL_PATCHSET_DEF(_kernel_9_patchset,
	{ SVC_GENERIC,   0x43DFC, _NOP(),          NULL }, // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x50628, _NOP(),          NULL }, // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x609E8, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_SYSM_INCR, 0x6493C, _MOVZW(19, 0x1D80, LSL16), NULL }, // System memory pool increase.
	{ ATM_GEN_PATCH, ID_SND_OFF_900, _B(ID_SND_OFF_900, CODE_OFF_1ST_900),       NULL },           // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_900, sizeof(PRC_ID_SND_900) >> 2,              PRC_ID_SND_900 }, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_900 + sizeof(PRC_ID_SND_900),                                    // Branch back and skip 4 instructions.
		_B(CODE_OFF_1ST_900 + sizeof(PRC_ID_SND_900), ID_SND_OFF_900 + 0x4 * 4), NULL },
	{ ATM_GEN_PATCH, ID_RCV_OFF_900, _B(ID_RCV_OFF_900, CODE_OFF_2ND_900),       NULL },           // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_900, sizeof(PRC_ID_RCV_900) >> 2,              PRC_ID_RCV_900 }, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_900 + sizeof(PRC_ID_RCV_900),                                    // Branch back and skip 4 instructions.
		_B(CODE_OFF_2ND_900 + sizeof(PRC_ID_RCV_900), ID_RCV_OFF_900 + 0x4 * 4), NULL }
);

KERNEL_PATCHSET_DEF(_kernel_10_patchset,
	{ SVC_GENERIC,   0x45DAC, _NOP(),          NULL }, // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x523E4, _NOP(),          NULL }, // Disable SVC verifications.
	{ DEBUG_MODE_EN, 0x62B14, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch.
	// Atmosphère kernel patches.
	{ ATM_SYSM_INCR, 0x66950, _MOVZW(19, 0x1D80, LSL16), NULL }, // System memory pool increase.
	{ ATM_GEN_PATCH, ID_SND_OFF_1000, _B(ID_SND_OFF_1000, CODE_OFF_1ST_1000),       NULL },            // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_1000, sizeof(PRC_ID_SND_1000) >> 2,               PRC_ID_SND_1000 }, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_1000 + sizeof(PRC_ID_SND_1000),                                      // Branch back and skip 4 instructions.
		_B(CODE_OFF_1ST_1000 + sizeof(PRC_ID_SND_1000), ID_SND_OFF_1000 + 0x4 * 4), NULL },
	{ ATM_GEN_PATCH, ID_RCV_OFF_1000, _B(ID_RCV_OFF_1000, CODE_OFF_2ND_1000),       NULL },            // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_1000, sizeof(PRC_ID_RCV_1000) >> 2,               PRC_ID_RCV_1000 }, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_1000 + sizeof(PRC_ID_RCV_1000),                                      // Branch back and skip 4 instructions.
		_B(CODE_OFF_2ND_1000 + sizeof(PRC_ID_RCV_1000), ID_RCV_OFF_1000 + 0x4 * 4), NULL }
);

KERNEL_PATCHSET_DEF(_kernel_11_patchset,
	{ SVC_GENERIC,   0x2FCE0, _NOP(),          NULL }, // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x39194, _NOP(),          NULL }, // Disable SVC verifications.
	{ DEBUG_MODE_EN, 0x460C0, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch.
	// Atmosphère kernel patches.
	{ ATM_SYSM_INCR, 0x490C4, _MOVZW(21, 0x1D80, LSL16), NULL }, // System memory pool increase.
	{ ATM_GEN_PATCH, ID_SND_OFF_1100, _B(ID_SND_OFF_1100, CODE_OFF_1ST_1100),       NULL},            // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_1100, sizeof(PRC_ID_SND_1100) >> 2,               PRC_ID_SND_1100}, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_1100 + sizeof(PRC_ID_SND_1100),                                     // Branch back and skip 4 instructions.
		_B(CODE_OFF_1ST_1100 + sizeof(PRC_ID_SND_1100), ID_SND_OFF_1100 + 0x4 * 4), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_1100, _B(ID_RCV_OFF_1100, CODE_OFF_2ND_1100),       NULL},            // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_1100, sizeof(PRC_ID_RCV_1100) >> 2,               PRC_ID_RCV_1100}, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_1100 + sizeof(PRC_ID_RCV_1100),                                     // Branch back and skip 4 instructions.
		_B(CODE_OFF_2ND_1100 + sizeof(PRC_ID_RCV_1100), ID_RCV_OFF_1100 + 0x4 * 4), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_1101_patchset,
	{ SVC_GENERIC,   0x2FD04, _NOP(),          NULL }, // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x39194, _NOP(),          NULL }, // Disable SVC verifications.
	{ DEBUG_MODE_EN, 0x460C0, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch.
	// Atmosphère kernel patches.
	{ ATM_SYSM_INCR, 0x490C4, _MOVZW(21, 0x1D80, LSL16), NULL }, // System memory pool increase.
	{ ATM_GEN_PATCH, ID_SND_OFF_1101, _B(ID_SND_OFF_1101, CODE_OFF_1ST_1100),       NULL},            // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_1100, sizeof(PRC_ID_SND_1100) >> 2,               PRC_ID_SND_1100}, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_1100 + sizeof(PRC_ID_SND_1100),                                     // Branch back and skip 4 instructions.
		_B(CODE_OFF_1ST_1100 + sizeof(PRC_ID_SND_1100), ID_SND_OFF_1101 + 0x4 * 4), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_1101, _B(ID_RCV_OFF_1101, CODE_OFF_2ND_1100),       NULL},            // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_1100, sizeof(PRC_ID_RCV_1100) >> 2,               PRC_ID_RCV_1100}, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_1100 + sizeof(PRC_ID_RCV_1100),                                     // Branch back and skip 4 instructions.
		_B(CODE_OFF_2ND_1100 + sizeof(PRC_ID_RCV_1100), ID_RCV_OFF_1101 + 0x4 * 4), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_12_patchset,
	{ SVC_GENERIC,   0x2FCB4, _NOP(),          NULL }, // Allow same process on svcControlCodeMemory.
	{ SVC_VERIFY_DS, 0x38440, _NOP(),          NULL }, // Disable SVC verifications.
	{ DEBUG_MODE_EN, 0x45118, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch.
	// Atmosphère kernel patches.
	{ ATM_SYSM_INCR, 0x4809C, _MOVZW(21, 0x1D80, LSL16), NULL }, // System memory pool increase.
	{ ATM_GEN_PATCH, ID_SND_OFF_1200, _B(ID_SND_OFF_1200, CODE_OFF_1ST_1200),       NULL},            // Send process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_1ST_1200, sizeof(PRC_ID_SND_1200) >> 2,               PRC_ID_SND_1200}, // Send process id code.
	{ ATM_GEN_PATCH, CODE_OFF_1ST_1200 + sizeof(PRC_ID_SND_1200),                                     // Branch back and skip 4 instructions.
		_B(CODE_OFF_1ST_1200 + sizeof(PRC_ID_SND_1200), ID_SND_OFF_1200 + 0x4 * 4), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_1200, _B(ID_RCV_OFF_1200, CODE_OFF_2ND_1200),       NULL},            // Receive process id branch.
	{ ATM_ARR_PATCH, CODE_OFF_2ND_1200, sizeof(PRC_ID_RCV_1200) >> 2,               PRC_ID_RCV_1200}, // Receive process id code.
	{ ATM_GEN_PATCH, CODE_OFF_2ND_1200 + sizeof(PRC_ID_RCV_1200),                                     // Branch back and skip 4 instructions.
		_B(CODE_OFF_2ND_1200 + sizeof(PRC_ID_RCV_1200), ID_RCV_OFF_1200 + 0x4 * 4), NULL}
);

// Kernel sha256 hashes. Offset 0x800 up to INI1 start.
static const pkg2_kernel_id_t _pkg2_kernel_ids[] =
{
	{ "\xb8\xc5\x0c\x68\x25\xa9\xb9\x5b", _kernel_1_patchset },   //  1.0.0
	{ "\x64\x0b\x51\xff\x28\x01\xb8\x30", _kernel_2_patchset },   //  2.0.0 - 2.3.0
	{ "\x50\x84\x23\xac\x6f\xa1\x5d\x3b", _kernel_3_patchset },   //  3.0.0 - 3.0.1
	{ "\x81\x9d\x08\xbe\xe4\x5e\x1f\xbb", _kernel_302_patchset }, //  3.0.2
	{ "\xe6\xc0\xb7\xe3\x2f\xf9\x44\x51", _kernel_4_patchset },   //  4.0.0 - 4.1.0
	{ "\xb2\x38\x61\xa8\xe1\xe2\xe4\xe4", _kernel_5_patchset },   //  5.0.0 - 5.1.0
	{ "\x85\x97\x40\xf6\xc0\x3e\x3d\x44", _kernel_6_patchset },   //  6.0.0 - 6.2.0
	{ "\xa2\x5e\x47\x0c\x8e\x6d\x2f\xd7", _kernel_7_patchset },   //  7.0.0 - 7.0.1
	{ "\xf1\x5e\xc8\x34\xfd\x68\xf0\xf0", _kernel_8_patchset },   //  8.0.0 - 8.1.0. Kernel only.
	{ "\x69\x00\x39\xdf\x21\x56\x70\x6b", _kernel_9_patchset },   //  9.0.0 - 9.1.0. Kernel only.
	{ "\xa2\xe3\xad\x1c\x98\xd8\x7a\x62", _kernel_9_patchset },   //  9.2.0. Kernel only.
	{ "\x21\xc1\xd7\x24\x8e\xcd\xbd\xa8", _kernel_10_patchset },  // 10.0.0 - 10.2.0. Kernel only.
	{ "\xD5\xD0\xBA\x5D\x52\xB9\x77\x85", _kernel_11_patchset },  // 11.0.0. Kernel only.
	{ "\xF8\x1E\xE0\x30\x3C\x7A\x08\x04", _kernel_1101_patchset },// 11.0.1. Kernel only.
	{ "\xA6\xD8\xFF\xF3\x67\x4A\x33\xFC", _kernel_12_patchset },  // 12.0.0 - 12.1.0. Kernel only.
	//! TODO: Mesosphere is now mandatory. Missing patches: 13.0.0+
};

#define KIP1_FS_NOGC_PATCH_SDMMC3 "\x80" // Replace SDMMC2 with SDMMC3.
#define KIP1_FS_NOGC_PATCH_NOINIT "\xE0\x03\x1F\x2A\xC0\x03\x5F\xD6"

// All kip patch offsets are without the 0x100-sized header.
static const kip1_patchset_t _fs_patches_100[] = {
	{ "nogc",     NULL },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_40x[] = {
	{ KPS(KIP_TEXT) | 0xA3459, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ KPS(KIP_TEXT) | 0xAAB44, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_40x[] = {
	{ "nogc",     _fs_nogc_40x },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_410[] = {
	{ KPS(KIP_TEXT) | 0xA34BD, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ KPS(KIP_TEXT) | 0xAABA8, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_410[] = {
	{ "nogc",     _fs_nogc_410 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_50x[] = {
	{ KPS(KIP_TEXT) | 0xCF3C5, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ KPS(KIP_TEXT) | 0xD73A0, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_50x[] = {
	{ "nogc",     _fs_nogc_50x },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_510[] = {
	{ KPS(KIP_TEXT) | 0xCF795, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ KPS(KIP_TEXT) | 0xD7770, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_510[] = {
	{ "nogc",     _fs_nogc_510 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_600[] = {
	{ KPS(KIP_TEXT) | 0x12CC20, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x1538F5, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patch_t _fs_nogc_600_exfat[] = {
	{ KPS(KIP_TEXT) | 0x138320, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x15EFF5, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_600[] = {
	{ "nogc",     _fs_nogc_600 },
	{ NULL, NULL }
};

static const kip1_patchset_t _fs_patches_600_exfat[] = {
	{ "nogc",     _fs_nogc_600_exfat },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_700[] = {
	{ KPS(KIP_TEXT) | 0x134160, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x15BF05, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patch_t _fs_nogc_700_exfat[] = {
	{ KPS(KIP_TEXT) | 0x13F710, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x1674B5, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_700[] = {
	{ "nogc",     _fs_nogc_700 },
	{ NULL, NULL }
};

static const kip1_patchset_t _fs_patches_700_exfat[] = {
	{ "nogc",     _fs_nogc_700_exfat },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_800[] = {
	{ KPS(KIP_TEXT) | 0x136800, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x15EB95, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patch_t _fs_nogc_800_exfat[] = {
	{ KPS(KIP_TEXT) | 0x141DB0, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x16A145, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_800[] = {
	{ "nogc",     _fs_nogc_800 },
	{ NULL, NULL }
};

static const kip1_patchset_t _fs_patches_800_exfat[] = {
	{ "nogc",     _fs_nogc_800_exfat },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_900[] = {
	{ KPS(KIP_TEXT) | 0x129420, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x143269, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_900[] = {
	{ "nogc",     _fs_nogc_900 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_910[] = {
	{ KPS(KIP_TEXT) | 0x129430, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x143279, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_910[] = {
	{ "nogc",     _fs_nogc_910 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1000[] = {
	{ KPS(KIP_TEXT) | 0x13BE90, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x14DE09, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1000[] = {
	{ "nogc",     _fs_nogc_1000 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1020[] = {
	{ KPS(KIP_TEXT) | 0x13C2F0, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x14E269, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1020[] = {
	{ "nogc",     _fs_nogc_1020 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1100[] = {
	{ KPS(KIP_TEXT) | 0x1398B4, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x156EB9, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1100[] = {
	{ "nogc",     _fs_nogc_1100 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1200[] = {
	{ KPS(KIP_TEXT) | 0x13EA24, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x155369, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1200[] = {
	{ "nogc",     _fs_nogc_1200 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1203[] = {
	{ KPS(KIP_TEXT) | 0x13EB34, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x155479, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1203[] = {
	{ "nogc",     _fs_nogc_1203 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1300[] = {
	{ KPS(KIP_TEXT) | 0x1425D0, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x159019, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1300[] = {
	{ "nogc",     _fs_nogc_1300 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1310[] = {
	{ KPS(KIP_TEXT) | 0x142570, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x158FB9, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1310[] = {
	{ "nogc",     _fs_nogc_1310 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1400[] = {
	{ KPS(KIP_TEXT) | 0x164230, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x18A2E9, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1400[] = {
	{ "nogc",     _fs_nogc_1400 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1400_exfat[] = {
	{ KPS(KIP_TEXT) | 0x16F5B0, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x195669, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1400_exfat[] = {
	{ "nogc",     _fs_nogc_1400_exfat },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1500[] = {
	{ KPS(KIP_TEXT) | 0x15ECE4, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x184159, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1500[] = {
	{ "nogc",     _fs_nogc_1500 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1500_exfat[] = {
	{ KPS(KIP_TEXT) | 0x169C74, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x18F0E9, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1500_exfat[] = {
	{ "nogc",     _fs_nogc_1500_exfat },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1600[] = {
	{ KPS(KIP_TEXT) | 0x160B70, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x1865D9, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1600[] = {
	{ "nogc",     _fs_nogc_1600 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1600_exfat[] = {
	{ KPS(KIP_TEXT) | 0x16B850, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x1912B9, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1600_exfat[] = {
	{ "nogc",     _fs_nogc_1600_exfat },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1603[] = {
	{ KPS(KIP_TEXT) | 0x160BC0, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x186629, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1603[] = {
	{ "nogc",     _fs_nogc_1603 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1603_exfat[] = {
	{ KPS(KIP_TEXT) | 0x16B8A0, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x191309, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1603_exfat[] = {
	{ "nogc",     _fs_nogc_1603_exfat },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1700[] = {
	{ KPS(KIP_TEXT) | 0x165100, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x18B049, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1700[] = {
	{ "nogc",     _fs_nogc_1700 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1700_exfat[] = {
	{ KPS(KIP_TEXT) | 0x16FF60, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x195EA9, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1700_exfat[] = {
	{ "nogc",     _fs_nogc_1700_exfat },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1800[] = {
	{ KPS(KIP_TEXT) | 0x164A50, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x18AE49, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1800[] = {
	{ "nogc",     _fs_nogc_1800 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1800_exfat[] = {
	{ KPS(KIP_TEXT) | 0x16FAE0, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x195ED9, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1800_exfat[] = {
	{ "nogc",     _fs_nogc_1800_exfat },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1900[] = {
	{ KPS(KIP_TEXT) | 0x16F070, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x195B75, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ KPS(KIP_TEXT) | 0x195D75, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1900[] = {
	{ "nogc",     _fs_nogc_1900 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_1900_exfat[] = {
	{ KPS(KIP_TEXT) | 0x17A8A0, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x1A13A5, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ KPS(KIP_TEXT) | 0x1A15A5, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_1900_exfat[] = {
	{ "nogc",     _fs_nogc_1900_exfat },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_2000[] = {
	{ KPS(KIP_TEXT) | 0x17C150, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x1A7D25, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ KPS(KIP_TEXT) | 0x1A7F25, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_2000[] = {
	{ "nogc",     _fs_nogc_2000 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_2000_exfat[] = {
	{ KPS(KIP_TEXT) | 0x187A70, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x1B3645, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ KPS(KIP_TEXT) | 0x1B3845, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_2000_exfat[] = {
	{ "nogc",     _fs_nogc_2000_exfat },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_2100[] = {
	{ KPS(KIP_TEXT) | 0x17FAE0, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x1AC8ED, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ KPS(KIP_TEXT) | 0x1AC905, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_2100[] = {
	{ "nogc",     _fs_nogc_2100 },
	{ NULL, NULL }
};

static const kip1_patch_t _fs_nogc_2100_exfat[] = {
	{ KPS(KIP_TEXT) | 0x18AC40, 8, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_NOINIT },
	{ KPS(KIP_TEXT) | 0x1B7A4D, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ KPS(KIP_TEXT) | 0x1B7A65, 1, KIP1_PATCH_SRC_NO_CHECK, KIP1_FS_NOGC_PATCH_SDMMC3 },
	{ 0, 0, NULL, NULL }
};

static const kip1_patchset_t _fs_patches_2100_exfat[] = {
	{ "nogc",     _fs_nogc_2100_exfat },
	{ NULL, NULL }
};

// SHA256 hashes.
static const kip1_id_t _kip_ids[] =
{
	{ "FS", "\xde\x9f\xdd\xa4\x08\x5d\xd5\xfe", _fs_patches_100 },        // FS 1.0.0
	{ "FS", "\xfc\x3e\x80\x99\x1d\xca\x17\x96", _fs_patches_100 },        // FS 1.0.0 exFAT
	{ "FS", "\xcd\x7b\xbe\x18\xd6\x13\x0b\x28", _fs_patches_100 },        // FS 2.0.0
	{ "FS", "\xe7\x66\x92\xdf\xaa\x04\x20\xe9", _fs_patches_100 },        // FS 2.0.0 exFAT
	{ "FS", "\x0d\x70\x05\x62\x7b\x07\x76\x7c", _fs_patches_100 },        // FS 2.1.0
	{ "FS", "\xdb\xd8\x5f\xca\xcc\x19\x3d\xa8", _fs_patches_100 },        // FS 2.1.0 exFAT
	{ "FS", "\xa8\x6d\xa5\xe8\x7e\xf1\x09\x7b", _fs_patches_100 },        // FS 3.0.0
	{ "FS", "\x98\x1c\x57\xe7\xf0\x2f\x70\xf7", _fs_patches_100 },        // FS 3.0.0 exFAT
	{ "FS", "\x57\x39\x7c\x06\x3f\x10\xb6\x31", _fs_patches_100 },        // FS 3.0.1
	{ "FS", "\x07\x30\x99\xd7\xc6\xad\x7d\x89", _fs_patches_100 },        // FS 3.0.1 exFAT
	{ "FS", "\x06\xe9\x07\x19\x59\x5a\x01\x0c", _fs_patches_40x },        // FS 4.0.1
	{ "FS", "\x54\x9b\x0f\x8d\x6f\x72\xc4\xe9", _fs_patches_40x },        // FS 4.0.1 exFAT
	{ "FS", "\x80\x96\xaf\x7c\x6a\x35\xaa\x82", _fs_patches_410 },        // FS 4.1.0
	{ "FS", "\x02\xd5\xab\xaa\xfd\x20\xc8\xb0", _fs_patches_410 },        // FS 4.1.0 exFAT
	{ "FS", "\xa6\xf2\x7a\xd9\xac\x7c\x73\xad", _fs_patches_50x },        // FS 5.0.0
	{ "FS", "\xce\x3e\xcb\xa2\xf2\xf0\x62\xf5", _fs_patches_50x },        // FS 5.0.0 exFAT
	{ "FS", "\x76\xf8\x74\x02\xc9\x38\x7c\x0f", _fs_patches_510 },        // FS 5.1.0
	{ "FS", "\x10\xb2\xd8\x16\x05\x48\x85\x99", _fs_patches_510 },        // FS 5.1.0 exFAT
	{ "FS", "\x1b\x82\xcb\x22\x18\x67\xcb\x52", _fs_patches_600 },        // FS 6.0.0-4.0
	{ "FS", "\x96\x6a\xdd\x3d\x20\xb6\x27\x13", _fs_patches_600_exfat },  // FS 6.0.0-4.0 exFAT
	{ "FS", "\x3a\x57\x4d\x43\x61\x86\x19\x1d", _fs_patches_600 },        // FS 6.0.0-5.0
	{ "FS", "\x33\x05\x53\xf6\xb5\xfb\x55\xc4", _fs_patches_600_exfat },  // FS 6.0.0-5.0 exFAT
	{ "FS", "\x2A\xDB\xE9\x7E\x9B\x5F\x41\x77", _fs_patches_700 },        // FS 7.0.0
	{ "FS", "\x2C\xCE\x65\x9C\xEC\x53\x6A\x8E", _fs_patches_700_exfat },  // FS 7.0.0 exFAT
	{ "FS", "\xB2\xF5\x17\x6B\x35\x48\x36\x4D", _fs_patches_800 },        // FS 8.0.0
	{ "FS", "\xDB\xD9\x41\xC0\xC5\x3C\x52\xCC", _fs_patches_800_exfat },  // FS 8.0.0 exFAT
	{ "FS", "\x6B\x09\xB6\x7B\x29\xC0\x20\x24", _fs_patches_800 },        // FS 8.1.0
	{ "FS", "\xB4\xCA\xE1\xF2\x49\x65\xD9\x2E", _fs_patches_800_exfat },  // FS 8.1.0 exFAT
	{ "FS", "\x46\x87\x40\x76\x1E\x19\x3E\xB7", _fs_patches_900 },        // FS 9.0.0
	{ "FS", "\x7C\x95\x13\x76\xE5\xC1\x2D\xF8", _fs_patches_900 },        // FS 9.0.0 exFAT
	{ "FS", "\xB5\xE7\xA6\x4C\x6F\x5C\x4F\xE3", _fs_patches_910 },        // FS 9.1.0
	{ "FS", "\xF1\x96\xD1\x44\xD0\x44\x45\xB6", _fs_patches_910 },        // FS 9.1.0 exFAT
	{ "FS", "\x3E\xEB\xD9\xB7\xBC\xD1\xB5\xE0", _fs_patches_1000 },       // FS 10.0.0
	{ "FS", "\x81\x7E\xA2\xB0\xB7\x02\xC1\xF3", _fs_patches_1000 },       // FS 10.0.0 exFAT
	{ "FS", "\xA9\x52\xB6\x57\xAD\xF9\xC2\xBA", _fs_patches_1020 },       // FS 10.2.0
	{ "FS", "\x16\x0D\x3E\x10\x4E\xAD\x61\x76", _fs_patches_1020 },       // FS 10.2.0 exFAT
	{ "FS", "\xE3\x99\x15\x6E\x84\x4E\xB0\xAA", _fs_patches_1100 },       // FS 11.0.0
	{ "FS", "\x0B\xA1\x5B\xB3\x04\xB5\x05\x63", _fs_patches_1100 },       // FS 11.0.0 exFAT
	{ "FS", "\xDC\x2A\x08\x49\x96\xBB\x3C\x01", _fs_patches_1200 },       // FS 12.0.0
	{ "FS", "\xD5\xA5\xBF\x36\x64\x0C\x49\xEA", _fs_patches_1200 },       // FS 12.0.0 exFAT
	{ "FS", "\xC8\x67\x62\xBE\x19\xA5\x1F\xA0", _fs_patches_1203 },       // FS 12.0.3
	{ "FS", "\xE1\xE8\xD3\xD6\xA2\xFE\x0B\x10", _fs_patches_1203 },       // FS 12.0.3 exFAT
	{ "FS", "\x7D\x20\x05\x47\x17\x8A\x83\x6A", _fs_patches_1300 },       // FS 13.0.0
	{ "FS", "\x51\xEB\xFA\x9C\xCF\x66\xC0\x9E", _fs_patches_1300 },       // FS 13.0.0 exFAT
	{ "FS", "\x91\xBA\x65\xA2\x1C\x1D\x50\xAE", _fs_patches_1310 },       // FS 13.1.0
	{ "FS", "\x76\x38\x27\xEE\x9C\x20\x7E\x5B", _fs_patches_1310 },       // FS 13.1.0 exFAT
	{ "FS", "\x88\x7A\xC1\x50\x80\x6C\x75\xCC", _fs_patches_1400 },       // FS 14.0.0
	{ "FS", "\xD4\x88\xD1\xF2\x92\x17\x35\x5C", _fs_patches_1400_exfat }, // FS 14.0.0 exFAT
	{ "FS", "\xD0\xD4\x49\x18\x14\xB5\x62\xAF", _fs_patches_1500 },       // FS 15.0.0
	{ "FS", "\x34\xC0\xD9\xED\x6A\xD1\x87\x3D", _fs_patches_1500_exfat }, // FS 15.0.0 exFAT
	{ "FS", "\x56\xE8\x56\x56\x6C\x38\xD8\xBE", _fs_patches_1600 },       // FS 16.0.0
	{ "FS", "\xCF\xAB\x45\x0C\x2C\x53\x9D\xA9", _fs_patches_1600_exfat }, // FS 16.0.0 exFAT
	{ "FS", "\x39\xEE\x1F\x1E\x0E\xA7\x32\x5D", _fs_patches_1603 },       // FS 16.0.3
	{ "FS", "\x62\xC6\x5E\xFD\x9A\xBF\x7C\x43", _fs_patches_1603_exfat }, // FS 16.0.3 exFAT
	{ "FS", "\x27\x07\x3B\xF0\xA1\xB8\xCE\x61", _fs_patches_1700 },       // FS 17.0.0
	{ "FS", "\xEE\x0F\x4B\xAC\x6D\x1F\xFC\x4B", _fs_patches_1700_exfat }, // FS 17.0.0 exFAT
	{ "FS", "\x79\x5F\x5A\x5E\xB0\xC6\x77\x9E", _fs_patches_1800 },       // FS 18.0.0
	{ "FS", "\x1E\x2C\x64\xB1\xCC\xE2\x78\x24", _fs_patches_1800_exfat }, // FS 18.0.0 exFAT
	{ "FS", "\xA3\x39\xF0\x1C\x95\xBF\xA7\x68", _fs_patches_1800 },       // FS 18.1.0
	{ "FS", "\x20\x4C\xBA\x86\xDE\x08\x44\x6A", _fs_patches_1800_exfat }, // FS 18.1.0 exFAT
	{ "FS", "\xD9\x4C\x68\x15\xF8\xF5\x0A\x20", _fs_patches_1900 },       // FS 19.0.0
	{ "FS", "\xED\xA8\x78\x68\xA4\x49\x07\x50", _fs_patches_1900_exfat }, // FS 19.0.0 exFAT
	{ "FS", "\x63\x54\x96\x9E\x60\xA7\x97\x7B", _fs_patches_2000 },       // FS 20.0.0
	{ "FS", "\x47\x41\x07\x10\x65\x4F\xA4\x3F", _fs_patches_2000_exfat }, // FS 20.0.0 exFAT
	{ "FS", "\xED\x34\xB4\x50\x58\x4A\x5B\x43", _fs_patches_2000 },       // FS 20.1.0
	{ "FS", "\xA5\x1A\xA4\x92\x6C\x41\x87\x59", _fs_patches_2000_exfat }, // FS 20.1.0 exFAT
	{ "FS", "\xEE\x4B\x30\x12\xA6\x84\x02\x25", _fs_patches_2100 },       // FS 21.0.0
	{ "FS", "\x6E\x2B\xD9\xBA\xA3\xB9\x10\xF1", _fs_patches_2100_exfat }, // FS 21.0.0 exFAT
};
