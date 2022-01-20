/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2019-2022 CTCaer
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
void se_get_aes_keys(u8 *buf, u8 *keys, u32 keysize);
void se_aes_key_set(u32 ks, void *key, u32 size);
void se_aes_iv_set(u32 ks, void *iv);
void se_aes_key_get(u32 ks, void *key, u32 size);
void se_aes_key_clear(u32 ks);
void se_aes_iv_clear(u32 ks);
int  se_aes_unwrap_key(u32 ks_dst, u32 ks_src, const void *input);
int  se_aes_crypt_cbc(u32 ks, u32 enc, void *dst, u32 dst_size, const void *src, u32 src_size);
int  se_aes_crypt_ecb(u32 ks, u32 enc, void *dst, u32 dst_size, const void *src, u32 src_size);
int  se_aes_crypt_block_ecb(u32 ks, u32 enc, void *dst, const void *src);
int  se_aes_xts_crypt_sec(u32 tweak_ks, u32 crypt_ks, u32 enc, u64 sec, void *dst, void *src, u32 secsize);
int  se_aes_xts_crypt_sec_nx(u32 tweak_ks, u32 crypt_ks, u32 enc, u64 sec, u8 *tweak, bool regen_tweak, u32 tweak_exp, void *dst, void *src, u32 sec_size);
int  se_aes_xts_crypt(u32 tweak_ks, u32 crypt_ks, u32 enc, u64 sec, void *dst, void *src, u32 secsize, u32 num_secs);
int  se_aes_crypt_ctr(u32 ks, void *dst, u32 dst_size, const void *src, u32 src_size, void *ctr);
int  se_calc_sha256(void *hash, u32 *msg_left, const void *src, u32 src_size, u64 total_size, u32 sha_cfg, bool is_oneshot);
int  se_calc_sha256_oneshot(void *hash, const void *src, u32 src_size);
int  se_calc_sha256_finalize(void *hash, u32 *msg_left);
int  se_gen_prng128(void *dst);

#endif
