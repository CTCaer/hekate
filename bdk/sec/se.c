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

#include "se.h"
#include <memory_map.h>
#include <soc/bpmp.h>
#include <soc/hw_init.h>
#include <soc/pmc.h>
#include <soc/timer.h>
#include <soc/t210.h>

typedef struct _se_ll_t
{
	vu32 num;
	vu32 addr;
	vu32 size;
} se_ll_t;

se_ll_t ll_src, ll_dst;
se_ll_t *ll_src_ptr, *ll_dst_ptr; // Must be u32 aligned.

static void _gf256_mul_x(void *block)
{
	u8 *pdata = (u8 *)block;
	u32 carry = 0;

	for (int i = 0xF; i >= 0; i--)
	{
		u8 b = pdata[i];
		pdata[i] = (b << 1) | carry;
		carry = b >> 7;
	}

	if (carry)
		pdata[0xF] ^= 0x87;
}

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

static void _se_ll_init(se_ll_t *ll, u32 addr, u32 size)
{
	ll->num  = 0;
	ll->addr = addr;
	ll->size = size;
}

static void _se_ll_set(se_ll_t *src, se_ll_t *dst)
{
	SE(SE_IN_LL_ADDR_REG)  = (u32)src;
	SE(SE_OUT_LL_ADDR_REG) = (u32)dst;
}

static int _se_wait()
{
	bool tegra_t210 = hw_get_chip_id() == GP_HIDREV_MAJOR_T210;

	// Wait for operation to be done.
	while (!(SE(SE_INT_STATUS_REG) & SE_INT_OP_DONE))
		;

	// Check for errors.
	if ((SE(SE_INT_STATUS_REG) & SE_INT_ERR_STAT)                          ||
		(SE(SE_STATUS_REG) & SE_STATUS_STATE_MASK) != SE_STATUS_STATE_IDLE ||
		(SE(SE_ERR_STATUS_REG) != 0)
	   )
	{
		return 0;
	}

	// T210B01: IRAM/TZRAM/DRAM AHB coherency WAR.
	if (!tegra_t210 && ll_dst_ptr)
	{
		u32 timeout = get_tmr_us() + 1000000;
		// Ensure data is out from SE.
		while (SE(SE_STATUS_REG) & SE_STATUS_MEM_IF_BUSY)
		{
			if (get_tmr_us() > timeout)
				return 0;
			usleep(1);
		}

		// Ensure data is out from AHB.
		if (ll_dst_ptr->addr >= DRAM_START)
		{
			timeout = get_tmr_us() + 200000;
			while (AHB_GIZMO(AHB_ARBITRATION_AHB_MEM_WRQUE_MST_ID) & MEM_WRQUE_SE_MST_ID)
			{
				if (get_tmr_us() > timeout)
					return 0;
				usleep(1);
			}
		}
	}

	return 1;
}

static int _se_execute_finalize()
{
	int res = _se_wait();

	// Invalidate data after OP is done.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_INVALID_WAY, false);

	ll_src_ptr = NULL;
	ll_dst_ptr = NULL;

	return res;
}

static int _se_execute(u32 op, void *dst, u32 dst_size, const void *src, u32 src_size, bool is_oneshot)
{
	ll_src_ptr = NULL;
	ll_dst_ptr = NULL;

	if (src)
	{
		ll_src_ptr = &ll_src;
		_se_ll_init(ll_src_ptr, (u32)src, src_size);
	}

	if (dst)
	{
		ll_dst_ptr = &ll_dst;
		_se_ll_init(ll_dst_ptr, (u32)dst, dst_size);
	}

	_se_ll_set(ll_src_ptr, ll_dst_ptr);

	SE(SE_ERR_STATUS_REG) = SE(SE_ERR_STATUS_REG);
	SE(SE_INT_STATUS_REG) = SE(SE_INT_STATUS_REG);

	// Flush data before starting OP.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLEAN_WAY, false);

	SE(SE_OPERATION_REG) = op;

	if (is_oneshot)
		return _se_execute_finalize();

	return 1;
}

