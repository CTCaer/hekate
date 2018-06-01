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

#include "types.h"

#define PATCHSET_DEF(name, ...) \
	patch_t name[] = { \
		__VA_ARGS__, \
		{ 0xFFFFFFFF, 0xFFFFFFFF } \
	}

typedef struct _patch_t
{
	u32 off;
	u32 val;
} patch_t;

typedef struct _pkg1_id_t
{
	const char *id;
	u32 kb;
	u32 tsec_off;
	u32 pkg11_off;
	u32 sec_map[3];
	u32 secmon_base;
	patch_t *secmon_patchset;
	patch_t *kernel_patchset;
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

const pkg1_id_t *pkg1_identify(u8 *pkg1);
void pkg1_decrypt(const pkg1_id_t *id, u8 *pkg1);
void pkg1_unpack(void *warmboot_dst, void *secmon_dst, const pkg1_id_t *id, u8 *pkg1);

#endif
