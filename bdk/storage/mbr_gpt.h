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
	u8  bootstrap[440];
	u32 signature;
	u16 copy_protected;
	mbr_part_t partitions[4];
	u16 boot_signature;
} __attribute__((packed)) mbr_t;

typedef struct _gpt_entry_t
{
	u8  type_guid[0x10];
	u8  part_guid[0x10];
	u64 lba_start;
	u64 lba_end;
	u64 attrs;
	u16 name[36];
} gpt_entry_t;

typedef struct _gpt_header_t
{
	u64 signature; // "EFI PART"
	u32 revision;
	u32 size;
	u32 crc32;
	u32 res1;
	u64 my_lba;
	u64 alt_lba;
	u64 first_use_lba;
	u64 last_use_lba;
	u8  disk_guid[0x10];
	u64 part_ent_lba;
	u32 num_part_ents;
	u32 part_ent_size;
	u32 part_ents_crc32;
	u8  res2[420]; // Used as first 3 partition entries backup for HOS.
} gpt_header_t;

typedef struct _gpt_t
{
	gpt_header_t header;
	gpt_entry_t  entries[128];
} gpt_t;

#endif
