/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 CTCaer
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
#include "hos.h"

#include "../libs/fatfs/ff.h"
#include "../utils/aarch64_util.h"
#include "../mem/heap.h"
#include "../sec/se.h"
#include "../libs/compr/blz.h"

#include "../gfx/gfx.h"

extern const u8 package2_keyseed[];

/*#include "util.h"
#define DPRINTF(...) gfx_printf(__VA_ARGS__)
#define DEBUG_PRINTING*/
#define DPRINTF(...)

u32 pkg2_calc_kip1_size(pkg2_kip1_t *kip1)
{
	u32 size = sizeof(pkg2_kip1_t);
	for (u32 j = 0; j < KIP1_NUM_SECTIONS; j++)
		size += kip1->sections[j].size_comp;
	return size;
}

void pkg2_get_newkern_info(u8 *kern_data)
{
	u32 info_op = *(u32 *)(kern_data + PKG2_NEWKERN_GET_INI1);
	pkg2_newkern_ini1_val = ((info_op & 0xFFFF) >> 3) + PKG2_NEWKERN_GET_INI1; // Parse ADR and PC.

	pkg2_newkern_ini1_start = *(u32 *)(kern_data + pkg2_newkern_ini1_val);
	pkg2_newkern_ini1_end   = *(u32 *)(kern_data + pkg2_newkern_ini1_val + 0x8);
}

void pkg2_parse_kips(link_t *info, pkg2_hdr_t *pkg2, bool *new_pkg2)
{
	u8 *ptr;
	// Check for new pkg2 type.
	if (!pkg2->sec_size[PKG2_SEC_INI1])
	{
		pkg2_get_newkern_info(pkg2->data);

		ptr = pkg2->data + pkg2_newkern_ini1_start;
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
		ki->size = pkg2_calc_kip1_size(kip1);
		list_append(info, &ki->link);
		ptr += ki->size;
DPRINTF(" kip1 %d:%s @ %08X (%08X)\n", i, kip1->name, (u32)kip1, ki->size);
	}
}

static const u8 mkey_keyseed_8xx[][0x10] =
{
	{0x4D, 0xD9, 0x98, 0x42, 0x45, 0x0D, 0xB1, 0x3C, 0x52, 0x0C, 0x9A, 0x44, 0xBB, 0xAD, 0xAF, 0x80} // Master key 8 encrypted with 9.
};

pkg2_hdr_t *pkg2_decrypt(void *data, u8 kb)
{
	pkg2_hdr_t mkey_test;
	u8 *pdata = (u8 *)data;
	u8 keyslot = 8;

	// Skip signature.
	pdata += 0x100;

	pkg2_hdr_t *hdr = (pkg2_hdr_t *)pdata;

	// Skip header.
	pdata += sizeof(pkg2_hdr_t);

	//! Check if we need to decrypt with newer mkeys. Valid for 8.1.0 and up.
	if ((kb >= KB_FIRMWARE_VERSION_810) && (kb < KB_FIRMWARE_VERSION_MAX))
	{
		u8 tmp_mkey[0x10];
		// Decrypt older encrypted mkey.
		se_aes_crypt_ecb(12, 0, tmp_mkey, 0x10, mkey_keyseed_8xx[KB_FIRMWARE_VERSION_MAX - kb - 1], 0x10);
		// Set and unwrap pkg2 key.
		se_aes_key_set(9, tmp_mkey, 0x10);
		se_aes_unwrap_key(9, 9, package2_keyseed);

		// Decrypt header and test if it's valid.
		se_aes_crypt_ctr(9, &mkey_test, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);

		if (mkey_test.magic == PKG2_MAGIC)
			keyslot = 9;
		else
			se_aes_key_clear(9);
	}

	// Decrypt header.
	se_aes_crypt_ctr(keyslot, hdr, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);
	//gfx_hexdump((u32)hdr, hdr, 0x100);

	if (hdr->magic != PKG2_MAGIC)
		return NULL;

	for (u32 i = 0; i < 4; i++)
	{
DPRINTF("sec %d has size %08X\n", i, hdr->sec_size[i]);
		if (!hdr->sec_size[i])
			continue;

		se_aes_crypt_ctr(keyslot, pdata, hdr->sec_size[i], pdata, hdr->sec_size[i], &hdr->sec_ctr[i * 0x10]);
		//gfx_hexdump((u32)pdata, pdata, 0x100);

		pdata += hdr->sec_size[i];
	}

	if (keyslot != 8)
		se_aes_key_clear(9);

	return hdr;
}
