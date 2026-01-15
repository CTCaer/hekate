/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2026 CTCaer
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
	u32 num;
	u32 addr;
	u32 size;
} se_ll_t;

se_ll_t ll_src, ll_dst; // Must be u32 aligned.
se_ll_t *ll_src_ptr, *ll_dst_ptr;

static void _se_ls_1bit(void *buf)
{
	u8 *block = (u8 *)buf;
	u32 carry = 0;

	for (int i = SE_AES_BLOCK_SIZE - 1; i >= 0; i--)
	{
		u8 b = block[i];
		block[i] = (b << 1) | carry;
		carry = b >> 7;
	}

	if (carry)
		block[SE_AES_BLOCK_SIZE - 1] ^= 0x87;
}

static void _se_ls_1bit_le(void *buf)
{
	u32 *block = (u32 *)buf;
	u32 carry = 0;

	for (u32 i = 0; i < 4; i++)
	{
		u32 b = block[i];
		block[i] = (b << 1) | carry;
		carry = b >> 31;
	}

	if (carry)
		block[0x0] ^= 0x87;
}

static void _se_ll_set(se_ll_t *ll, u32 addr, u32 size)
{
	ll->num  = 0;
	ll->addr = addr;
	ll->size = size & 0xFFFFFF;
}

static int _se_op_wait()
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

	// WAR: Coherency flushing.
	if (ll_dst_ptr)
	{
		// Ensure data is out from SE.
		if (tegra_t210)
			usleep(15); // Worst case scenario.
		else
		{
			// T210B01 has a status bit for that.
			u32 retries = 500000;
			while (SE(SE_STATUS_REG) & SE_STATUS_MEM_IF_BUSY)
			{
				if (!retries)
					return 0;
				usleep(1);
				retries--;
			}
		}

		// Ensure data is out from AHB.
		u32 retries = 500000;
		while (AHB_GIZMO(AHB_ARBITRATION_AHB_MEM_WRQUE_MST_ID) & MEM_WRQUE_SE_MST_ID)
		{
			if (!retries)
				return 0;
			usleep(1);
			retries--;
		}
	}

	return 1;
}

static int _se_execute_finalize()
{
	int res = _se_op_wait();

	// Invalidate data after OP is done.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_INVALID_WAY, false);

	return res;
}

