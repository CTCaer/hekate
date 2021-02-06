/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2020 CTCaer
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

#include "../config.h"
#include <libs/fatfs/ff.h>
#include <mem/heap.h>
#include <sec/se.h>
#include <sec/se_t210.h>
#include <libs/compr/blz.h>

#include <gfx_utils.h>

extern hekate_config h_cfg;
extern const u8 package2_keyseed[];

u32 pkg2_newkern_ini1_val;
u32 pkg2_newkern_ini1_start;
u32 pkg2_newkern_ini1_end;

/*#include <utils/util.h>
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
	u32 pkg2_newkern_ini1_off = 0;
	pkg2_newkern_ini1_start = 0;

	// Find static OP offset that is close to INI1 offset.
	u32 counter_ops = 0x100;
	while (counter_ops)
	{
		if (*(u32 *)(kern_data + 0x100 - counter_ops) == PKG2_NEWKERN_GET_INI1_HEURISTIC)
		{
			pkg2_newkern_ini1_off = 0x100 - counter_ops + 12; // OP found. Add 12 for the INI1 offset.
			break;
		}

		counter_ops -= 4;
	}

	// Offset not found?
	if (!counter_ops)
		return;

	u32 info_op = *(u32 *)(kern_data + pkg2_newkern_ini1_off);
	pkg2_newkern_ini1_val = ((info_op & 0xFFFF) >> 3) + pkg2_newkern_ini1_off; // Parse ADR and PC.

	pkg2_newkern_ini1_start = *(u32 *)(kern_data + pkg2_newkern_ini1_val);
	pkg2_newkern_ini1_end   = *(u32 *)(kern_data + pkg2_newkern_ini1_val + 0x8);
}

bool pkg2_parse_kips(link_t *info, pkg2_hdr_t *pkg2, bool *new_pkg2)
{
	u8 *ptr;
	// Check for new pkg2 type.
	if (!pkg2->sec_size[PKG2_SEC_INI1])
	{
		pkg2_get_newkern_info(pkg2->data);

		if (!pkg2_newkern_ini1_start)
			return false;

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

	return true;
}

static const u8 mkey_vector_8xx[][SE_KEY_128_SIZE] =
{
	// Master key 8 encrypted with 9.  (8.1.0 with 9.0.0)
	{ 0x4D, 0xD9, 0x98, 0x42, 0x45, 0x0D, 0xB1, 0x3C, 0x52, 0x0C, 0x9A, 0x44, 0xBB, 0xAD, 0xAF, 0x80 },
	// Master key 9 encrypted with 10. (9.0.0 with 9.1.0)
	{ 0xB8, 0x96, 0x9E, 0x4A, 0x00, 0x0D, 0xD6, 0x28, 0xB3, 0xD1, 0xDB, 0x68, 0x5F, 0xFB, 0xE1, 0x2A }
};

static bool _pkg2_key_unwrap_validate(pkg2_hdr_t *tmp_test, pkg2_hdr_t *hdr, u8 src_slot, u8 *mkey, const u8 *key_seed)
{
	// Decrypt older encrypted mkey.
	se_aes_crypt_ecb(src_slot, 0, mkey, SE_KEY_128_SIZE, key_seed, SE_KEY_128_SIZE);
	// Set and unwrap pkg2 key.
	se_aes_key_clear(9);
	se_aes_key_set(9, mkey, SE_KEY_128_SIZE);
	se_aes_unwrap_key(9, 9, package2_keyseed);

	// Decrypt header.
	se_aes_crypt_ctr(9, tmp_test, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);

	// Return if header is valid.
	return (tmp_test->magic == PKG2_MAGIC);
}

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

	// Check if we need to decrypt with newer mkeys. Valid for sept for 8.1.0 and up.
	se_aes_crypt_ctr(8, &mkey_test, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);

	if (mkey_test.magic == PKG2_MAGIC)
		goto key_found;

	// Decrypt older pkg2 via new mkeys.
	if ((kb >= KB_FIRMWARE_VERSION_810) && (kb < KB_FIRMWARE_VERSION_MAX))
	{
		u8 tmp_mkey[SE_KEY_128_SIZE];
		u8 decr_slot = !h_cfg.t210b01 ? (!h_cfg.aes_slots_new ? 12 : 13) : 7; // Sept mkey or T210B01 mkey.
		u8 mkey_seeds_cnt = sizeof(mkey_vector_8xx) / SE_KEY_128_SIZE;
		u8 mkey_seeds_idx = mkey_seeds_cnt; // Real index + 1.
		u8 mkey_seeds_min_idx = mkey_seeds_cnt - (KB_FIRMWARE_VERSION_MAX - kb);

		while (mkey_seeds_cnt)
		{
			// Decrypt and validate mkey.
			int res = _pkg2_key_unwrap_validate(&mkey_test, hdr, decr_slot,
				tmp_mkey, mkey_vector_8xx[mkey_seeds_idx - 1]);

			if (res)
			{
				keyslot = 9;
				goto key_found;
			}
			else
			{
				// Set current mkey in order to decrypt a lower mkey.
				mkey_seeds_idx--;
				se_aes_key_clear(9);
				se_aes_key_set(9, tmp_mkey, SE_KEY_128_SIZE);

				decr_slot = 9; // Temp key.

				// Check if we tried last key for that pkg2 version.
				// And start with a lower mkey in case sept is older.
				if (mkey_seeds_idx == mkey_seeds_min_idx)
				{
					mkey_seeds_cnt--;
					mkey_seeds_idx = mkey_seeds_cnt;
					decr_slot = !h_cfg.aes_slots_new ? 12 : 13; // Sept mkey.
				}

				// Out of keys. pkg2 is latest or process failed.
				if (!mkey_seeds_cnt)
					se_aes_key_clear(9);
			}
		}
	}

key_found:
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

		se_aes_crypt_ctr(keyslot, pdata, hdr->sec_size[i], pdata, hdr->sec_size[i], &hdr->sec_ctr[i * SE_AES_IV_SIZE]);
		//gfx_hexdump((u32)pdata, pdata, 0x100);

		pdata += hdr->sec_size[i];
	}

	if (keyslot != 8)
		se_aes_key_clear(9);

	return hdr;
}