static int _se_execute_oneshot(u32 op, void *dst, u32 dst_size, const void *src, u32 src_size)
{
	return _se_execute(op, dst, dst_size, src, src_size, true);
}

static int _se_execute_one_block(u32 op, void *dst, u32 dst_size, const void *src, u32 src_size)
{
	if (!src || !dst)
		return 0;

	u32 block[SE_AES_BLOCK_SIZE / sizeof(u32)] = {0};

	SE(SE_CRYPTO_BLOCK_COUNT_REG) = 1 - 1;

	memcpy(block, src, src_size);
	int res = _se_execute_oneshot(op, block, SE_AES_BLOCK_SIZE, block, SE_AES_BLOCK_SIZE);
	memcpy(dst, block, dst_size);

	return res;
}

static void _se_aes_ctr_set(const void *ctr)
{
	u32 data[SE_AES_IV_SIZE / 4];
	memcpy(data, ctr, SE_AES_IV_SIZE);

	for (u32 i = 0; i < SE_CRYPTO_LINEAR_CTR_REG_COUNT; i++)
		SE(SE_CRYPTO_LINEAR_CTR_REG + (4 * i)) = data[i];
}

void se_rsa_acc_ctrl(u32 rs, u32 flags)
{
	if (flags & SE_RSA_KEY_TBL_DIS_KEY_ACCESS_FLAG)
		SE(SE_RSA_KEYTABLE_ACCESS_REG + 4 * rs) =
			(((flags >> 4) & SE_RSA_KEY_TBL_DIS_KEYUSE_FLAG) | (flags & SE_RSA_KEY_TBL_DIS_KEY_READ_UPDATE_FLAG)) ^
				SE_RSA_KEY_TBL_DIS_KEY_READ_UPDATE_USE_FLAG;
	if (flags & SE_RSA_KEY_LOCK_FLAG)
		SE(SE_RSA_SECURITY_PERKEY_REG) &= ~BIT(rs);
}

void se_key_acc_ctrl(u32 ks, u32 flags)
{
	if (flags & SE_KEY_TBL_DIS_KEY_ACCESS_FLAG)
		SE(SE_CRYPTO_KEYTABLE_ACCESS_REG + 4 * ks) = ~flags;
	if (flags & SE_KEY_LOCK_FLAG)
		SE(SE_CRYPTO_SECURITY_PERKEY_REG) &= ~BIT(ks);
}

u32 se_key_acc_ctrl_get(u32 ks)
{
	return SE(SE_CRYPTO_KEYTABLE_ACCESS_REG + 4 * ks);
}

void se_aes_key_set(u32 ks, const void *key, u32 size)
{
	u32 data[SE_AES_MAX_KEY_SIZE / 4];
	memcpy(data, key, size);

	for (u32 i = 0; i < (size / 4); i++)
	{
		SE(SE_CRYPTO_KEYTABLE_ADDR_REG) = SE_KEYTABLE_SLOT(ks) | SE_KEYTABLE_PKT(i); // QUAD is automatically set by PKT.
		SE(SE_CRYPTO_KEYTABLE_DATA_REG) = data[i];
	}
}

void se_aes_iv_set(u32 ks, const void *iv)
{
	u32 data[SE_AES_IV_SIZE / 4];
	memcpy(data, iv, SE_AES_IV_SIZE);

	for (u32 i = 0; i < (SE_AES_IV_SIZE / 4); i++)
	{
		SE(SE_CRYPTO_KEYTABLE_ADDR_REG) = SE_KEYTABLE_SLOT(ks) | SE_KEYTABLE_QUAD(ORIGINAL_IV) | SE_KEYTABLE_PKT(i);
		SE(SE_CRYPTO_KEYTABLE_DATA_REG) = data[i];
	}
}