static int _se_execute(u32 op, void *dst, u32 dst_size, const void *src, u32 src_size, bool is_oneshot)
{
	if (dst_size > SE_LL_MAX_SIZE || src_size > SE_LL_MAX_SIZE)
		return 0;

	ll_src_ptr = NULL;
	ll_dst_ptr = NULL;

	if (src)
	{
		ll_src_ptr = &ll_src;
		_se_ll_set(ll_src_ptr, (u32)src, src_size);
	}

	if (dst)
	{
		ll_dst_ptr = &ll_dst;
		_se_ll_set(ll_dst_ptr, (u32)dst, dst_size);
	}

	// Set linked list pointers.
	SE(SE_IN_LL_ADDR_REG)  = (u32)ll_src_ptr;
	SE(SE_OUT_LL_ADDR_REG) = (u32)ll_dst_ptr;

	// Clear status.
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

static int _se_execute_aes_oneshot(void *dst, const void *src, u32 size)
{
	// Set optional memory interface.
	if (dst >= (void *)DRAM_START && src >= (void *)DRAM_START)
		SE(SE_CRYPTO_CONFIG_REG) |= SE_CRYPTO_MEMIF(MEMIF_MCCIF);

	u32 size_aligned = ALIGN_DOWN(size, SE_AES_BLOCK_SIZE);
	u32 size_residue = size % SE_AES_BLOCK_SIZE;
	int res = 1;

	// Handle initial aligned message.
	if (size_aligned)
	{
		SE(SE_CRYPTO_LAST_BLOCK_REG) = (size >> 4) - 1;

		res = _se_execute_oneshot(SE_OP_START, dst, size_aligned, src, size_aligned);
	}

	// Handle leftover partial message.
	if (res && size_residue)
	{
		// Copy message to a block sized buffer in case it's partial.
		u32 block[SE_AES_BLOCK_SIZE / sizeof(u32)] = {0};
		memcpy(block, src + size_aligned, size_residue);

		// Use updated IV for CBC and OFB. Ignored on others.
		SE(SE_CRYPTO_CONFIG_REG) |= SE_CRYPTO_IV_SEL(IV_UPDATED);

		SE(SE_CRYPTO_LAST_BLOCK_REG) = (SE_AES_BLOCK_SIZE >> 4) - 1;

		res = _se_execute_oneshot(SE_OP_START, block, SE_AES_BLOCK_SIZE, block, SE_AES_BLOCK_SIZE);

		// Copy result back.
		memcpy(dst + size_aligned, block, size_residue);
	}

	return res;
}

static void _se_aes_counter_set(const void *ctr)
{
	u32 data[SE_AES_IV_SIZE / sizeof(u32)];
	memcpy(data, ctr, SE_AES_IV_SIZE);

	for (u32 i = 0; i < SE_CRYPTO_LINEAR_CTR_REG_COUNT; i++)
		SE(SE_CRYPTO_LINEAR_CTR_REG + sizeof(u32) * i) = data[i];
}

void se_rsa_acc_ctrl(u32 rs, u32 flags)
{
	if (flags & SE_RSA_KEY_TBL_DIS_KEY_ACCESS_FLAG)
		SE(SE_RSA_KEYTABLE_ACCESS_REG + sizeof(u32) * rs) =
			(((flags >> 4) & SE_RSA_KEY_TBL_DIS_KEYUSE_FLAG) | (flags & SE_RSA_KEY_TBL_DIS_KEY_READ_UPDATE_FLAG)) ^
				SE_RSA_KEY_TBL_DIS_KEY_READ_UPDATE_USE_FLAG;
	if (flags & SE_RSA_KEY_LOCK_FLAG)
		SE(SE_RSA_SECURITY_PERKEY_REG) &= ~BIT(rs);
}

void se_key_acc_ctrl(u32 ks, u32 flags)
{
	if (flags & SE_KEY_TBL_DIS_KEY_ACCESS_FLAG)
		SE(SE_CRYPTO_KEYTABLE_ACCESS_REG + sizeof(u32) * ks) = ~flags;
	if (flags & SE_KEY_LOCK_FLAG)
		SE(SE_CRYPTO_SECURITY_PERKEY_REG) &= ~BIT(ks);
}

u32 se_key_acc_ctrl_get(u32 ks)
{
	return SE(SE_CRYPTO_KEYTABLE_ACCESS_REG + sizeof(u32) * ks);
}

void se_aes_key_set(u32 ks, const void *key, u32 size)
{
	u32 data[SE_AES_MAX_KEY_SIZE / sizeof(u32)];
	memcpy(data, key, size);

	for (u32 i = 0; i < (size / sizeof(u32)); i++)
	{
		// QUAD KEYS_4_7 bit is automatically set by PKT macro.
		SE(SE_CRYPTO_KEYTABLE_ADDR_REG) = SE_KEYTABLE_SLOT(ks) | SE_KEYTABLE_QUAD(KEYS_0_3) | SE_KEYTABLE_PKT(i);
		SE(SE_CRYPTO_KEYTABLE_DATA_REG) = data[i];
	}
}

void se_aes_iv_set(u32 ks, const void *iv, u32 size)
{
	u32 data[SE_AES_MAX_KEY_SIZE / sizeof(u32)];
	memcpy(data, iv, size);

	for (u32 i = 0; i < (size / sizeof(u32)); i++)
	{
		// QUAD UPDATED_IV bit is automatically set by PKT macro.
		SE(SE_CRYPTO_KEYTABLE_ADDR_REG) = SE_KEYTABLE_SLOT(ks) | SE_KEYTABLE_QUAD(ORIGINAL_IV) | SE_KEYTABLE_PKT(i);
		SE(SE_CRYPTO_KEYTABLE_DATA_REG) = data[i];
	}
}

void se_aes_key_get(u32 ks, void *key, u32 size)
{
	u32 data[SE_AES_MAX_KEY_SIZE / sizeof(u32)];

	for (u32 i = 0; i < (size / sizeof(u32)); i++)
	{
		// QUAD KEYS_4_7 bit is automatically set by PKT macro.
		SE(SE_CRYPTO_KEYTABLE_ADDR_REG) = SE_KEYTABLE_SLOT(ks) | SE_KEYTABLE_QUAD(KEYS_0_3) | SE_KEYTABLE_PKT(i);
		data[i] = SE(SE_CRYPTO_KEYTABLE_DATA_REG);
	}

	memcpy(key, data, size);
}

void se_aes_key_clear(u32 ks)
{
	for (u32 i = 0; i < (SE_AES_MAX_KEY_SIZE / sizeof(u32)); i++)
	{
		// QUAD KEYS_4_7 bit is automatically set by PKT macro.
		SE(SE_CRYPTO_KEYTABLE_ADDR_REG) = SE_KEYTABLE_SLOT(ks) | SE_KEYTABLE_QUAD(KEYS_0_3) | SE_KEYTABLE_PKT(i);
		SE(SE_CRYPTO_KEYTABLE_DATA_REG) = 0;
	}
}

void se_aes_iv_clear(u32 ks)
{
	for (u32 i = 0; i < (SE_AES_MAX_KEY_SIZE / sizeof(u32)); i++)
	{
		// QUAD UPDATED_IV bit is automatically set by PKT macro.
		SE(SE_CRYPTO_KEYTABLE_ADDR_REG) = SE_KEYTABLE_SLOT(ks) | SE_KEYTABLE_QUAD(ORIGINAL_IV) | SE_KEYTABLE_PKT(i);
		SE(SE_CRYPTO_KEYTABLE_DATA_REG) = 0;
	}
}

int se_aes_unwrap_key(u32 ks_dst, u32 ks_src, const void *seed)
{
	SE(SE_CONFIG_REG)              = SE_CONFIG_DEC_MODE(MODE_KEY128)   | SE_CONFIG_DEC_ALG(ALG_AES_DEC) | SE_CONFIG_DST(DST_KEYTABLE);
	SE(SE_CRYPTO_CONFIG_REG)       = SE_CRYPTO_KEY_INDEX(ks_src)       | SE_CRYPTO_CORE_SEL(CORE_DECRYPT);
	SE(SE_CRYPTO_LAST_BLOCK_REG)   = (SE_AES_BLOCK_SIZE >> 4) - 1;
	SE(SE_CRYPTO_KEYTABLE_DST_REG) = SE_KEYTABLE_DST_KEY_INDEX(ks_dst) | SE_KEYTABLE_DST_WORD_QUAD(KEYS_0_3);

	return _se_execute_oneshot(SE_OP_START, NULL, 0, seed, SE_KEY_128_SIZE);
}

int se_aes_crypt_ecb(u32 ks, int enc, void *dst, const void *src, u32 size)
{
	if (enc)
	{
		SE(SE_CONFIG_REG)        = SE_CONFIG_ENC_MODE(MODE_KEY128) | SE_CONFIG_ENC_ALG(ALG_AES_ENC)   | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks)         | SE_CRYPTO_CORE_SEL(CORE_ENCRYPT) |
								   SE_CRYPTO_XOR_POS(XOR_BYPASS);
	}
	else
	{
		SE(SE_CONFIG_REG)        = SE_CONFIG_DEC_MODE(MODE_KEY128) | SE_CONFIG_DEC_ALG(ALG_AES_DEC)   | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks)         | SE_CRYPTO_CORE_SEL(CORE_DECRYPT) |
								   SE_CRYPTO_XOR_POS(XOR_BYPASS);
	}

	return _se_execute_aes_oneshot(dst, src, size);
}

