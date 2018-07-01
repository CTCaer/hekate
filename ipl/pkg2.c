/*
* Copyright (c) 2018 naehrwert
* Copyright (c) 2018 CTCaer
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
#include "arm64.h"
#include "heap.h"
#include "se.h"

/*#include "gfx.h"
extern gfx_ctxt_t gfx_ctxt;
extern gfx_con_t gfx_con;
#define DPRINTF(...) gfx_printf(&gfx_con, __VA_ARGS__)*/
#define DPRINTF(...)

//TODO: Replace hardcoded AArch64 instructions with instruction macros.
//TODO: Reduce hardcoded values without searching kernel for patterns?
// The process ID send/receive kernel patches were taken from Atmosphère's kernel patches.
// They should only be used when running Atmosphère.
#define FREE_CODE_OFF_1ST_100 0x4797C
#define FREE_CODE_OFF_1ST_200 0x6486C
#define FREE_CODE_OFF_1ST_300 0x494A4
#define FREE_CODE_OFF_1ST_302 0x494BC
#define FREE_CODE_OFF_1ST_400 0x4FBC0
#define FREE_CODE_OFF_1ST_500 0x5C020

#define ID_SND_OFF_100 0x23CC0
#define ID_SND_OFF_200 0x3F134
#define ID_SND_OFF_300 0x26080
#define ID_SND_OFF_302 0x26080
#define ID_SND_OFF_400 0x2AF64
#define ID_SND_OFF_500 0x2AD34

#define ID_RCV_OFF_100 0x219F0
#define ID_RCV_OFF_200 0x3D1A8
#define ID_RCV_OFF_300 0x240F0
#define ID_RCV_OFF_302 0x240F0
#define ID_RCV_OFF_400 0x28F6C
#define ID_RCV_OFF_500 0x28DAC

static u32 PRC_ID_SND_100[] =
{
	0xA9BF2FEA, 0x2A0E03EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B,
	0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9412948,	0xA8C12FEA
};
#define FREE_CODE_OFF_2ND_100 (FREE_CODE_OFF_1ST_100 + sizeof(PRC_ID_SND_100) + 4)
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
#define FREE_CODE_OFF_2ND_200 (FREE_CODE_OFF_1ST_200 + sizeof(PRC_ID_SND_200) + 4)
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
#define FREE_CODE_OFF_2ND_300 (FREE_CODE_OFF_1ST_300 + sizeof(PRC_ID_SND_300) + 4)
static u32 PRC_ID_RCV_300[] =
{
	0xA9BF2FEA, 0x2A0F03EA, 0xD37EF54A, 0xF9405FEB, 0xF86A696A, 0xF9407BEB, 0x92FFFFE9, 0x8A090148,
	0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415568, 0xA8C12FEA
};

static u32 PRC_ID_SND_302[] =
{
	0xA9BF2FEA, 0x2A1803EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B,
	0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9415548, 0xA8C12FEA
};
#define FREE_CODE_OFF_2ND_302 (FREE_CODE_OFF_1ST_302 + sizeof(PRC_ID_SND_302) + 4)
static u32 PRC_ID_RCV_302[] =
{
	0xA9BF2FEA, 0x2A0F03EA, 0xD37EF54A, 0xF9405FEB, 0xF86A696A, 0xF9407BEB, 0x92FFFFE9, 0x8A090148,
	0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415568, 0xA8C12FEA
};

static u32 PRC_ID_SND_400[] =
{
	0xF9403BED, 0x2A0E03EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A,
	0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415B28, 0xD503201F
};
#define FREE_CODE_OFF_2ND_400 (FREE_CODE_OFF_1ST_400 + sizeof(PRC_ID_SND_400) + 4)
static u32 PRC_ID_RCV_400[] =
{
	0xD280000D, 0x2A0E03ED, 0xD37EF5AD, 0xF86D6B4D, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A0901AD,
	0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415B28, 0xD503201F
};