void se_aes_key_get(u32 ks, void *key, u32 size)
{
	u32 data[SE_AES_MAX_KEY_SIZE / 4];

	for (u32 i = 0; i < (size / 4); i++)
	{
		SE(SE_CRYPTO_KEYTABLE_ADDR_REG) = SE_KEYTABLE_SLOT(ks) | SE_KEYTABLE_PKT(i); // QUAD is automatically set by PKT.
		data[i] = SE(SE_CRYPTO_KEYTABLE_DATA_REG);
	}

	memcpy(key, data, size);
}

void se_aes_key_clear(u32 ks)
{
	for (u32 i = 0; i < (SE_AES_MAX_KEY_SIZE / 4); i++)
	{
		SE(SE_CRYPTO_KEYTABLE_ADDR_REG) = SE_KEYTABLE_SLOT(ks) | SE_KEYTABLE_PKT(i); // QUAD is automatically set by PKT.
		SE(SE_CRYPTO_KEYTABLE_DATA_REG) = 0;
	}
}

void se_aes_iv_clear(u32 ks)
{
	for (u32 i = 0; i < (SE_AES_IV_SIZE / 4); i++)
	{
		SE(SE_CRYPTO_KEYTABLE_ADDR_REG) = SE_KEYTABLE_SLOT(ks) | SE_KEYTABLE_QUAD(ORIGINAL_IV) | SE_KEYTABLE_PKT(i);
		SE(SE_CRYPTO_KEYTABLE_DATA_REG) = 0;
	}
}

void se_aes_iv_updated_clear(u32 ks)
{
	for (u32 i = 0; i < (SE_AES_IV_SIZE / 4); i++)
	{
		SE(SE_CRYPTO_KEYTABLE_ADDR_REG) = SE_KEYTABLE_SLOT(ks) | SE_KEYTABLE_QUAD(UPDATED_IV) | SE_KEYTABLE_PKT(i);
		SE(SE_CRYPTO_KEYTABLE_DATA_REG) = 0;
	}
}

int se_aes_unwrap_key(u32 ks_dst, u32 ks_src, const void *input)
{
	SE(SE_CONFIG_REG)              = SE_CONFIG_DEC_ALG(ALG_AES_DEC) | SE_CONFIG_DST(DST_KEYTABLE);
	SE(SE_CRYPTO_CONFIG_REG)       = SE_CRYPTO_KEY_INDEX(ks_src) | SE_CRYPTO_CORE_SEL(CORE_DECRYPT);
	SE(SE_CRYPTO_BLOCK_COUNT_REG)  = 1 - 1;
	SE(SE_CRYPTO_KEYTABLE_DST_REG) = SE_KEYTABLE_DST_KEY_INDEX(ks_dst) | SE_KEYTABLE_DST_WORD_QUAD(KEYS_0_3);

	return _se_execute_oneshot(SE_OP_START, NULL, 0, input, SE_KEY_128_SIZE);
}

int se_aes_crypt_hash(u32 ks, u32 enc, void *dst, u32 dst_size, const void *src, u32 src_size)
{
	if (enc)
	{
		SE(SE_CONFIG_REG)        = SE_CONFIG_ENC_ALG(ALG_AES_ENC) | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks)          | SE_CRYPTO_VCTRAM_SEL(VCTRAM_AESOUT) |
								   SE_CRYPTO_CORE_SEL(CORE_ENCRYPT) | SE_CRYPTO_XOR_POS(XOR_TOP) |
								   SE_CRYPTO_HASH(HASH_ENABLE);
	}
	else
	{
		SE(SE_CONFIG_REG)        = SE_CONFIG_DEC_ALG(ALG_AES_DEC) | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks)          | SE_CRYPTO_VCTRAM_SEL(VCTRAM_PREVMEM) |
								   SE_CRYPTO_CORE_SEL(CORE_DECRYPT) | SE_CRYPTO_XOR_POS(XOR_BOTTOM) |
								   SE_CRYPTO_HASH(HASH_ENABLE);
	}
	SE(SE_CRYPTO_BLOCK_COUNT_REG) = (src_size >> 4) - 1;
	return _se_execute_oneshot(SE_OP_START, dst, dst_size, src, src_size);
}