int se_aes_crypt_cbc(u32 ks, int enc, void *dst, const void *src, u32 size)
{
	if (enc)
	{
		SE(SE_CONFIG_REG)        = SE_CONFIG_ENC_MODE(MODE_KEY128)  | SE_CONFIG_ENC_ALG(ALG_AES_ENC)      | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks)          | SE_CRYPTO_VCTRAM_SEL(VCTRAM_AESOUT) |
								   SE_CRYPTO_CORE_SEL(CORE_ENCRYPT) | SE_CRYPTO_XOR_POS(XOR_TOP);
	}
	else
	{
		SE(SE_CONFIG_REG)        = SE_CONFIG_DEC_MODE(MODE_KEY128)  | SE_CONFIG_DEC_ALG(ALG_AES_DEC)       | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks)          | SE_CRYPTO_VCTRAM_SEL(VCTRAM_PREVMEM) |
								   SE_CRYPTO_CORE_SEL(CORE_DECRYPT) | SE_CRYPTO_XOR_POS(XOR_BOTTOM);
	}

	return _se_execute_aes_oneshot(dst, src, size);
}

int se_aes_crypt_ofb(u32 ks, void *dst, const void *src, u32 size)
{
	SE(SE_SPARE_REG)         = SE_INPUT_NONCE_LE;
	SE(SE_CONFIG_REG)        = SE_CONFIG_ENC_MODE(MODE_KEY128)  | SE_CONFIG_ENC_ALG(ALG_AES_ENC)      | SE_CONFIG_DST(DST_MEMORY);
	SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks)          | SE_CRYPTO_INPUT_SEL(INPUT_AESOUT) |
							   SE_CRYPTO_CORE_SEL(CORE_ENCRYPT) | SE_CRYPTO_XOR_POS(XOR_BOTTOM);

	return _se_execute_aes_oneshot(dst, src, size);
}

