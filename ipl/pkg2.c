/*
* Copyright (c) 2018 naehrwert
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
#include "heap.h"
#include "se.h"

/*#include "gfx.h"
extern gfx_ctxt_t gfx_ctxt;
extern gfx_con_t gfx_con;
#define DPRINTF(...) gfx_printf(&gfx_con, __VA_ARGS__)*/
#define DPRINTF(...)

#define _MOVZX(r, i, s) 0xD2800000 | (((s) & 0x30) << 17) | (((i) & 0xFFFF) << 5) | ((r) & 0x1F)
#define _NOP() 0xD503201F

// Include kernel patches here, so we can utilize pkg1 id
PATCHSET_DEF(_kernel_1_patchset,
	{ 0x3764C, _NOP() },         // Disable SVC verifications
	{ 0x44074, _MOVZX(8, 1, 0) } // Enable Debug Patch
);

PATCHSET_DEF(_kernel_2_patchset,
	{ 0x54834, _NOP() },         // Disable SVC verifications
	{ 0x6086C, _MOVZX(8, 1, 0) } // Enable Debug Patch
);

PATCHSET_DEF(_kernel_3_patchset,
	{ 0x3BD24, _NOP() },         // Disable SVC verifications
	{ 0x483FC, _MOVZX(8, 1, 0) } // Enable Debug Patch
);

PATCHSET_DEF(_kernel_302_patchset,
	{ 0x3BD24, _NOP() },         // Disable SVC verifications
	{ 0x48414, _MOVZX(8, 1, 0) } // Enable Debug Patch
);

PATCHSET_DEF(_kernel_4_patchset,
	{ 0x41EB4, _NOP() },         // Disable SVC verifications
	{ 0x4EBFC, _MOVZX(8, 1, 0) } // Enable Debug Patch
);

PATCHSET_DEF(_kernel_5_patchset,
	{ 0x45E6C, _NOP() },         // Disable SVC verifications
	{ 0x5513C, _MOVZX(8, 1, 0) } // Enable Debug Patch
);

static const pkg2_kernel_id_t _pkg2_kernel_ids[] = {
	{ 0x427f2647, _kernel_1_patchset },   //1.0.0
	{ 0xae19cf1b, _kernel_2_patchset },   //2.0.0 - 2.3.0
	{ 0x73c9e274, _kernel_3_patchset },   //3.0.0 - 3.0.1
	{ 0xe0e8cdc4, _kernel_302_patchset }, //3.0.2
	{ 0x485d0157, _kernel_4_patchset },   //4.0.0 - 4.1.0
	{ 0xf3c363f2, _kernel_5_patchset },   //5.0.0 - 5.1.0
	{ 0, 0 } //End.
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