int se_aes_crypt_ecb(u32 ks, u32 enc, void *dst, u32 dst_size, const void *src, u32 src_size)
{
	if (enc)
	{
		SE(SE_CONFIG_REG)        = SE_CONFIG_ENC_ALG(ALG_AES_ENC) | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks) | SE_CRYPTO_CORE_SEL(CORE_ENCRYPT);
	}
	else
	{
		SE(SE_CONFIG_REG)        = SE_CONFIG_DEC_ALG(ALG_AES_DEC) | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks) | SE_CRYPTO_CORE_SEL(CORE_DECRYPT);
	}
	SE(SE_CRYPTO_BLOCK_COUNT_REG) = (src_size >> 4) - 1;
	return _se_execute_oneshot(SE_OP_START, dst, dst_size, src, src_size);
}

int se_aes_crypt_cbc(u32 ks, u32 enc, void *dst, u32 dst_size, const void *src, u32 src_size)
{
	if (enc)
	{
		SE(SE_CONFIG_REG)        = SE_CONFIG_ENC_ALG(ALG_AES_ENC) | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks)          | SE_CRYPTO_VCTRAM_SEL(VCTRAM_AESOUT) |
								   SE_CRYPTO_CORE_SEL(CORE_ENCRYPT) | SE_CRYPTO_XOR_POS(XOR_TOP);
	}
	else
	{
		SE(SE_CONFIG_REG)        = SE_CONFIG_DEC_ALG(ALG_AES_DEC) | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks)          | SE_CRYPTO_VCTRAM_SEL(VCTRAM_PREVMEM) |
								   SE_CRYPTO_CORE_SEL(CORE_DECRYPT) | SE_CRYPTO_XOR_POS(XOR_BOTTOM);
	}
	SE(SE_CRYPTO_BLOCK_COUNT_REG) = (src_size >> 4) - 1;
	return _se_execute_oneshot(SE_OP_START, dst, dst_size, src, src_size);
}

int se_aes_crypt_block_ecb(u32 ks, u32 enc, void *dst, const void *src)
{
	return se_aes_crypt_ecb(ks, enc, dst, SE_AES_BLOCK_SIZE, src, SE_AES_BLOCK_SIZE);
}

int se_aes_crypt_ctr(u32 ks, void *dst, u32 dst_size, const void *src, u32 src_size, void *ctr)
{
	SE(SE_SPARE_REG)         = SE_ECO(SE_ERRATA_FIX_ENABLE);
	SE(SE_CONFIG_REG)        = SE_CONFIG_ENC_ALG(ALG_AES_ENC) | SE_CONFIG_DST(DST_MEMORY);
	SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks)       | SE_CRYPTO_CORE_SEL(CORE_ENCRYPT)   |
							   SE_CRYPTO_XOR_POS(XOR_BOTTOM) | SE_CRYPTO_INPUT_SEL(INPUT_LNR_CTR) |
							   SE_CRYPTO_CTR_CNTN(1);
	_se_aes_ctr_set(ctr);

	u32 src_size_aligned = src_size & 0xFFFFFFF0;
	u32 src_size_delta = src_size & 0xF;

	if (src_size_aligned)
	{
		SE(SE_CRYPTO_BLOCK_COUNT_REG) = (src_size >> 4) - 1;
		if (!_se_execute_oneshot(SE_OP_START, dst, dst_size, src, src_size_aligned))
			return 0;
	}

	if (src_size - src_size_aligned && src_size_aligned < dst_size)
		return _se_execute_one_block(SE_OP_START, dst + src_size_aligned,
			MIN(src_size_delta, dst_size - src_size_aligned),
			src + src_size_aligned, src_size_delta);

	return 1;
}