static u32 PRC_ID_SND_500[] =
{
	0x2A1703EA, 0xD37EF54A, 0xF86A6B6A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9,
	0xEB09015F, 0x54000060, 0xF94043EA, 0xF9415948, 0xF94043EA
};
#define FREE_CODE_OFF_2ND_500 (FREE_CODE_OFF_1ST_500 + sizeof(PRC_ID_SND_500) + 4)
static u32 PRC_ID_RCV_500[] =
{
	0xF9403BED, 0x2A1503EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A,
	0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415B08, 0xF9406FEA
};

// Include kernel patches here, so we can utilize pkg1 id
KERNEL_PATCHSET_DEF(_kernel_1_patchset,
	{ SVC_VERIFY_DS, 0x3764C, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x44074, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_100, _B(ID_SND_OFF_100, FREE_CODE_OFF_1ST_100), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_100, sizeof(PRC_ID_SND_100) >> 2, PRC_ID_SND_100}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_100 + sizeof(PRC_ID_SND_100),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_1ST_100 + sizeof(PRC_ID_SND_100), ID_SND_OFF_100 + 4), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_100, _B(ID_RCV_OFF_100, FREE_CODE_OFF_2ND_100), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_100, sizeof(PRC_ID_RCV_100) >> 2, PRC_ID_RCV_100}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_100 + sizeof(PRC_ID_RCV_100),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_2ND_100 + sizeof(PRC_ID_RCV_100), ID_RCV_OFF_100 + 4), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_2_patchset,
	{ SVC_VERIFY_DS, 0x54834, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x6086C, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_200, _B(ID_SND_OFF_200, FREE_CODE_OFF_1ST_200), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_200, sizeof(PRC_ID_SND_200) >> 2, PRC_ID_SND_200}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_200 + sizeof(PRC_ID_SND_200),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_1ST_200 + sizeof(PRC_ID_SND_200), ID_SND_OFF_200 + 4), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_200, _B(ID_RCV_OFF_200, FREE_CODE_OFF_2ND_200), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_200, sizeof(PRC_ID_RCV_200) >> 2, PRC_ID_RCV_200}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_200 + sizeof(PRC_ID_RCV_200),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_2ND_200 + sizeof(PRC_ID_RCV_200), ID_RCV_OFF_200 + 4), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_3_patchset,
	{ SVC_VERIFY_DS, 0x3BD24, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x483FC, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_300, _B(ID_SND_OFF_300, FREE_CODE_OFF_1ST_300), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_300, sizeof(PRC_ID_SND_300) >> 2, PRC_ID_SND_300}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_300 + sizeof(PRC_ID_SND_300),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_1ST_300 + sizeof(PRC_ID_SND_300), ID_SND_OFF_300 + 4), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_300, _B(ID_RCV_OFF_300, FREE_CODE_OFF_2ND_300), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_300, sizeof(PRC_ID_RCV_300) >> 2, PRC_ID_RCV_300}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_300 + sizeof(PRC_ID_RCV_300),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_2ND_300 + sizeof(PRC_ID_RCV_300), ID_RCV_OFF_300 + 4), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_302_patchset,
	{ SVC_VERIFY_DS, 0x3BD24, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x48414, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_302, _B(ID_SND_OFF_302, FREE_CODE_OFF_1ST_302), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_302, sizeof(PRC_ID_SND_302) >> 2, PRC_ID_SND_302}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_302 + sizeof(PRC_ID_SND_302),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_1ST_302 + sizeof(PRC_ID_SND_302), ID_SND_OFF_302 + 4), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_302, _B(ID_RCV_OFF_302, FREE_CODE_OFF_2ND_302), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_302, sizeof(PRC_ID_RCV_302) >> 2, PRC_ID_RCV_302}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_302 + sizeof(PRC_ID_RCV_302),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_2ND_302 + sizeof(PRC_ID_RCV_302), ID_RCV_OFF_302 + 4), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_4_patchset,
	{ SVC_VERIFY_DS, 0x41EB4, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x4EBFC, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_400, _B(ID_SND_OFF_400, FREE_CODE_OFF_1ST_400), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_400, sizeof(PRC_ID_SND_400) >> 2, PRC_ID_SND_400}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_400 + sizeof(PRC_ID_SND_400),                      // Branch back and skip 2 instructions.
		_B(FREE_CODE_OFF_1ST_400 + sizeof(PRC_ID_SND_400), ID_SND_OFF_400 + 8), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_400, _B(ID_RCV_OFF_400, FREE_CODE_OFF_2ND_400), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_400, sizeof(PRC_ID_RCV_400) >> 2, PRC_ID_RCV_400}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_400 + sizeof(PRC_ID_RCV_400),                      // Branch back and skip 1 instruction.
		_B(FREE_CODE_OFF_2ND_400 + sizeof(PRC_ID_RCV_400), ID_RCV_OFF_400 + 4), NULL}
);

