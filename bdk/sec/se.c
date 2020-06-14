/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
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
#include "se_t210.h"
#include <mem/heap.h>
#include <soc/bpmp.h>
#include <soc/t210.h>
#include <utils/util.h>

typedef struct _se_ll_t
{
	vu32 num;
	vu32 addr;
	vu32 size;
} se_ll_t;

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

static void _se_ll_init(se_ll_t *ll, u32 addr, u32 size)
{
	ll->num = 0;
	ll->addr = addr;
	ll->size = size;
}

static void _se_ll_set(se_ll_t *dst, se_ll_t *src)
{
	SE(SE_IN_LL_ADDR_REG_OFFSET) = (u32)src;
	SE(SE_OUT_LL_ADDR_REG_OFFSET) = (u32)dst;
}

static int _se_wait()
{
	while (!(SE(SE_INT_STATUS_REG_OFFSET) & SE_INT_OP_DONE(INT_SET)))
		;
	if (SE(SE_INT_STATUS_REG_OFFSET) & SE_INT_ERROR(INT_SET) ||
		SE(SE_STATUS_0) & SE_STATUS_0_STATE_WAIT_IN ||
		SE(SE_ERR_STATUS_0) != SE_ERR_STATUS_0_SE_NS_ACCESS_CLEAR)
		return 0;
	return 1;
}

se_ll_t *ll_dst, *ll_src;
static int _se_execute(u32 op, void *dst, u32 dst_size, const void *src, u32 src_size, bool is_oneshot)
{
	ll_dst = NULL;
	ll_src = NULL;

	if (dst)
	{
		ll_dst = (se_ll_t *)malloc(sizeof(se_ll_t));
		_se_ll_init(ll_dst, (u32)dst, dst_size);
	}

	if (src)
	{
		ll_src = (se_ll_t *)malloc(sizeof(se_ll_t));
		_se_ll_init(ll_src, (u32)src, src_size);
	}

	_se_ll_set(ll_dst, ll_src);

	SE(SE_ERR_STATUS_0) = SE(SE_ERR_STATUS_0);
	SE(SE_INT_STATUS_REG_OFFSET) = SE(SE_INT_STATUS_REG_OFFSET);

	bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);

	SE(SE_OPERATION_REG_OFFSET) = SE_OPERATION(op);

	if (is_oneshot)
	{
		int res = _se_wait();

		bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);

		if (src)
			free(ll_src);
		if (dst)
			free(ll_dst);

		return res;
	}

	return 1;
}

static int _se_execute_finalize()
{
	int res = _se_wait();

	bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);

	if (ll_src)
	{
		free(ll_src);
		ll_src = NULL;
	}
	if (ll_dst)
	{
		free(ll_dst);
		ll_dst = NULL;
	}

	return res;
}

static int _se_execute_oneshot(u32 op, void *dst, u32 dst_size, const void *src, u32 src_size)
{
	return _se_execute(op, dst, dst_size, src, src_size, true);
}

static int _se_execute_one_block(u32 op, void *dst, u32 dst_size, const void *src, u32 src_size)
{
	if (!src || !dst)
		return 0;

	u8 *block = (u8 *)malloc(0x10);
	memset(block, 0, 0x10);

	SE(SE_BLOCK_COUNT_REG_OFFSET) = 0;

	memcpy(block, src, src_size);
	int res = _se_execute_oneshot(op, block, 0x10, block, 0x10);
	memcpy(dst, block, dst_size);

	free(block);
	return res;
}

static void _se_aes_ctr_set(void *ctr)
{
	u32 *data = (u32 *)ctr;
	for (u32 i = 0; i < 4; i++)
		SE(SE_CRYPTO_CTR_REG_OFFSET + 4 * i) = data[i];
}

void se_rsa_acc_ctrl(u32 rs, u32 flags)
{
	if (flags & SE_RSA_KEY_TBL_DIS_KEY_ALL_FLAG)
		SE(SE_RSA_KEYTABLE_ACCESS_REG_OFFSET + 4 * rs) =
			((flags >> SE_RSA_KEY_TBL_DIS_KEYUSE_FLAG_SHIFT) & SE_RSA_KEY_TBL_DIS_KEYUSE_FLAG) |
			((flags & SE_RSA_KEY_TBL_DIS_KEY_READ_UPDATE_FLAG) ^ SE_RSA_KEY_TBL_DIS_KEY_ALL_COMMON_FLAG);
	if (flags & SE_RSA_KEY_TBL_DIS_KEY_LOCK_FLAG)
		SE(SE_RSA_KEYTABLE_ACCESS_LOCK_OFFSET) &= ~(1 << rs);
}