int se_aes_crypt_ctr(u32 ks, void *dst, const void *src, u32 size, void *ctr)
{
	SE(SE_SPARE_REG)         = SE_INPUT_NONCE_LE;
	SE(SE_CONFIG_REG)        = SE_CONFIG_ENC_MODE(MODE_KEY128) | SE_CONFIG_ENC_ALG(ALG_AES_ENC)     | SE_CONFIG_DST(DST_MEMORY);
	SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_KEY_INDEX(ks)         | SE_CRYPTO_CORE_SEL(CORE_ENCRYPT)   |
							   SE_CRYPTO_XOR_POS(XOR_BOTTOM)   | SE_CRYPTO_INPUT_SEL(INPUT_LNR_CTR) |
							   SE_CRYPTO_CTR_CNTN(1);

	_se_aes_counter_set(ctr);

	return _se_execute_aes_oneshot(dst, src, size);
}

int se_aes_crypt_xts_sec(u32 tweak_ks, u32 crypt_ks, int enc, u64 sec, void *dst, void *src, u32 secsize)
{
	int res = 0;
	u32 tmp[SE_AES_BLOCK_SIZE / sizeof(u32)];
	u8 *tweak = (u8 *)tmp;
	u8 *pdst = (u8 *)dst;
	u8 *psrc = (u8 *)src;

	// Generate tweak.
	for (int i = SE_AES_BLOCK_SIZE - 1; i >= 0; i--)
	{
		tweak[i] = sec & 0xFF;
		sec >>= 8;
	}
	if (!se_aes_crypt_ecb(tweak_ks, ENCRYPT, tweak, tweak, SE_AES_BLOCK_SIZE))
		goto out;

	// We are assuming a 0x10-aligned sector size in this implementation.
	for (u32 i = 0; i < secsize / SE_AES_BLOCK_SIZE; i++)
	{
		for (u32 j = 0; j < SE_AES_BLOCK_SIZE; j++)
			pdst[j] = psrc[j] ^ tweak[j];
		if (!se_aes_crypt_ecb(crypt_ks, enc, pdst, pdst, SE_AES_BLOCK_SIZE))
			goto out;
		for (u32 j = 0; j < SE_AES_BLOCK_SIZE; j++)
			pdst[j] = pdst[j] ^ tweak[j];
		_se_ls_1bit(tweak);
		psrc += SE_AES_BLOCK_SIZE;
		pdst += SE_AES_BLOCK_SIZE;
	}

	res = 1;

out:
	return res;
}