KERNEL_PATCHSET_DEF(_kernel_5_patchset,
	{ SVC_VERIFY_DS, 0x45E6C, _NOP(), NULL },          // Disable SVC verifications
	{ DEBUG_MODE_EN, 0x5513C, _MOVZX(8, 1, 0), NULL }, // Enable Debug Patch
	// Atmosphère kernel patches.
	{ ATM_GEN_PATCH, ID_SND_OFF_500, _B(ID_SND_OFF_500, FREE_CODE_OFF_1ST_500), NULL},    // Send process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_1ST_500, sizeof(PRC_ID_SND_500) >> 2, PRC_ID_SND_500}, // Send process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_1ST_500 + sizeof(PRC_ID_SND_500),                      // Branch back and skip 2 instructions.
		_B(FREE_CODE_OFF_1ST_500 + sizeof(PRC_ID_SND_500), ID_SND_OFF_500 + 8), NULL},
	{ ATM_GEN_PATCH, ID_RCV_OFF_500, _B(ID_RCV_OFF_500, FREE_CODE_OFF_2ND_500), NULL},    // Receive process id branch.
	{ ATM_ARR_PATCH, FREE_CODE_OFF_2ND_500, sizeof(PRC_ID_RCV_500) >> 2, PRC_ID_RCV_500}, // Receive process id code.
	{ ATM_GEN_PATCH, FREE_CODE_OFF_2ND_500 + sizeof(PRC_ID_RCV_500),                      // Branch back and skip 2 instructions.
		_B(FREE_CODE_OFF_2ND_500 + sizeof(PRC_ID_RCV_500), ID_RCV_OFF_500 + 8), NULL}
);

static const pkg2_kernel_id_t _pkg2_kernel_ids[] =
{
	{ 0x427f2647, _kernel_1_patchset },   //1.0.0
	{ 0xae19cf1b, _kernel_2_patchset },   //2.0.0 - 2.3.0
	{ 0x73c9e274, _kernel_3_patchset },   //3.0.0 - 3.0.1
	{ 0xe0e8cdc4, _kernel_302_patchset }, //3.0.2
	{ 0x485d0157, _kernel_4_patchset },   //4.0.0 - 4.1.0
	{ 0xf3c363f2, _kernel_5_patchset },   //5.0.0 - 5.1.0
	{ 0, 0 }                              //End.
};

const pkg2_kernel_id_t *pkg2_identify(u32 id)
{
	for (u32 i = 0; _pkg2_kernel_ids[i].crc32c_id; i++)
		if (id == _pkg2_kernel_ids[i].crc32c_id)
			return &_pkg2_kernel_ids[i];
	return NULL;
}

static u32 _pkg2_calc_kip1_size(pkg2_kip1_t *kip1)
{
	u32 size = sizeof(pkg2_kip1_t);
	for (u32 j = 0; j < KIP1_NUM_SECTIONS; j++)
		size += kip1->sections[j].size_comp;
	return size;
}

void pkg2_parse_kips(link_t *info, pkg2_hdr_t *pkg2)
{
	u8 *ptr = pkg2->data + pkg2->sec_size[PKG2_SEC_KERNEL];
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
		if (ki->kip1->tid == tid)
		{
			ki->kip1 = kip1;
			ki->size = _pkg2_calc_kip1_size(kip1);
DPRINTF("replaced kip (new size %08X)\n", ki->size);
			return;
		}
}