void se_key_acc_ctrl(u32 ks, u32 flags)
{
	if (flags & SE_KEY_TBL_DIS_KEY_ACCESS_FLAG)
		SE(SE_KEY_TABLE_ACCESS_REG_OFFSET + 4 * ks) = ~flags;
	if (flags & SE_KEY_TBL_DIS_KEY_LOCK_FLAG)
		SE(SE_KEY_TABLE_ACCESS_LOCK_OFFSET) &= ~(1 << ks);
}

u32 se_key_acc_ctrl_get(u32 ks)
{
	return SE(SE_KEY_TABLE_ACCESS_REG_OFFSET + 4 * ks);
}

void se_aes_key_set(u32 ks, void *key, u32 size)
{
	u32 *data = (u32 *)key;
	for (u32 i = 0; i < size / 4; i++)
	{
		SE(SE_KEYTABLE_REG_OFFSET) = SE_KEYTABLE_SLOT(ks) | i;
		SE(SE_KEYTABLE_DATA0_REG_OFFSET) = data[i];
	}
}

void se_aes_key_clear(u32 ks)
{
	for (u32 i = 0; i < TEGRA_SE_AES_MAX_KEY_SIZE / 4; i++)
	{
		SE(SE_KEYTABLE_REG_OFFSET) = SE_KEYTABLE_SLOT(ks) | i;
		SE(SE_KEYTABLE_DATA0_REG_OFFSET) = 0;
	}
}

int se_aes_unwrap_key(u32 ks_dst, u32 ks_src, const void *input)
{
	SE(SE_CONFIG_REG_OFFSET) = SE_CONFIG_DEC_ALG(ALG_AES_DEC) | SE_CONFIG_DST(DST_KEYTAB);
	SE(SE_CRYPTO_REG_OFFSET) = SE_CRYPTO_KEY_INDEX(ks_src) | SE_CRYPTO_CORE_SEL(CORE_DECRYPT);
	SE(SE_BLOCK_COUNT_REG_OFFSET) = 0;
	SE(SE_CRYPTO_KEYTABLE_DST_REG_OFFSET) = SE_CRYPTO_KEYTABLE_DST_KEY_INDEX(ks_dst);

	return _se_execute_oneshot(OP_START, NULL, 0, input, 0x10);
}

int se_aes_crypt_ecb(u32 ks, u32 enc, void *dst, u32 dst_size, const void *src, u32 src_size)
{
	if (enc)
	{
		SE(SE_CONFIG_REG_OFFSET) = SE_CONFIG_ENC_ALG(ALG_AES_ENC) | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_REG_OFFSET) = SE_CRYPTO_KEY_INDEX(ks) | SE_CRYPTO_CORE_SEL(CORE_ENCRYPT);
	}
	else
	{
		SE(SE_CONFIG_REG_OFFSET) = SE_CONFIG_DEC_ALG(ALG_AES_DEC) | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_REG_OFFSET) = SE_CRYPTO_KEY_INDEX(ks) | SE_CRYPTO_CORE_SEL(CORE_DECRYPT);
	}
	SE(SE_BLOCK_COUNT_REG_OFFSET) = (src_size >> 4) - 1;
	return _se_execute_oneshot(OP_START, dst, dst_size, src, src_size);
}

int se_aes_crypt_cbc(u32 ks, u32 enc, void *dst, u32 dst_size, const void *src, u32 src_size)
{
	if (enc)
	{
		SE(SE_CONFIG_REG_OFFSET) = SE_CONFIG_ENC_ALG(ALG_AES_ENC) | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_REG_OFFSET) = SE_CRYPTO_KEY_INDEX(ks) | SE_CRYPTO_VCTRAM_SEL(VCTRAM_AESOUT) |
			SE_CRYPTO_CORE_SEL(CORE_ENCRYPT) | SE_CRYPTO_XOR_POS(XOR_TOP);
	}
	else
	{
		SE(SE_CONFIG_REG_OFFSET) = SE_CONFIG_DEC_ALG(ALG_AES_DEC) | SE_CONFIG_DST(DST_MEMORY);
		SE(SE_CRYPTO_REG_OFFSET) = SE_CRYPTO_KEY_INDEX(ks) | SE_CRYPTO_VCTRAM_SEL(VCTRAM_PREVAHB) |
			SE_CRYPTO_CORE_SEL(CORE_DECRYPT) | SE_CRYPTO_XOR_POS(XOR_BOTTOM);
	}
	SE(SE_BLOCK_COUNT_REG_OFFSET) = (src_size >> 4) - 1;
	return _se_execute_oneshot(OP_START, dst, dst_size, src, src_size);
}

