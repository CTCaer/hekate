/*
 * Copyright (c) 2018 naehrwert
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

#ifndef MBR_GPT_H
#define MBR_GPT_H

#include <utils/types.h>

typedef struct _mbr_chs_t
{
	u8 head;
	u8 sector;
	u8 cylinder;
} __attribute__((packed)) mbr_chs_t;

typedef struct _mbr_part_t
{
	u8  status;
	mbr_chs_t start_sct_chs;
	u8  type;
	mbr_chs_t end_sct_chs;
	u32 start_sct;
	u32 size_sct;
} __attribute__((packed)) mbr_part_t;

typedef struct _mbr_t
{
/* 0x000 */	u8  bootstrap[440];
/* 0x1B8 */	u32 signature;
/* 0x1BC */	u16 copy_protected;
/* 0x1BE */	mbr_part_t partitions[4];
/* 0x1FE */	u16 boot_signature;
} __attribute__((packed)) mbr_t;

typedef struct _gpt_entry_t
{
/* 0x00 */	u8  type_guid[0x10];
/* 0x10 */	u8  part_guid[0x10];
/* 0x20 */	u64 lba_start;
/* 0x28 */	u64 lba_end;
/* 0x30 */	u64 attrs;
/* 0x38 */	u16 name[36];
} gpt_entry_t;

typedef struct _gpt_header_t
{
/* 0x00 */	u64 signature; // "EFI PART"
/* 0x08 */	u32 revision;
/* 0x0C */	u32 size;
/* 0x10 */	u32 crc32;
/* 0x14 */	u32 res1;
/* 0x18 */	u64 my_lba;
/* 0x20 */	u64 alt_lba;
/* 0x28 */	u64 first_use_lba;
/* 0x30 */	u64 last_use_lba;
/* 0x38 */	u8  disk_guid[0x10];
/* 0x48 */	u64 part_ent_lba;
/* 0x50 */	u32 num_part_ents;
/* 0x54 */	u32 part_ent_size;
/* 0x58 */	u32 part_ents_crc32;
/* 0x5C */	u8  res2[420]; // Used as first 3 partition entries backup for HOS.
} gpt_header_t;

typedef struct _gpt_t
{
	gpt_header_t header;
	gpt_entry_t  entries[128];
} gpt_t;

#endif