int se_aes_crypt_xts_sec_nx(u32 tweak_ks, u32 crypt_ks, int enc, u64 sec, u8 *tweak, bool regen_tweak, u32 tweak_exp, void *dst, void *src, u32 sec_size)
{
	u32 *pdst = (u32 *)dst;
	u32 *psrc = (u32 *)src;
	u32 *ptweak = (u32 *)tweak;

	if (regen_tweak)
	{
		for (int i = SE_AES_BLOCK_SIZE - 1; i >= 0; i--)
		{
			tweak[i] = sec & 0xFF;
			sec >>= 8;
		}
		if (!se_aes_crypt_ecb(tweak_ks, ENCRYPT, tweak, tweak, SE_AES_BLOCK_SIZE))
			return 0;
	}

	// tweak_exp allows using a saved tweak to reduce _se_ls_1bit_le calls.
	for (u32 i = 0; i < (tweak_exp << 5); i++)
		_se_ls_1bit_le(tweak);

	u8 orig_tweak[SE_KEY_128_SIZE] __attribute__((aligned(4)));
	memcpy(orig_tweak, tweak, SE_KEY_128_SIZE);

	// We are assuming a 16 sector aligned size in this implementation.
	for (u32 i = 0; i < (sec_size >> 4); i++)
	{
		for (u32 j = 0; j < (SE_AES_BLOCK_SIZE / sizeof(u32)); j++)
			pdst[j] = psrc[j] ^ ptweak[j];

		_se_ls_1bit_le(tweak);
		psrc += sizeof(u32);
		pdst += sizeof(u32);
	}

	if (!se_aes_crypt_ecb(crypt_ks, enc, dst, dst, sec_size))
		return 0;

	pdst = (u32 *)dst;
	ptweak = (u32 *)orig_tweak;
	for (u32 i = 0; i < (sec_size >> 4); i++)
	{
		for (u32 j = 0; j < (SE_AES_BLOCK_SIZE / sizeof(u32)); j++)
			pdst[j] = pdst[j] ^ ptweak[j];

		_se_ls_1bit_le(orig_tweak);
		pdst += sizeof(u32);
	}

	return 1;
}

int se_aes_crypt_xts(u32 tweak_ks, u32 crypt_ks, int enc, u64 sec, void *dst, void *src, u32 secsize, u32 num_secs)
{
	u8 *pdst = (u8 *)dst;
	u8 *psrc = (u8 *)src;

	for (u32 i = 0; i < num_secs; i++)
		if (!se_aes_crypt_xts_sec(tweak_ks, crypt_ks, enc, sec + i, pdst + secsize * i, psrc + secsize * i, secsize))
			return 0;

	return 1;
}

static void _se_sha_hash_256_get_hash(void *hash)
{
	// Copy output hash.
	u32 hash32[SE_SHA_256_SIZE / sizeof(u32)];
	for (u32 i = 0; i < (SE_SHA_256_SIZE / sizeof(u32)); i++)
		hash32[i] = byte_swap_32(SE(SE_HASH_RESULT_REG + sizeof(u32) * i));
	memcpy(hash, hash32, SE_SHA_256_SIZE);
}