int se_aes_crypt_block_ecb(u32 ks, u32 enc, void *dst, const void *src)
{
	return se_aes_crypt_ecb(ks, enc, dst, 0x10, src, 0x10);
}

int se_aes_crypt_ctr(u32 ks, void *dst, u32 dst_size, const void *src, u32 src_size, void *ctr)
{
	SE(SE_SPARE_0_REG_OFFSET) = 1;
	SE(SE_CONFIG_REG_OFFSET) = SE_CONFIG_ENC_ALG(ALG_AES_ENC) | SE_CONFIG_DST(DST_MEMORY);
	SE(SE_CRYPTO_REG_OFFSET) = SE_CRYPTO_KEY_INDEX(ks) | SE_CRYPTO_CORE_SEL(CORE_ENCRYPT) |
		SE_CRYPTO_XOR_POS(XOR_BOTTOM) | SE_CRYPTO_INPUT_SEL(INPUT_LNR_CTR) | SE_CRYPTO_CTR_VAL(1);
	_se_aes_ctr_set(ctr);

	u32 src_size_aligned = src_size & 0xFFFFFFF0;
	u32 src_size_delta = src_size & 0xF;

	if (src_size_aligned)
	{
		SE(SE_BLOCK_COUNT_REG_OFFSET) = (src_size >> 4) - 1;
		if (!_se_execute_oneshot(OP_START, dst, dst_size, src, src_size_aligned))
			return 0;
	}

	if (src_size - src_size_aligned && src_size_aligned < dst_size)
		return _se_execute_one_block(OP_START, dst + src_size_aligned,
			MIN(src_size_delta, dst_size - src_size_aligned),
			src + src_size_aligned, src_size_delta);

	return 1;
}

int se_aes_xts_crypt_sec(u32 ks1, u32 ks2, u32 enc, u64 sec, void *dst, void *src, u32 secsize)
{
	int res = 0;
	u8 *tweak = (u8 *)malloc(0x10);
	u8 *pdst = (u8 *)dst;
	u8 *psrc = (u8 *)src;

	//Generate tweak.
	for (int i = 0xF; i >= 0; i--)
	{
		tweak[i] = sec & 0xFF;
		sec >>= 8;
	}
	if (!se_aes_crypt_block_ecb(ks1, 1, tweak, tweak))
		goto out;

	//We are assuming a 0x10-aligned sector size in this implementation.
	for (u32 i = 0; i < secsize / 0x10; i++)
	{
		for (u32 j = 0; j < 0x10; j++)
			pdst[j] = psrc[j] ^ tweak[j];
		if (!se_aes_crypt_block_ecb(ks2, enc, pdst, pdst))
			goto out;
		for (u32 j = 0; j < 0x10; j++)
			pdst[j] = pdst[j] ^ tweak[j];
		_gf256_mul_x(tweak);
		psrc += 0x10;
		pdst += 0x10;
	}

	res = 1;

out:;
	free(tweak);
	return res;
}

int se_aes_xts_crypt(u32 ks1, u32 ks2, u32 enc, u64 sec, void *dst, void *src, u32 secsize, u32 num_secs)
{
	u8 *pdst = (u8 *)dst;
	u8 *psrc = (u8 *)src;

	for (u32 i = 0; i < num_secs; i++)
		if (!se_aes_xts_crypt_sec(ks1, ks2, enc, sec + i, pdst + secsize * i, psrc + secsize * i, secsize))
			return 0;

	return 1;
}