void pkg2_add_kip(link_t *info, pkg2_kip1_t *kip1)
{
	pkg2_kip1_info_t *ki = (pkg2_kip1_info_t *)malloc(sizeof(pkg2_kip1_info_t));
	ki->kip1 = kip1;
	ki->size = _pkg2_calc_kip1_size(kip1);
DPRINTF("added kip (size %08X)\n", ki->size);
	list_append(info, &ki->link);
}

void pkg2_merge_kip(link_t *info, pkg2_kip1_t *kip1)
{
	if (pkg2_has_kip(info, kip1->tid))
		pkg2_replace_kip(info, kip1->tid, kip1);
	else
		pkg2_add_kip(info, kip1);
}

pkg2_hdr_t *pkg2_decrypt(void *data)
{
	u8 *pdata = (u8 *)data;
	
	//Skip signature.
	pdata += 0x100;

	pkg2_hdr_t *hdr = (pkg2_hdr_t *)pdata;

	//Skip header.
	pdata += sizeof(pkg2_hdr_t);

	//Decrypt header.
	se_aes_crypt_ctr(8, hdr, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);
	//gfx_hexdump(&gfx_con, (u32)hdr, hdr, 0x100);

	if (hdr->magic != PKG2_MAGIC)
		return NULL;

	for (u32 i = 0; i < 4; i++)
	{
DPRINTF("sec %d has size %08X\n", i, hdr->sec_size[i]);
		if (!hdr->sec_size[i])
			continue;

		se_aes_crypt_ctr(8, pdata, hdr->sec_size[i], pdata, hdr->sec_size[i], &hdr->sec_ctr[i * 0x10]);
		//gfx_hexdump(&gfx_con, (u32)pdata, pdata, 0x100);

		pdata += hdr->sec_size[i];
	}

	return hdr;
}

void pkg2_build_encrypt(void *dst, void *kernel, u32 kernel_size, link_t *kips_info)
{
	u8 *pdst = (u8 *)dst;

	//Signature.
	memset(pdst, 0, 0x100);
	pdst += 0x100;

	//Header.
	pkg2_hdr_t *hdr = (pkg2_hdr_t *)pdst;
	memset(hdr, 0, sizeof(pkg2_hdr_t));
	pdst += sizeof(pkg2_hdr_t);
	hdr->magic = PKG2_MAGIC;
	hdr->base = 0x10000000;
DPRINTF("kernel @ %08X (%08X)\n", (u32)kernel, kernel_size);

	//Kernel.
	memcpy(pdst, kernel, kernel_size);
	hdr->sec_size[PKG2_SEC_KERNEL] = kernel_size;
	hdr->sec_off[PKG2_SEC_KERNEL] = 0x10000000;
	se_aes_crypt_ctr(8, pdst, kernel_size, pdst, kernel_size, &hdr->sec_ctr[PKG2_SEC_KERNEL * 0x10]);
	pdst += kernel_size;
DPRINTF("kernel encrypted\n");

	//INI1.
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
	ini1->size = ini1_size;
	hdr->sec_size[PKG2_SEC_INI1] = ini1_size;
	hdr->sec_off[PKG2_SEC_INI1] = 0x14080000;
	se_aes_crypt_ctr(8, ini1, ini1_size, ini1, ini1_size, &hdr->sec_ctr[PKG2_SEC_INI1 * 0x10]);
DPRINTF("INI1 encrypted\n");

	//Encrypt header.
	*(u32 *)hdr->ctr = 0x100 + sizeof(pkg2_hdr_t) + kernel_size + ini1_size;
	se_aes_crypt_ctr(8, hdr, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);
	memset(hdr->ctr, 0 , 0x10);
	*(u32 *)hdr->ctr = 0x100 + sizeof(pkg2_hdr_t) + kernel_size + ini1_size;
}