static int _se_sha_hash_256(void *hash, u64 total_size, const void *src, u32 src_size, bool is_oneshot)
{
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

	// Increase leftover size if not last message. (Engine will always stop at src_size.)
	u32 msg_left = src_size;
	if (total_size < src_size)
		msg_left++;

	// Setup config for SHA256.
	SE(SE_CONFIG_REG) = SE_CONFIG_ENC_MODE(MODE_SHA256) | SE_CONFIG_ENC_ALG(ALG_SHA) | SE_CONFIG_DST(DST_HASHREG);

	// Set total size: BITS(total_size), up to 2 EB.
	SE(SE_SHA_MSG_LENGTH_0_REG) = (u32)(total_size << 3);
	SE(SE_SHA_MSG_LENGTH_1_REG) = (u32)(total_size >> 29);
	SE(SE_SHA_MSG_LENGTH_2_REG) = 0;
	SE(SE_SHA_MSG_LENGTH_3_REG) = 0;

	// Set leftover size: BITS(src_size).
	SE(SE_SHA_MSG_LEFT_0_REG) = (u32)(msg_left << 3);
	SE(SE_SHA_MSG_LEFT_1_REG) = (u32)(msg_left >> 29);
	SE(SE_SHA_MSG_LEFT_2_REG) = 0;
	SE(SE_SHA_MSG_LEFT_3_REG) = 0;

	// Set config based on init or partial continuation.
	if (total_size == src_size || !total_size)
		SE(SE_SHA_CONFIG_REG) = SHA_INIT_HASH;
	else
		SE(SE_SHA_CONFIG_REG) = SHA_CONTINUE;

	// Trigger the operation. src vs total size decides if it's partial.
	int res = _se_execute(SE_OP_START, NULL, 0, src, src_size, is_oneshot);

	if (res && is_oneshot)
		_se_sha_hash_256_get_hash(hash);

	return res;
}

int se_sha_hash_256_async(void *hash, const void *src, u32 size)
{
	return _se_sha_hash_256(hash, size, src, size, false);
}

int se_sha_hash_256_oneshot(void *hash, const void *src, u32 size)
{
	return _se_sha_hash_256(hash, size, src, size, true);
}

int se_sha_hash_256_partial_start(void *hash, const void *src, u32 size, bool is_oneshot)
{
	// Check if aligned SHA256 block size.
	if (size % SE_SHA2_MIN_BLOCK_SIZE)
		return 0;

	return _se_sha_hash_256(hash, 0, src, size, is_oneshot);
}

int se_sha_hash_256_partial_update(void *hash, const void *src, u32 size, bool is_oneshot)
{
	// Check if aligned to SHA256 block size.
	if (size % SE_SHA2_MIN_BLOCK_SIZE)
		return 0;

	return _se_sha_hash_256(hash, size - 1, src, size, is_oneshot);
}

int se_sha_hash_256_partial_end(void *hash, u64 total_size, const void *src, u32 src_size, bool is_oneshot)
{
	return _se_sha_hash_256(hash, total_size, src, src_size, is_oneshot);
}

int se_sha_hash_256_finalize(void *hash)
{
	int res = _se_execute_finalize();

	_se_sha_hash_256_get_hash(hash);

	return res;
}

int se_rng_pseudo(void *dst, u32 size)
{
	// Setup config for SP 800-90 PRNG.
	SE(SE_CONFIG_REG)        = SE_CONFIG_ENC_MODE(MODE_KEY128) | SE_CONFIG_ENC_ALG(ALG_RNG)    | SE_CONFIG_DST(DST_MEMORY);
	SE(SE_CRYPTO_CONFIG_REG) = SE_CRYPTO_XOR_POS(XOR_BYPASS)   | SE_CRYPTO_INPUT_SEL(INPUT_RANDOM);
	SE(SE_RNG_CONFIG_REG)    = SE_RNG_CONFIG_SRC(SRC_ENTROPY)  | SE_RNG_CONFIG_MODE(MODE_NORMAL);
	SE(SE_RNG_SRC_CONFIG_REG) |= SE_RNG_SRC_CONFIG_ENTR_SRC(RO_ENTR_ENABLE); // DRBG. Depends on ENTROPY clock.
	SE(SE_RNG_RESEED_INTERVAL_REG) = 4096;

	u32 size_aligned = ALIGN_DOWN(size, SE_RNG_BLOCK_SIZE);
	u32 size_residue = size % SE_RNG_BLOCK_SIZE;
	int res = 0;

	// Handle initial aligned message.
	if (size_aligned)
	{
		SE(SE_CRYPTO_LAST_BLOCK_REG) = (size >> 4) - 1;

		res = _se_execute_oneshot(SE_OP_START, dst, size_aligned, NULL, 0);
	}

	// Handle leftover partial message.
	if (res && size_residue)
	{
		// Copy message to a block sized buffer in case it's partial.
		u32 block[SE_RNG_BLOCK_SIZE / sizeof(u32)] = {0};

		SE(SE_CRYPTO_LAST_BLOCK_REG) = (SE_AES_BLOCK_SIZE >> 4) - 1;

		res = _se_execute_oneshot(SE_OP_START, block, SE_RNG_BLOCK_SIZE, NULL, 0);

		// Copy result back.
		if (res)
			memcpy(dst + size_aligned, block, size_residue);
	}

	return res;
}