int se_calc_sha256(void *hash, u32 *msg_left, const void *src, u32 src_size, u64 total_size, u32 sha_cfg, bool is_oneshot)
{
	int res;
	u32 *hash32 = (u32 *)hash;

	if (src_size > 0xFFFFFF || (u32)hash % 4 || !hash) // Max 16MB - 1 chunks and aligned x4 hash buffer.
		return 0;

	// Setup config for SHA256.
	SE(SE_CONFIG_REG_OFFSET) = SE_CONFIG_ENC_MODE(MODE_SHA256) | SE_CONFIG_ENC_ALG(ALG_SHA) | SE_CONFIG_DST(DST_HASHREG);
	SE(SE_SHA_CONFIG_REG_OFFSET) = sha_cfg;
	SE(SE_BLOCK_COUNT_REG_OFFSET) = 0;

	// Set total size to current buffer size if empty.
	if (!total_size)
		total_size = src_size;

	// Set total size: BITS(src_size), up to 2 EB.
	SE(SE_SHA_MSG_LENGTH_0_REG_OFFSET) = (u32)(total_size << 3);
	SE(SE_SHA_MSG_LENGTH_1_REG_OFFSET) = (u32)(total_size >> 29);
	SE(SE_SHA_MSG_LENGTH_2_REG_OFFSET) = 0;
	SE(SE_SHA_MSG_LENGTH_3_REG_OFFSET) = 0;

	// Set size left to hash.
	SE(SE_SHA_MSG_LEFT_0_REG_OFFSET) = (u32)(total_size << 3);
	SE(SE_SHA_MSG_LEFT_1_REG_OFFSET) = (u32)(total_size >> 29);
	SE(SE_SHA_MSG_LEFT_2_REG_OFFSET) = 0;
	SE(SE_SHA_MSG_LEFT_3_REG_OFFSET) = 0;

	// If we hash in chunks, copy over the intermediate.
	if (sha_cfg == SHA_CONTINUE && msg_left)
	{
		// Restore message left to process.
		SE(SE_SHA_MSG_LEFT_0_REG_OFFSET) = msg_left[0];
		SE(SE_SHA_MSG_LEFT_1_REG_OFFSET) = msg_left[1];

		// Restore hash reg.
		for (u32 i = 0; i < 8; i++)
			SE(SE_HASH_RESULT_REG_OFFSET + (i << 2)) = byte_swap_32(hash32[i]);
	}

	// Trigger the operation.
	res = _se_execute(OP_START, NULL, 0, src, src_size, is_oneshot);

	if (is_oneshot)
	{
		// Backup message left.
		if (msg_left)
		{
			msg_left[0] = SE(SE_SHA_MSG_LEFT_0_REG_OFFSET);
			msg_left[1] = SE(SE_SHA_MSG_LEFT_1_REG_OFFSET);
		}

		// Copy output hash.
		for (u32 i = 0; i < 8; i++)
			hash32[i] = byte_swap_32(SE(SE_HASH_RESULT_REG_OFFSET + (i << 2)));
	}

	return res;
}

int se_calc_sha256_oneshot(void *hash, const void *src, u32 src_size)
{
	return se_calc_sha256(hash, NULL, src, src_size, 0, SHA_INIT_HASH, true);
}

int se_calc_sha256_finalize(void *hash, u32 *msg_left)
{
	u32 *hash32 = (u32 *)hash;
	int res = _se_execute_finalize();

	// Backup message left.
	if (msg_left)
	{
		msg_left[0] = SE(SE_SHA_MSG_LEFT_0_REG_OFFSET);
		msg_left[1] = SE(SE_SHA_MSG_LEFT_1_REG_OFFSET);
	}

	// Copy output hash.
	for (u32 i = 0; i < 8; i++)
		hash32[i] = byte_swap_32(SE(SE_HASH_RESULT_REG_OFFSET + (i << 2)));

	return res;
}

int se_gen_prng128(void *dst)
{
	// Setup config for X931 PRNG.
	SE(SE_CONFIG_REG_OFFSET) = SE_CONFIG_ENC_MODE(MODE_KEY128) | SE_CONFIG_ENC_ALG(ALG_RNG) | SE_CONFIG_DST(DST_MEMORY);
	SE(SE_CRYPTO_REG_OFFSET) = SE_CRYPTO_HASH(HASH_DISABLE) | SE_CRYPTO_XOR_POS(XOR_BYPASS) | SE_CRYPTO_INPUT_SEL(INPUT_RANDOM);

	SE(SE_RNG_CONFIG_REG_OFFSET) = SE_RNG_CONFIG_SRC(RNG_SRC_ENTROPY) | SE_RNG_CONFIG_MODE(RNG_MODE_NORMAL);
	//SE(SE_RNG_SRC_CONFIG_REG_OFFSET) =
	//		SE_RNG_SRC_CONFIG_ENT_SRC(RNG_SRC_RO_ENT_ENABLE) | SE_RNG_SRC_CONFIG_ENT_SRC_LOCK(RNG_SRC_RO_ENT_LOCK_ENABLE);
	SE(SE_RNG_RESEED_INTERVAL_REG_OFFSET) = 1;

	SE(SE_BLOCK_COUNT_REG_OFFSET) = (16 >> 4) - 1;

	// Trigger the operation.
	return _se_execute_oneshot(OP_START, dst, 16, NULL, 0);
}

