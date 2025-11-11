/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2025 CTCaer
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

#include <bdk.h>

#include "pkg2.h"
#include "hos.h"

#include "../config.h"
#include <libs/fatfs/ff.h>
#include <libs/compr/blz.h>

extern const u8 package2_keyseed[];

u32 pkg2_newkern_ini1_start;
u32 pkg2_newkern_ini1_end;

/*#define DPRINTF(...) gfx_printf(__VA_ARGS__)
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
	u32 crt_start = 0;
	u32 pkg2_newkern_ini1_info = 0;
	pkg2_newkern_ini1_start    = 0;

	u32 first_op = *(u32 *)kern_data;
	if ((first_op & 0xFE000000) == 0x14000000)
		crt_start = (first_op & 0x1FFFFFF) << 2;

	// Find static OP offset that is close to INI1 offset.
	u32 counter_ops = 0x100;
	while (counter_ops)
	{
		if (*(u32 *)(kern_data + crt_start + 0x100 - counter_ops) == PKG2_NEWKERN_GET_INI1_HEURISTIC)
		{
			// OP found. Add 12 for the INI1 info offset.
			pkg2_newkern_ini1_info = crt_start + 0x100 - counter_ops + 12;

			// On v2 kernel with dynamic crt there's a NOP after heuristic. Offset one op.
			if (crt_start)
				pkg2_newkern_ini1_info += 4;
			break;
		}

		counter_ops -= 4;
	}

	// Offset not found?
	if (!counter_ops)
		return;

	u32 info_op = *(u32 *)(kern_data + pkg2_newkern_ini1_info);
	pkg2_newkern_ini1_info += ((info_op & 0xFFFF) >> 3); // Parse ADR and PC.

	pkg2_newkern_ini1_start = *(u32 *)(kern_data + pkg2_newkern_ini1_info);
	pkg2_newkern_ini1_end   = *(u32 *)(kern_data + pkg2_newkern_ini1_info + 0x8);

	// On v2 kernel with dynamic crt, values are relative to value address.
	if (crt_start)
	{
		pkg2_newkern_ini1_start += pkg2_newkern_ini1_info;
		pkg2_newkern_ini1_end   += pkg2_newkern_ini1_info + 0x8;
	}
}

//!TODO: Update on mkey changes.
static const u8 mkey_vector_7xx[HOS_MKEY_VER_MAX - HOS_MKEY_VER_810 + 1][SE_KEY_128_SIZE] =
{
	// Master key 7  encrypted with 8.  (7.0.0 with 8.1.0)
	{ 0xEA, 0x60, 0xB3, 0xEA, 0xCE, 0x8F, 0x24, 0x46, 0x7D, 0x33, 0x9C, 0xD1, 0xBC, 0x24, 0x98, 0x29 },
	// Master key 8  encrypted with 9.  (8.1.0 with 9.0.0)
	{ 0x4D, 0xD9, 0x98, 0x42, 0x45, 0x0D, 0xB1, 0x3C, 0x52, 0x0C, 0x9A, 0x44, 0xBB, 0xAD, 0xAF, 0x80 },
	// Master key 9  encrypted with 10. (9.0.0 with 9.1.0)
	{ 0xB8, 0x96, 0x9E, 0x4A, 0x00, 0x0D, 0xD6, 0x28, 0xB3, 0xD1, 0xDB, 0x68, 0x5F, 0xFB, 0xE1, 0x2A },
	// Master key 10 encrypted with 11. (9.1.0 with 12.1.0)
	{ 0xC1, 0x8D, 0x16, 0xBB, 0x2A, 0xE4, 0x1D, 0xD4, 0xC2, 0xC1, 0xB6, 0x40, 0x94, 0x35, 0x63, 0x98 },
	// Master key 11 encrypted with 12. (12.1.0 with 13.0.0)
	{ 0xA3, 0x24, 0x65, 0x75, 0xEA, 0xCC, 0x6E, 0x8D, 0xFB, 0x5A, 0x16, 0x50, 0x74, 0xD2, 0x15, 0x06 },
	// Master key 12 encrypted with 13. (13.0.0 with 14.0.0)
	{ 0x83, 0x67, 0xAF, 0x01, 0xCF, 0x93, 0xA1, 0xAB, 0x80, 0x45, 0xF7, 0x3F, 0x72, 0xFD, 0x3B, 0x38 },
	// Master key 13 encrypted with 14. (14.0.0 with 15.0.0)
	{ 0xB1, 0x81, 0xA6, 0x0D, 0x72, 0xC7, 0xEE, 0x15, 0x21, 0xF3, 0xC0, 0xB5, 0x6B, 0x61, 0x6D, 0xE7 },
	// Master key 14 encrypted with 15. (15.0.0 with 16.0.0)
	{ 0xAF, 0x11, 0x4C, 0x67, 0x17, 0x7A, 0x52, 0x43, 0xF7, 0x70, 0x2F, 0xC7, 0xEF, 0x81, 0x72, 0x16 },
	// Master key 15 encrypted with 16. (16.0.0 with 17.0.0)
	{ 0x25, 0x12, 0x8B, 0xCB, 0xB5, 0x46, 0xA1, 0xF8, 0xE0, 0x52, 0x15, 0xB7, 0x0B, 0x57, 0x00, 0xBD },
	// Master key 16 encrypted with 17. (17.0.0 with 18.0.0)
	{ 0x58, 0x15, 0xD2, 0xF6, 0x8A, 0xE8, 0x19, 0xAB, 0xFB, 0x2D, 0x52, 0x9D, 0xE7, 0x55, 0xF3, 0x93 },
	// Master key 17 encrypted with 18. (18.0.0 with 19.0.0)
	{ 0x4A, 0x01, 0x3B, 0xC7, 0x44, 0x6E, 0x45, 0xBD, 0xE6, 0x5E, 0x2B, 0xEC, 0x07, 0x37, 0x52, 0x86 },
	// Master key 18 encrypted with 19. (19.0.0 with 20.0.0)
	{ 0x97, 0xE4, 0x11, 0xAB, 0x22, 0x72, 0x1A, 0x1F, 0x70, 0x5C, 0x00, 0xB3, 0x96, 0x30, 0x05, 0x28 },
	// Master key 19 encrypted with 20. (20.0.0 with 21.0.0)
	{ 0xF7, 0x92, 0xC0, 0xEC, 0xF3, 0xA4, 0x8C, 0xB7, 0x0D, 0xB3, 0xF3, 0xAB, 0x10, 0x9B, 0x18, 0xBA },
};

static bool _pkg2_key_unwrap_validate(pkg2_hdr_t *tmp_test, pkg2_hdr_t *hdr, u8 src_slot, u8 *mkey, const u8 *key_seed)
{
	// Decrypt older encrypted mkey.
	se_aes_crypt_ecb(src_slot, DECRYPT, mkey, SE_KEY_128_SIZE, key_seed, SE_KEY_128_SIZE);
	// Set and unwrap pkg2 key.
	se_aes_key_set(9, mkey, SE_KEY_128_SIZE);
	se_aes_unwrap_key(9, 9, package2_keyseed);

	// Decrypt header.
	se_aes_crypt_ctr(9, tmp_test, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);

	// Return if header is valid.
	return (tmp_test->magic == PKG2_MAGIC);
}

pkg2_hdr_t *pkg2_decrypt(void *data, u8 mkey)
{
	pkg2_hdr_t mkey_test;
	u8 *pdata = (u8 *)data;
	u8 pkg2_keyslot = 8;

	// Skip signature.
	pdata += 0x100;

	pkg2_hdr_t *hdr = (pkg2_hdr_t *)pdata;

	// Skip header.
	pdata += sizeof(pkg2_hdr_t);

	// Check if we need to decrypt with newer mkeys. Valid for THK for 7.0.0 and up.
	se_aes_crypt_ctr(8, &mkey_test, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);

	if (mkey_test.magic == PKG2_MAGIC)
		goto key_found;

	// Decrypt older pkg2 via new mkeys.
	if ((mkey >= HOS_MKEY_VER_700) && (mkey < HOS_MKEY_VER_MAX))
	{
		u8 tmp_mkey[SE_KEY_128_SIZE];
		u8 decr_slot = 7; // THK mkey or T210B01 mkey.
		u8 mkey_seeds_cnt = sizeof(mkey_vector_7xx) / SE_KEY_128_SIZE;
		u8 mkey_seeds_idx = mkey_seeds_cnt; // Real index + 1.
		u8 mkey_seeds_min_idx = mkey_seeds_cnt - (HOS_MKEY_VER_MAX - mkey);

		while (mkey_seeds_cnt)
		{
			// Decrypt and validate mkey.
			int res = _pkg2_key_unwrap_validate(&mkey_test, hdr, decr_slot,
				tmp_mkey, mkey_vector_7xx[mkey_seeds_idx - 1]);

			if (res)
			{
				pkg2_keyslot = 9;
				goto key_found;
			}
			else
			{
				// Set current mkey in order to decrypt a lower mkey.
				mkey_seeds_idx--;
				se_aes_key_set(9, tmp_mkey, SE_KEY_128_SIZE);

				decr_slot = 9; // Temp key.

				// Check if we tried last key for that pkg2 version.
				// And start with a lower mkey in case mkey is older.
				if (mkey_seeds_idx == mkey_seeds_min_idx)
				{
					mkey_seeds_cnt--;
					mkey_seeds_idx = mkey_seeds_cnt;
					decr_slot = 7; // THK mkey or T210B01 mkey.
				}
			}
		}
	}

key_found:
	// Decrypt header.
	se_aes_crypt_ctr(pkg2_keyslot, hdr, sizeof(pkg2_hdr_t), hdr, sizeof(pkg2_hdr_t), hdr);

	if (hdr->magic != PKG2_MAGIC)
		return NULL;

	// Decrypt sections.
	for (u32 i = 0; i < 4; i++)
	{
DPRINTF("sec %d has size %08X\n", i, hdr->sec_size[i]);
		if (!hdr->sec_size[i])
			continue;

		se_aes_crypt_ctr(pkg2_keyslot, pdata, hdr->sec_size[i], pdata, hdr->sec_size[i], &hdr->sec_ctr[i * SE_AES_IV_SIZE]);

		pdata += hdr->sec_size[i];
	}

	return hdr;
}
