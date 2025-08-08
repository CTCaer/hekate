/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2022-2025 CTCaer
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

#ifndef _PKG1_H_
#define _PKG1_H_

#include <bdk.h>

#define PKG1_MAGIC 0x31314B50

#define PK11_SECTION_WB 0
#define PK11_SECTION_LD 1
#define PK11_SECTION_SM 2

#define PKG1_BOOTLOADER_SIZE          SZ_256K
#define PKG1_BOOTLOADER_MAIN_OFFSET   (0x100000 / EMMC_BLOCKSIZE)
#define PKG1_BOOTLOADER_BACKUP_OFFSET (0x140000 / EMMC_BLOCKSIZE)
#define PKG1_BOOTLOADER_SAFE_OFFSET   (0x000000 / EMMC_BLOCKSIZE)
#define PKG1_HOS_EKS_OFFSET           (0x180000 / EMMC_BLOCKSIZE)

typedef struct _bl_hdr_t210b01_t
{
/* 0x000 */	u8  aes_mac[0x10];
/* 0x010 */	u8  rsa_sig[0x100];
/* 0x110 */	u8  salt[0x20];
/* 0x130 */	u8  sha256[0x20];
/* 0x150 */	u32 version;
/* 0x154 */	u32 size;
/* 0x158 */	u32 load_addr;
/* 0x15C */	u32 entrypoint;
/* 0x160 */	u8  rsvd[0x10];
} bl_hdr_t210b01_t;

typedef struct _eks_keys_t
{
	u8 master_kekseed[SE_KEY_128_SIZE];
	u8 random_data[0x70];
	u8 package1_key[SE_KEY_128_SIZE];
} eks_keys_t;

typedef struct _pkg1_eks_t
{
	u8 cmac[SE_KEY_128_SIZE];
	u8 ctr[SE_AES_IV_SIZE];
	eks_keys_t keys;
	u8 padding[0x150];
} pkg1_eks_t;

typedef struct _pk1_hdr_t
{
/* 0x00 */	u32 si_sha256; // Secure Init.
/* 0x04 */	u32 sm_sha256; // Secure Monitor.
/* 0x08 */	u32 sl_sha256; // Secure Loader.
/* 0x0C */	u32 unk;       // what's this? It's not warmboot.
/* 0x10 */	char timestamp[14];
/* 0x1E */	u8 keygen;
/* 0x1F */	u8 version;
} pk1_hdr_t;

typedef struct _pkg1_id_t
{
	const char *id;
	u16 mkey;
	u16 tsec_off;
	u32 pkg11_off;
	u32 secmon_base;
	u32 warmboot_base;
} pkg1_id_t;

typedef struct _pk11_hdr_t
{
/* 0x00 */	u32 magic;
/* 0x04 */	u32 wb_size;
/* 0x08 */	u32 wb_off;
/* 0x0C */	u32 pad;
/* 0x10 */	u32 ldr_size;
/* 0x14 */	u32 ldr_off;
/* 0x18 */	u32 sm_size;
/* 0x1C */	u32 sm_off;
} pk11_hdr_t;

const pkg1_id_t *pkg1_identify(u8 *pkg1, char *build_date);
int pkg1_decrypt(const pkg1_id_t *id, u8 *pkg1);
const u8 *pkg1_unpack(void *wm_dst, void *sm_dst, void *ldr_dst, const pkg1_id_t *id, u8 *pkg1);

#endif