void se_get_aes_keys(u8 *buf, u8 *keys, u32 keysize)
{
	u8 *aligned_buf = (u8 *)ALIGN((u32)buf, 0x40);

	// Set Secure Random Key.
	SE(SE_CONFIG_REG_OFFSET) = SE_CONFIG_ENC_MODE(MODE_KEY128) | SE_CONFIG_ENC_ALG(ALG_RNG) | SE_CONFIG_DST(DST_SRK);
	SE(SE_CRYPTO_REG_OFFSET) = SE_CRYPTO_KEY_INDEX(0) | SE_CRYPTO_CORE_SEL(CORE_ENCRYPT) | SE_CRYPTO_INPUT_SEL(INPUT_RANDOM);
	SE(SE_RNG_CONFIG_REG_OFFSET) = SE_RNG_CONFIG_SRC(RNG_SRC_ENTROPY) | SE_RNG_CONFIG_MODE(RNG_MODE_FORCE_RESEED);
	SE(SE_CRYPTO_LAST_BLOCK) = 0;
	_se_execute_oneshot(OP_START, NULL, 0, NULL, 0);

	// Save AES keys.
	SE(SE_CONFIG_REG_OFFSET) = SE_CONFIG_ENC_MODE(MODE_KEY128) | SE_CONFIG_ENC_ALG(ALG_AES_ENC) | SE_CONFIG_DST(DST_MEMORY);

	for (u32 i = 0; i < TEGRA_SE_KEYSLOT_COUNT; i++)
	{
		SE(SE_CONTEXT_SAVE_CONFIG_REG_OFFSET) = SE_CONTEXT_SAVE_SRC(AES_KEYTABLE) |
			(i << SE_KEY_INDEX_SHIFT) | SE_CONTEXT_SAVE_WORD_QUAD(KEYS_0_3);

		SE(SE_CRYPTO_LAST_BLOCK) = 0;
		_se_execute_oneshot(OP_CTX_SAVE, aligned_buf, 0x10, NULL, 0);
		memcpy(keys + i * keysize, aligned_buf, 0x10);

		if (keysize > 0x10)
		{
			SE(SE_CONTEXT_SAVE_CONFIG_REG_OFFSET) = SE_CONTEXT_SAVE_SRC(AES_KEYTABLE) |
				(i << SE_KEY_INDEX_SHIFT) | SE_CONTEXT_SAVE_WORD_QUAD(KEYS_4_7);

			SE(SE_CRYPTO_LAST_BLOCK) = 0;
			_se_execute_oneshot(OP_CTX_SAVE, aligned_buf, 0x10, NULL, 0);
			memcpy(keys + i * keysize + 0x10, aligned_buf, 0x10);
		}
	}

	// Save SRK to PMC secure scratches.
	SE(SE_CONTEXT_SAVE_CONFIG_REG_OFFSET) = SE_CONTEXT_SAVE_SRC(SRK);
	SE(0x80) = 0; // SE_CRYPTO_LAST_BLOCK
	_se_execute_oneshot(OP_CTX_SAVE, NULL, 0, NULL, 0);

	// End context save.
	SE(SE_CONFIG_REG_OFFSET) = 0;
	_se_execute_oneshot(OP_CTX_SAVE, NULL, 0, NULL, 0);

	// Get SRK.
	u32 srk[4];
	srk[0] = PMC(0xC0);
	srk[1] = PMC(0xC4);
	srk[2] = PMC(0x224);
	srk[3] = PMC(0x228);

	// Decrypt context.
	se_aes_key_clear(3);
	se_aes_key_set(3, srk, 0x10);
	se_aes_crypt_cbc(3, 0, keys, TEGRA_SE_KEYSLOT_COUNT * keysize, keys, TEGRA_SE_KEYSLOT_COUNT * keysize);
	se_aes_key_clear(3);
}