int se_aes_xts_crypt_sec(u32 tweak_ks, u32 crypt_ks, u32 enc, u64 sec, void *dst, void *src, u32 secsize)
{
	int res = 0;
	u32 tmp[SE_AES_BLOCK_SIZE / sizeof(u32)];
	u8 *tweak = (u8 *)tmp;
	u8 *pdst = (u8 *)dst;
	u8 *psrc = (u8 *)src;

	// Generate tweak.
	for (int i = 0xF; i >= 0; i--)
	{
		tweak[i] = sec & 0xFF;
		sec >>= 8;
	}
	if (!se_aes_crypt_block_ecb(tweak_ks, ENCRYPT, tweak, tweak))
		goto out;

	// We are assuming a 0x10-aligned sector size in this implementation.
	for (u32 i = 0; i < secsize / SE_AES_BLOCK_SIZE; i++)
	{
		for (u32 j = 0; j < SE_AES_BLOCK_SIZE; j++)
			pdst[j] = psrc[j] ^ tweak[j];
		if (!se_aes_crypt_block_ecb(crypt_ks, enc, pdst, pdst))
			goto out;
		for (u32 j = 0; j < SE_AES_BLOCK_SIZE; j++)
			pdst[j] = pdst[j] ^ tweak[j];
		_gf256_mul_x(tweak);
		psrc += SE_AES_BLOCK_SIZE;
		pdst += SE_AES_BLOCK_SIZE;
	}

	res = 1;

out:
	return res;
}

int se_aes_xts_crypt_sec_nx(u32 tweak_ks, u32 crypt_ks, u32 enc, u64 sec, u8 *tweak, bool regen_tweak, u32 tweak_exp, void *dst, void *src, u32 sec_size)
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
		if (!se_aes_crypt_block_ecb(tweak_ks, ENCRYPT, tweak, tweak))
			return 0;
	}

	// tweak_exp allows using a saved tweak to reduce _gf256_mul_x_le calls.
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

int se_aes_xts_crypt(u32 tweak_ks, u32 crypt_ks, u32 enc, u64 sec, void *dst, void *src, u32 secsize, u32 num_secs)
{
	u8 *pdst = (u8 *)dst;
	u8 *psrc = (u8 *)src;

	for (u32 i = 0; i < num_secs; i++)
		if (!se_aes_xts_crypt_sec(tweak_ks, crypt_ks, enc, sec + i, pdst + secsize * i, psrc + secsize * i, secsize))
			return 0;

	return 1;
}

static void se_calc_sha256_get_hash(void *hash, u32 *msg_left)
{
	u32 hash32[SE_SHA_256_SIZE / 4];

	// Backup message left.
	if (msg_left)
	{
		msg_left[0] = SE(SE_SHA_MSG_LEFT_0_REG);
		msg_left[1] = SE(SE_SHA_MSG_LEFT_1_REG);
	}

	// Copy output hash.
	for (u32 i = 0; i < (SE_SHA_256_SIZE / 4); i++)
		hash32[i] = byte_swap_32(SE(SE_HASH_RESULT_REG + (i * 4)));
	memcpy(hash, hash32, SE_SHA_256_SIZE);
}

