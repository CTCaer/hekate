/*
 * eMMC BIS driver for Nintendo Switch
 *
 * Copyright (c) 2019-2020 shchmue
 * Copyright (c) 2019-2021 CTCaer
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

#include <memory_map.h>

#include <mem/heap.h>
#include <sec/se.h>
#include <sec/se_t210.h>
#include "../storage/nx_emmc.h"
#include <storage/nx_sd.h>
#include <storage/sdmmc.h>
#include <utils/types.h>

#define BIS_CLUSTER_SECTORS   32
#define BIS_CLUSTER_SIZE      16384
#define BIS_CACHE_MAX_ENTRIES 16384
#define BIS_CACHE_LOOKUP_TBL_EMPTY_ENTRY -1

typedef struct _cluster_cache_t
{
	u32  cluster_idx;            // Index of the cluster in the partition.
	bool dirty;                  // Has been modified without write-back flag.
	u8   data[BIS_CLUSTER_SIZE]; // The cached cluster itself. Aligned to 8 bytes for DMA engine.
} cluster_cache_t;

typedef struct _bis_cache_t
{
	bool full;
	bool enabled;
	u32  dirty_cnt;
	u32  top_idx;
	u8   dma_buff[BIS_CLUSTER_SIZE]; // Aligned to 8 bytes for DMA engine.
	cluster_cache_t clusters[];
} bis_cache_t;

static u8  ks_crypt = 0;
static u8  ks_tweak = 0;
static u32 emu_offset = 0;
static emmc_part_t *system_part = NULL;
static u32 *cache_lookup_tbl = (u32 *)NX_BIS_LOOKUP_ADDR;
static bis_cache_t *bis_cache = (bis_cache_t *)NX_BIS_CACHE_ADDR;

static void _gf256_mul_x_le(void *block)
{
	u32 *pdata = (u32 *)block;
	u32 carry = 0;

	for (u32 i = 0; i < 4; i++)
	{
		u32 b = pdata[i];
		pdata[i] = (b << 1) | carry;
		carry = b >> 31;
	}

	if (carry)
		pdata[0x0] ^= 0x87;
}

static int _nx_aes_xts_crypt_sec(u32 tweak_ks, u32 crypt_ks, u32 enc, u8 *tweak, bool regen_tweak, u32 tweak_exp, u64 sec, void *dst, void *src, u32 sec_size)
{
	u32 *pdst = (u32 *)dst;
	u32 *psrc = (u32 *)src;
	u32 *ptweak = (u32 *)tweak;

	if (regen_tweak)
	{
		for (int i = 0xF; i >= 0; i--)
		{
			tweak[i] = sec & 0xFF;
			sec >>= 8;
		}
		if (!se_aes_crypt_block_ecb(tweak_ks, 1, tweak, tweak))
			return 0;
	}

	// tweak_exp allows us to use a saved tweak to reduce _gf256_mul_x_le calls.
	for (u32 i = 0; i < (tweak_exp << 5); i++)
		_gf256_mul_x_le(tweak);

	u8 orig_tweak[SE_KEY_128_SIZE] __attribute__((aligned(4)));
	memcpy(orig_tweak, tweak, SE_KEY_128_SIZE);

	// We are assuming a 16 sector aligned size in this implementation.
	for (u32 i = 0; i < (sec_size >> 4); i++)
	{
		for (u32 j = 0; j < 4; j++)
			pdst[j] = psrc[j] ^ ptweak[j];

		_gf256_mul_x_le(tweak);
		psrc += 4;
		pdst += 4;
	}

	if (!se_aes_crypt_ecb(crypt_ks, enc, dst, sec_size, dst, sec_size))
		return 0;

	pdst = (u32 *)dst;
	ptweak = (u32 *)orig_tweak;
	for (u32 i = 0; i < (sec_size >> 4); i++)
	{
		for (u32 j = 0; j < 4; j++)
			pdst[j] = pdst[j] ^ ptweak[j];

		_gf256_mul_x_le(orig_tweak);
		pdst += 4;
	}

	return 1;
}

static int nx_emmc_bis_write_block(u32 sector, u32 count, void *buff, bool flush)
{
	if (!system_part)
		return 3; // Not ready.

	int res;
	u8   tweak[SE_KEY_128_SIZE] __attribute__((aligned(4)));
	u32  cluster = sector / BIS_CLUSTER_SECTORS;
	u32  aligned_sector = cluster * BIS_CLUSTER_SECTORS;
	u32  sector_in_cluster = sector % BIS_CLUSTER_SECTORS;
	u32  lookup_idx = cache_lookup_tbl[cluster];
	bool is_cached = lookup_idx != BIS_CACHE_LOOKUP_TBL_EMPTY_ENTRY;

	// Write to cached cluster.
	if (is_cached)
	{
		if (buff)
			memcpy(bis_cache->clusters[lookup_idx].data + sector_in_cluster * NX_EMMC_BLOCKSIZE, buff, count * NX_EMMC_BLOCKSIZE);
		else
			buff = bis_cache->clusters[lookup_idx].data;
		if (!bis_cache->clusters[lookup_idx].dirty)
			bis_cache->dirty_cnt++;
		bis_cache->clusters[lookup_idx].dirty = true;

		if (!flush)
			return 0; // Success.

		// Reset args to trigger a full cluster flush to emmc.
		sector_in_cluster = 0;
		sector = aligned_sector;
		count = BIS_CLUSTER_SECTORS;
	}

	// Encrypt cluster.
	if (!_nx_aes_xts_crypt_sec(ks_tweak, ks_crypt, 1, tweak, true, sector_in_cluster, cluster, bis_cache->dma_buff, buff, count * NX_EMMC_BLOCKSIZE))
		return 1; // Encryption error.

	// If not reading from cache, do a regular read and decrypt.
	if (!emu_offset)
		res = nx_emmc_part_write(&emmc_storage, system_part, sector, count, bis_cache->dma_buff);
	else
		res = sdmmc_storage_write(&sd_storage, emu_offset + system_part->lba_start + sector, count, bis_cache->dma_buff);
	if (!res)
		return 1; // R/W error.

	// Mark cache entry not dirty if write succeeds.
	if (is_cached)
	{
		bis_cache->clusters[lookup_idx].dirty = false;
		bis_cache->dirty_cnt--;
	}

	return 0; // Success.
}

static void _nx_emmc_bis_cluster_cache_init(bool enable_cache)
{
	u32 cache_lookup_tbl_size = (system_part->lba_end - system_part->lba_start + 1) / BIS_CLUSTER_SECTORS * sizeof(*cache_lookup_tbl);

	// Clear cache header.
	memset(bis_cache, 0, sizeof(bis_cache_t));

	// Clear cluster lookup table.
	memset(cache_lookup_tbl, BIS_CACHE_LOOKUP_TBL_EMPTY_ENTRY, cache_lookup_tbl_size);

	// Enable cache.
	bis_cache->enabled = enable_cache;
}

static void _nx_emmc_bis_flush_cache()
{
	if (!bis_cache->enabled || !bis_cache->dirty_cnt)
		return;

	for (u32 i = 0; i < bis_cache->top_idx && bis_cache->dirty_cnt; i++)
	{
		if (bis_cache->clusters[i].dirty) {
			nx_emmc_bis_write_block(bis_cache->clusters[i].cluster_idx * BIS_CLUSTER_SECTORS, BIS_CLUSTER_SECTORS, NULL, true);
			bis_cache->dirty_cnt--;
		}
	}

	_nx_emmc_bis_cluster_cache_init(true);
}

static int nx_emmc_bis_read_block_normal(u32 sector, u32 count, void *buff)
{
	static u32 prev_cluster = -1;
	static u32 prev_sector = 0;
	static u8  tweak[SE_KEY_128_SIZE] __attribute__((aligned(4)));

	int  res;
	bool regen_tweak = true;
	u32  tweak_exp = 0;
	u32  cluster = sector / BIS_CLUSTER_SECTORS;
	u32  sector_in_cluster = sector % BIS_CLUSTER_SECTORS;

	// If not reading from cache, do a regular read and decrypt.
	if (!emu_offset)
		res = nx_emmc_part_read(&emmc_storage, system_part, sector, count, bis_cache->dma_buff);
	else
		res = sdmmc_storage_read(&sd_storage, emu_offset + system_part->lba_start + sector, count, bis_cache->dma_buff);
	if (!res)
		return 1; // R/W error.

	if (prev_cluster != cluster) // Sector in different cluster than last read.
	{
		prev_cluster = cluster;
		tweak_exp = sector_in_cluster;
	}
	else if (sector > prev_sector) // Sector in same cluster and past last sector.
	{
		// Calculates the new tweak using the saved one, reducing expensive _gf256_mul_x_le calls.
		tweak_exp = sector - prev_sector - 1;
		regen_tweak = false;
	}
	else // Sector in same cluster and before or same as last sector.
		tweak_exp = sector_in_cluster;

	// Maximum one cluster (1 XTS crypto block 16KB).
	if (!_nx_aes_xts_crypt_sec(ks_tweak, ks_crypt, 0, tweak, regen_tweak, tweak_exp, prev_cluster, buff, bis_cache->dma_buff, count * NX_EMMC_BLOCKSIZE))
		return 1; // R/W error.

	prev_sector = sector + count - 1;

	return 0; // Success.
}

static int nx_emmc_bis_read_block_cached(u32 sector, u32 count, void *buff)
{
	int res;
	u8  cache_tweak[SE_KEY_128_SIZE] __attribute__((aligned(4)));
	u32 cluster = sector / BIS_CLUSTER_SECTORS;
	u32 cluster_sector = cluster * BIS_CLUSTER_SECTORS;
	u32 sector_in_cluster = sector % BIS_CLUSTER_SECTORS;
	u32 lookup_idx = cache_lookup_tbl[cluster];

	// Read from cached cluster.
	if (lookup_idx != BIS_CACHE_LOOKUP_TBL_EMPTY_ENTRY)
	{
		memcpy(buff, bis_cache->clusters[lookup_idx].data + sector_in_cluster * NX_EMMC_BLOCKSIZE, count * NX_EMMC_BLOCKSIZE);

		return 0; // Success.
	}

	// Flush cache if full.
	if (bis_cache->top_idx >= BIS_CACHE_MAX_ENTRIES)
		_nx_emmc_bis_flush_cache();

	// Set new cached cluster parameters.
	bis_cache->clusters[bis_cache->top_idx].cluster_idx = cluster;
	bis_cache->clusters[bis_cache->top_idx].dirty = false;
	cache_lookup_tbl[cluster] = bis_cache->top_idx;

	// Read the whole cluster the sector resides in.
	if (!emu_offset)
		res = nx_emmc_part_read(&emmc_storage, system_part, cluster_sector, BIS_CLUSTER_SECTORS, bis_cache->dma_buff);
	else
		res = sdmmc_storage_read(&sd_storage, emu_offset + system_part->lba_start + cluster_sector, BIS_CLUSTER_SECTORS, bis_cache->dma_buff);
	if (!res)
		return 1; // R/W error.

	// Decrypt cluster.
	if (!_nx_aes_xts_crypt_sec(ks_tweak, ks_crypt, 0, cache_tweak, true, 0, cluster, bis_cache->dma_buff, bis_cache->dma_buff, BIS_CLUSTER_SIZE))
		return 1; // Decryption error.

	// Copy to cluster cache.
	memcpy(bis_cache->clusters[bis_cache->top_idx].data, bis_cache->dma_buff, BIS_CLUSTER_SIZE);
	memcpy(buff, bis_cache->dma_buff + sector_in_cluster * NX_EMMC_BLOCKSIZE, count * NX_EMMC_BLOCKSIZE);

	// Increment cache count.
	bis_cache->top_idx++;

	return 0; // Success.
}

static int nx_emmc_bis_read_block(u32 sector, u32 count, void *buff)
{
	if (!system_part)
		return 3; // Not ready.

	if (bis_cache->enabled)
		return nx_emmc_bis_read_block_cached(sector, count, buff);
	else
		return nx_emmc_bis_read_block_normal(sector, count, buff);
}

int nx_emmc_bis_read(u32 sector, u32 count, void *buff)
{
	u8 *buf = (u8 *)buff;
	u32 curr_sct = sector;

	while (count)
	{
		u32 sct_cnt = MIN(count, BIS_CLUSTER_SECTORS);
		if (nx_emmc_bis_read_block(curr_sct, sct_cnt, buf))
			return 0;

		count    -= sct_cnt;
		curr_sct += sct_cnt;
		buf      += sct_cnt * NX_EMMC_BLOCKSIZE;
	}

	return 1;
}

int nx_emmc_bis_write(u32 sector, u32 count, void *buff)
{
	u8 *buf = (u8 *)buff;
	u32 curr_sct = sector;

	while (count)
	{
		u32 sct_cnt = MIN(count, BIS_CLUSTER_SECTORS);
		if (nx_emmc_bis_write_block(curr_sct, sct_cnt, buf, false))
			return 0;

		count    -= sct_cnt;
		curr_sct += sct_cnt;
		buf      += sct_cnt * NX_EMMC_BLOCKSIZE;
	}

	return 1;
}

void nx_emmc_bis_init(emmc_part_t *part, bool enable_cache, u32 emummc_offset)
{
	system_part = part;
	emu_offset = emummc_offset;

	_nx_emmc_bis_cluster_cache_init(enable_cache);

	if (!strcmp(part->name, "PRODINFO") || !strcmp(part->name, "PRODINFOF"))
	{
		ks_crypt = 0;
		ks_tweak = 1;
	}
	else if (!strcmp(part->name, "SAFE"))
	{
		ks_crypt = 2;
		ks_tweak = 3;
	}
	else if (!strcmp(part->name, "SYSTEM") || !strcmp(part->name, "USER"))
	{
		ks_crypt = 4;
		ks_tweak = 5;
	}
	else
		system_part = NULL;
}

void nx_emmc_bis_end()
{
	_nx_emmc_bis_flush_cache();
	system_part = NULL;
}
