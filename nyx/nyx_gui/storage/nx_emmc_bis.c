/*
 * eMMC BIS driver for Nintendo Switch
 *
 * Copyright (c) 2019 shchmue
 * Copyright (c) 2019-2020 CTCaer
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

#include "../../../common/memory_map.h"

#include "../sec/se.h"
#include "../storage/nx_emmc.h"
#include "../storage/sdmmc.h"
#include "../utils/types.h"

#define MAX_SEC_CACHE_ENTRIES 1500

typedef struct _sector_cache_t
{
	u32 sector;
	u32 visit_cnt;
	u8  tweak[0x10];
	u8  data[0x200];
	u8  align[8];
} sector_cache_t;

static u8 ks_crypt = 0;
static u8 ks_tweak = 0;
static u32 sector_cache_cnt = 0;
static emmc_part_t *system_part = NULL;
static sector_cache_t *sector_cache = (sector_cache_t *)NX_BIS_CACHE_ADDR;

static void _gf256_mul_x_le(u8 *block)
{
	u8 *pdata = (u8 *)block;
	u32 carry = 0;

	for (u32 i = 0; i < 0x10; i++)
	{
		u8 b = pdata[i];
		pdata[i] = (b << 1) | carry;
		carry = b >> 7;
	}

	if (carry)
		pdata[0x0] ^= 0x87;
}

static int _nx_aes_xts_crypt_sec(u32 ks1, u32 ks2, u32 enc, u8 *tweak, bool regen_tweak, u32 tweak_exp, u64 sec, void *dst, void *src, u32 sec_size)
{
	u8 *pdst = (u8 *)dst;
	u8 *psrc = (u8 *)src;

	if (regen_tweak)
	{
		for (int i = 0xF; i >= 0; i--)
		{
			tweak[i] = sec & 0xFF;
			sec >>= 8;
		}
		if (!se_aes_crypt_block_ecb(ks1, 1, tweak, tweak))
			return 0;
	}

	for (u32 i = 0; i < (tweak_exp << 5); i++)
		_gf256_mul_x_le(tweak);

	u8 tmp_tweak[0x10];
	memcpy(tmp_tweak, tweak, 0x10);

	// We are assuming a 0x10-aligned sector size in this implementation.
	for (u32 i = 0; i < (sec_size >> 4); i++)
	{
		for (u32 j = 0; j < 0x10; j++)
			pdst[j] = psrc[j] ^ tweak[j];

		_gf256_mul_x_le(tweak);
		psrc += 0x10;
		pdst += 0x10;
	}

	se_aes_crypt_ecb(ks2, enc, dst, sec_size, src, sec_size);

	memcpy(tweak, tmp_tweak, 0x10);

	pdst = (u8 *)dst;
	for (u32 i = 0; i < (sec_size >> 4); i++)
	{
		for (u32 j = 0; j < 0x10; j++)
			pdst[j] = pdst[j] ^ tweak[j];

		_gf256_mul_x_le(tweak);
		pdst += 0x10;
	}

	return 1;
}

static int nx_emmc_bis_read_block(u32 sector, u32 count, void *buff)
{
	if (!system_part)
		return 3; // Not ready.

	static u32 prev_cluster = -1;
	static u32 prev_sector = 0;
	static u8 tweak[0x10];

	u32 cache_idx = 0;
	u32 tweak_exp = 0;
	bool regen_tweak = true;
	bool cache_sector = false;

	if (count == 1)
	{
		for ( ; cache_idx < sector_cache_cnt; cache_idx++)
		{
			if (sector_cache[cache_idx].sector == sector)
			{
				sector_cache[cache_idx].visit_cnt++;
				memcpy(buff, sector_cache[cache_idx].data, 0x200);
				memcpy(tweak, sector_cache[cache_idx].tweak, 0x10);
				prev_sector = sector;
				prev_cluster = sector >> 5;

				return 0;
			}
		}
		// add to cache
		if (cache_idx == sector_cache_cnt && cache_idx < MAX_SEC_CACHE_ENTRIES)
		{
			sector_cache[cache_idx].sector = sector;
			sector_cache[cache_idx].visit_cnt++;
			cache_sector = true;
			sector_cache_cnt++;
		}
	}

	if (nx_emmc_part_read(&emmc_storage, system_part, sector, count, buff))
	{
		if (prev_cluster != sector >> 5) // Sector in different cluster than last read.
		{
			prev_cluster = sector >> 5;
			tweak_exp = sector % 0x20;
		}
		else if (sector > prev_sector) // Sector in same cluster and past last sector.
		{
			tweak_exp = sector - prev_sector - 1;
			regen_tweak = false;
		}
		else // Sector in same cluster and before or same as last sector.
			tweak_exp = sector % 0x20;

		// Maximum one cluster (1 XTS crypto block 16KB).
		_nx_aes_xts_crypt_sec(ks_tweak, ks_crypt, 0, tweak, regen_tweak, tweak_exp, prev_cluster, buff, buff, count << 9);
		if (cache_sector)
		{
			memcpy(sector_cache[cache_idx].data, buff, 0x200);
			memcpy(sector_cache[cache_idx].tweak, tweak, 0x10);
		}
		prev_sector = sector + count - 1;

		return 0;
	}

	// Error occurred.
	return 1;
}

int nx_emmc_bis_read(u32 sector, u32 count, void *buff)
{
	int res = 1;
	u8 *buf = (u8 *)buff;
	u32 curr_sct = sector;

	while (count)
	{
		u32 sct_cnt = MIN(count, 0x20);
		res = nx_emmc_bis_read_block(curr_sct, sct_cnt, buf);
		if (res)
			return 1;

		count -= sct_cnt;
		curr_sct += sct_cnt;
		buf += 512 * sct_cnt;
	}

	return res;
}

void nx_emmc_bis_init(emmc_part_t *part)
{
	system_part = part;
	sector_cache_cnt = 0;

	switch (part->index)
	{
	case 0:  // PRODINFO.
	case 1:  // PRODINFOF.
		ks_crypt = 0;
		ks_tweak = 1;
		break;
	case 8:  // SAFE.
		ks_crypt = 2;
		ks_tweak = 3;
		break;
	case 9:  // SYSTEM.
	case 10: // USER.
		ks_crypt = 4;
		ks_tweak = 5;
		break;
	}
}