int se_calc_sha256(void *hash, u32 *msg_left, const void *src, u32 src_size, u64 total_size, u32 sha_cfg, bool is_oneshot)
{
	int res;
	u32 hash32[SE_SHA_256_SIZE / 4];

	//! TODO: src_size must be 512 bit aligned if continuing and not last block for SHA256.
	if (src_size > 0xFFFFFF || !hash) // Max 16MB - 1 chunks and aligned x4 hash buffer.
		return 0;

	// Src size of 0 is not supported, so return null string sha256.
	if (!src_size)
	{
		const u8 null_hash[SE_SHA_256_SIZE] = {
			0xE3, 0xB0, 0xC4, 0x42, 0x98, 0xFC, 0x1C, 0x14, 0x9A, 0xFB, 0xF4, 0xC8, 0x99, 0x6F, 0xB9, 0x24,
			0x27, 0xAE, 0x41, 0xE4, 0x64, 0x9B, 0x93, 0x4C, 0xA4, 0x95, 0x99, 0x1B, 0x78, 0x52, 0xB8, 0x55
		};
		memcpy(hash, null_hash, SE_SHA_256_SIZE);
		return 1;
	}

	// Setup config for SHA256.
	SE(SE_CONFIG_REG) = SE_CONFIG_ENC_MODE(MODE_SHA256) | SE_CONFIG_ENC_ALG(ALG_SHA) | SE_CONFIG_DST(DST_HASHREG);
	SE(SE_SHA_CONFIG_REG) = sha_cfg;
	SE(SE_CRYPTO_BLOCK_COUNT_REG) = 1 - 1;

	// Set total size to current buffer size if empty.
	if (!total_size)
		total_size = src_size;

	// Set total size: BITS(src_size), up to 2 EB.
	SE(SE_SHA_MSG_LENGTH_0_REG) = (u32)(total_size << 3);
	SE(SE_SHA_MSG_LENGTH_1_REG) = (u32)(total_size >> 29);
	SE(SE_SHA_MSG_LENGTH_2_REG) = 0;
	SE(SE_SHA_MSG_LENGTH_3_REG) = 0;

	// Set size left to hash.
	SE(SE_SHA_MSG_LEFT_0_REG) = (u32)(total_size << 3);
	SE(SE_SHA_MSG_LEFT_1_REG) = (u32)(total_size >> 29);
	SE(SE_SHA_MSG_LEFT_2_REG) = 0;
	SE(SE_SHA_MSG_LEFT_3_REG) = 0;

	// If we hash in chunks, copy over the intermediate.
	if (sha_cfg == SHA_CONTINUE && msg_left)
	{
		// Restore message left to process.
		SE(SE_SHA_MSG_LEFT_0_REG) = msg_left[0];
		SE(SE_SHA_MSG_LEFT_1_REG) = msg_left[1];

		// Restore hash reg.
		memcpy(hash32, hash, SE_SHA_256_SIZE);
		for (u32 i = 0; i < (SE_SHA_256_SIZE / 4); i++)
			SE(SE_HASH_RESULT_REG + (i * 4)) = byte_swap_32(hash32[i]);
	}

	// Trigger the operation.
	res = _se_execute(SE_OP_START, NULL, 0, src, src_size, is_oneshot);

	if (is_oneshot)
		se_calc_sha256_get_hash(hash, msg_left);

	return res;
}

int se_calc_sha256_oneshot(void *hash, const void *src, u32 src_size)
{
	return se_calc_sha256(hash, NULL, src, src_size, 0, SHA_INIT_HASH, true);
}

int se_calc_sha256_finalize(void *hash, u32 *msg_left)
{
	int res = _se_execute_finalize();

	se_calc_sha256_get_hash(hash, msg_left);

	return res;
}