void se_aes_ctx_get_keys(u8 *buf, u8 *keys, u32 keysize)
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
	se_aes_key_set(3, srk, SE_KEY_128_SIZE);
	se_aes_crypt_cbc(3, DECRYPT, keys, keys, SE_AES_KEYSLOT_COUNT * keysize);
	se_aes_key_clear(3);
}

int se_aes_hash_cmac(u32 ks, void *hash, const void *src, u32 size)
{
	u32 tmp1[SE_KEY_128_SIZE / sizeof(u32)] = {0};
	u32 tmp2[SE_AES_BLOCK_SIZE / sizeof(u32)] = {0};
	u8 *subkey = (u8 *)tmp1;
	u8 *last_block = (u8 *)tmp2;

	// Generate sub key (CBC with zeroed IV, basically ECB).
	se_aes_iv_clear(ks);
	if (!se_aes_crypt_cbc(ks, ENCRYPT, subkey, subkey, SE_KEY_128_SIZE))
		return 0;

	// Generate K1 subkey.
	_se_ls_1bit(subkey);
	if (size & 0xF)
		_se_ls_1bit(subkey); // Convert to K2.

	// Switch to hash register. The rest of the config is already set.
	SE(SE_CONFIG_REG)        |= SE_CONFIG_DST(DST_HASHREG);
	SE(SE_CRYPTO_CONFIG_REG) |= SE_CRYPTO_HASH(HASH_ENABLE);

	// Initial blocks.
	u32 num_blocks = (size + 0xF) >> 4;
	if (num_blocks > 1)
	{
		SE(SE_CRYPTO_LAST_BLOCK_REG) = num_blocks - 2;

		if (!_se_execute_oneshot(SE_OP_START, NULL, 0, src, size))
			return 0;

		// Use updated IV for next OP as a continuation.
		SE(SE_CRYPTO_CONFIG_REG) |= SE_CRYPTO_IV_SEL(IV_UPDATED);
	}

	// Last block.
	if (size & 0xF)
	{
		memcpy(last_block, src + (size & (~0xF)), size & 0xF);
		last_block[size & 0xF] = 0x80;
	}
	else if (size >= SE_AES_BLOCK_SIZE)
		memcpy(last_block, src + size - SE_AES_BLOCK_SIZE, SE_AES_BLOCK_SIZE);

	for (u32 i = 0; i < SE_KEY_128_SIZE; i++)
		last_block[i] ^= subkey[i];

	SE(SE_CRYPTO_LAST_BLOCK_REG) = (SE_AES_BLOCK_SIZE >> 4) - 1;

	int res = _se_execute_oneshot(SE_OP_START, NULL, 0, last_block, SE_AES_BLOCK_SIZE);

	// Copy output hash.
	if (res)
	{
		u32 *hash32 = (u32 *)hash;
		for (u32 i = 0; i < (SE_AES_CMAC_DIGEST_SIZE / sizeof(u32)); i++)
			hash32[i] = SE(SE_HASH_RESULT_REG + sizeof(u32) * i);
	}

	return res;
}
