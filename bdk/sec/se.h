/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2019-2026 CTCaer
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

#ifndef _SE_H_
#define _SE_H_

#include "se_t210.h"
#include <utils/types.h>

void se_rsa_acc_ctrl(u32 rs, u32 flags);
void se_key_acc_ctrl(u32 ks, u32 flags);
u32  se_key_acc_ctrl_get(u32 ks);

/*! AES Key Management Functions */
void se_aes_key_set(u32 ks, const void *key, u32 size);
void se_aes_iv_set(u32 ks, const void *iv, u32 size);
void se_aes_key_get(u32 ks, void *key, u32 size);
void se_aes_key_clear(u32 ks);
void se_aes_iv_clear(u32 ks);
int  se_aes_unwrap_key(u32 ks_dst, u32 ks_src, const void *seed);
void se_aes_ctx_get_keys(u8 *buf, u8 *keys, u32 keysize);
/*! Encryption Functions */
int  se_aes_crypt_ecb(u32 ks, int enc, void *dst, const void *src, u32 size);
int  se_aes_crypt_cbc(u32 ks, int enc, void *dst, const void *src, u32 size);
int  se_aes_crypt_ofb(u32 ks, void *dst, const void *src, u32 size);
int  se_aes_crypt_ctr(u32 ks, void *dst, const void *src, u32 size, void *ctr);
int  se_aes_crypt_xts_sec(u32 tweak_ks, u32 crypt_ks, int enc, u64 sec, void *dst, void *src, u32 secsize);
int  se_aes_crypt_xts_sec_nx(u32 tweak_ks, u32 crypt_ks, int enc, u64 sec, u8 *tweak, bool regen_tweak, u32 tweak_exp, void *dst, void *src, u32 sec_size);
int  se_aes_crypt_xts(u32 tweak_ks, u32 crypt_ks, int enc, u64 sec, void *dst, void *src, u32 secsize, u32 num_secs);
/*! Hashing Functions */
int  se_sha_hash_256_async(void *hash, const void *src, u32 size);
int  se_sha_hash_256_oneshot(void *hash, const void *src, u32 size);
int  se_sha_hash_256_partial_start(void *hash, const void *src, u32 size, bool is_oneshot);
int  se_sha_hash_256_partial_update(void *hash, const void *src, u32 size, bool is_oneshot);
int  se_sha_hash_256_partial_end(void *hash, u64 total_size, const void *src, u32 src_size, bool is_oneshot);
int  se_sha_hash_256_finalize(void *hash);
int  se_aes_hash_cmac(u32 ks, void *hash, const void *src, u32 size);
/*! Random Functions */
int  se_rng_pseudo(void *dst, u32 size);

#endif