int se_gen_prng128(void *dst)
{
	// Setup config for X931 PRNG.
	SE(SE_CONFIG_REG)        = SE_CONFIG_ENC_MODE(MODE_KEY128)  | SE_CONFIG_ENC_ALG(ALG_RNG)    | SE_CONFIG_DST(DST_MEMORY);
	SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_HASH(HASH_DISABLE)     | SE_CRYPTO_XOR_POS(XOR_BYPASS) | SE_CRYPTO_INPUT_SEL(INPUT_RANDOM);
	SE(SE_RNG_CONFIG_REG)    = SE_RNG_CONFIG_SRC(SRC_ENTROPY)   | SE_RNG_CONFIG_MODE(MODE_NORMAL);
	//SE(SE_RNG_SRC_CONFIG_REG) =
	//		SE_RNG_SRC_CONFIG_ENTR_SRC(RO_ENTR_ENABLE) | SE_RNG_SRC_CONFIG_ENTR_SRC_LOCK(RO_ENTR_LOCK_ENABLE);
	SE(SE_RNG_RESEED_INTERVAL_REG) = 1;

	SE(SE_CRYPTO_BLOCK_COUNT_REG) = (16 >> 4) - 1;

	// Trigger the operation.
	return _se_execute_oneshot(SE_OP_START, dst, 16, NULL, 0);
}

void se_get_aes_keys(u8 *buf, u8 *keys, u32 keysize)
{
	u8 *aligned_buf = (u8 *)ALIGN((u32)buf, 0x40);

	// Set Secure Random Key.
	SE(SE_CONFIG_REG)        = SE_CONFIG_ENC_MODE(MODE_KEY128) | SE_CONFIG_ENC_ALG(ALG_RNG)       | SE_CONFIG_DST(DST_SRK);
	SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(0)          | SE_CRYPTO_CORE_SEL(CORE_ENCRYPT) | SE_CRYPTO_INPUT_SEL(INPUT_RANDOM);
	SE(SE_RNG_CONFIG_REG)    = SE_RNG_CONFIG_SRC(SRC_ENTROPY)  | SE_RNG_CONFIG_MODE(MODE_FORCE_RESEED);
	SE(SE_CRYPTO_LAST_BLOCK) = 0;
	_se_execute_oneshot(SE_OP_START, NULL, 0, NULL, 0);

	// Save AES keys.
	SE(SE_CONFIG_REG) = SE_CONFIG_ENC_MODE(MODE_KEY128) | SE_CONFIG_ENC_ALG(ALG_AES_ENC) | SE_CONFIG_DST(DST_MEMORY);

	for (u32 i = 0; i < SE_AES_KEYSLOT_COUNT; i++)
	{
		SE(SE_CONTEXT_SAVE_CONFIG_REG) = SE_CONTEXT_SRC(AES_KEYTABLE) | SE_KEYTABLE_DST_KEY_INDEX(i) |
										 SE_CONTEXT_AES_KEY_INDEX(0)  | SE_CONTEXT_AES_WORD_QUAD(KEYS_0_3);

		SE(SE_CRYPTO_LAST_BLOCK) = 0;
		_se_execute_oneshot(SE_OP_CTX_SAVE, aligned_buf, SE_AES_BLOCK_SIZE, NULL, 0);
		memcpy(keys + i * keysize, aligned_buf, SE_AES_BLOCK_SIZE);

		if (keysize > SE_KEY_128_SIZE)
		{
			SE(SE_CONTEXT_SAVE_CONFIG_REG) = SE_CONTEXT_SRC(AES_KEYTABLE) | SE_KEYTABLE_DST_KEY_INDEX(i) |
											 SE_CONTEXT_AES_KEY_INDEX(0)  | SE_CONTEXT_AES_WORD_QUAD(KEYS_4_7);

			SE(SE_CRYPTO_LAST_BLOCK) = 0;
			_se_execute_oneshot(SE_OP_CTX_SAVE, aligned_buf, SE_AES_BLOCK_SIZE, NULL, 0);
			memcpy(keys + i * keysize + SE_AES_BLOCK_SIZE, aligned_buf, SE_AES_BLOCK_SIZE);
		}
	}

	// Save SRK to PMC secure scratches.
	SE(SE_CONTEXT_SAVE_CONFIG_REG) = SE_CONTEXT_SRC(SRK);
	SE(SE_CRYPTO_LAST_BLOCK)       = 0;
	_se_execute_oneshot(SE_OP_CTX_SAVE, NULL, 0, NULL, 0);

	// End context save.
	SE(SE_CONFIG_REG) = 0;
	_se_execute_oneshot(SE_OP_CTX_SAVE, NULL, 0, NULL, 0);

	// Get SRK.
	u32 srk[4];
	srk[0] = PMC(APBDEV_PMC_SECURE_SCRATCH4);
	srk[1] = PMC(APBDEV_PMC_SECURE_SCRATCH5);
	srk[2] = PMC(APBDEV_PMC_SECURE_SCRATCH6);
	srk[3] = PMC(APBDEV_PMC_SECURE_SCRATCH7);

	// Decrypt context.
	se_aes_key_clear(3);
	se_aes_key_set(3, srk, SE_KEY_128_SIZE);
	se_aes_crypt_cbc(3, DECRYPT, keys, SE_AES_KEYSLOT_COUNT * keysize, keys, SE_AES_KEYSLOT_COUNT * keysize);
	se_aes_key_clear(3);
}

