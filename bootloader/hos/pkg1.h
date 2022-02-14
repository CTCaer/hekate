/*
 * Copyright (c) 2018 naehrwert
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
#define PKG1_BOOTLOADER_MAIN_OFFSET   0x100000
#define PKG1_BOOTLOADER_BACKUP_OFFSET 0x140000
#define PKG1_HOS_KEYBLOBS_OFFSET      0x180000

#define PKG1_ERISTA_ON_MARIKO_MAGIC 0xE59FD00C // For 4.0.0 Erista and up.
#define PKG1_MARIKO_ON_ERISTA_MAGIC 0x40010040 // Mariko pkg1 entrypoint.

typedef struct _patch_t
{
	u32 off;
	u32 val;
} patch_t;

#define PATCHSET_DEF(name, ...) \
	patch_t name[] = { \
		__VA_ARGS__, \
		{ 0xFFFFFFFF, 0xFFFFFFFF } \
	}

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
	u16 kb;
	u16 fuses;
	u16 tsec_off;
	u16 pkg11_off;
	u32 secmon_base;
	u32 warmboot_base;
	patch_t *secmon_patchset;
} pkg1_id_t;

typedef struct _pk11_hdr_t
{
	u32 magic;
	u32 wb_size;
	u32 wb_off;
	u32 pad;
	u32 ldr_size;
	u32 ldr_off;
	u32 sm_size;
	u32 sm_off;
} pk11_hdr_t;

const pkg1_id_t *pkg1_get_latest();
const pkg1_id_t *pkg1_identify(u8 *pkg1);
int  pkg1_decrypt(const pkg1_id_t *id, u8 *pkg1);
const u8 *pkg1_unpack(void *wm_dst, u32 *wb_sz, void *sm_dst, void *ldr_dst, const pkg1_id_t *id, u8 *pkg1);
void pkg1_secmon_patch(void *hos_ctxt, u32 secmon_base, bool t210b01);
void pkg1_warmboot_patch(void *hos_ctxt);
int  pkg1_warmboot_config(void *hos_ctxt, u32 warmboot_base);
void pkg1_warmboot_rsa_mod(u32 warmboot_base);

#endif
