/*
 * Copyright (c) 2019 CTCaer
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

#ifndef _INIPATCH_H_
#define _INIPATCH_H_

#include <bdk.h>

typedef struct _ini_patchset_t
{
	char *name;
	u32 offset;  // section + offset of patch to apply.
	u32 length;  // In bytes, 0 means last patch.
	u8 *srcData; // That must match.
	u8 *dstData; // Gets replaced with.
	link_t link;
} ini_patchset_t;

typedef struct _ini_kip_sec_t
{
	char *name;
	u8 hash[8];
	link_t pts;
	link_t link;
} ini_kip_sec_t;

int ini_patch_parse(link_t *dst, char *ini_path);

#endif