int se_aes_cmac_128(u32 ks, void *dst, const void *src, u32 src_size)
{
	int res = 0;

	u32 tmp1[SE_KEY_128_SIZE / sizeof(u32)] = {0};
	u32 tmp2[SE_AES_BLOCK_SIZE / sizeof(u32)] = {0};
	u8 *key = (u8 *)tmp1;
	u8 *last_block = (u8 *)tmp2;

	se_aes_iv_clear(ks);
	se_aes_iv_updated_clear(ks);

	// Generate sub key
	if (!se_aes_crypt_hash(ks, ENCRYPT, key, SE_KEY_128_SIZE, key, SE_KEY_128_SIZE))
		goto out;

	_gf256_mul_x(key);
	if (src_size & 0xF)
		_gf256_mul_x(key);

	SE(SE_CONFIG_REG) = SE_CONFIG_ENC_MODE(MODE_KEY128) | SE_CONFIG_ENC_ALG(ALG_AES_ENC) | SE_CONFIG_DST(DST_HASHREG);
	SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks) | SE_CRYPTO_INPUT_SEL(INPUT_MEMORY) |
		SE_CRYPTO_XOR_POS(XOR_TOP) | SE_CRYPTO_VCTRAM_SEL(VCTRAM_AESOUT) | SE_CRYPTO_HASH(HASH_ENABLE) |
		SE_CRYPTO_CORE_SEL(CORE_ENCRYPT);
	se_aes_iv_clear(ks);
	se_aes_iv_updated_clear(ks);

	u32 num_blocks = (src_size + 0xf) >> 4;
	if (num_blocks > 1)
	{
		SE(SE_CRYPTO_BLOCK_COUNT_REG) = num_blocks - 2;
		if (!_se_execute_oneshot(SE_OP_START, NULL, 0, src, src_size))
			goto out;
		SE(SE_CRYPTO_CONFIG_REG) |= SE_CRYPTO_IV_SEL(IV_UPDATED);
	}

	if (src_size & 0xf)
	{
		memcpy(last_block, src + (src_size & ~0xf), src_size & 0xf);
		last_block[src_size & 0xf] = 0x80;
	}
	else if (src_size >= SE_AES_BLOCK_SIZE)
	{
		memcpy(last_block, src + src_size - SE_AES_BLOCK_SIZE, SE_AES_BLOCK_SIZE);
	}

	for (u32 i = 0; i < SE_KEY_128_SIZE; i++)
		last_block[i] ^= key[i];

	SE(SE_CRYPTO_BLOCK_COUNT_REG) = 0;
	res = _se_execute_oneshot(SE_OP_START, NULL, 0, last_block, SE_AES_BLOCK_SIZE);

	u32 *dst32 = (u32 *)dst;
	for (u32 i = 0; i < (SE_KEY_128_SIZE / 4); i++)
		dst32[i] = SE(SE_HASH_RESULT_REG + (i * 4));

out:
	return res;
}
