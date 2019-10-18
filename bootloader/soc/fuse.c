/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 shuffle2
 * Copyright (c) 2018 balika011
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

#include <string.h>

#include "../soc/fuse.h"
#include "../soc/t210.h"

#define ARRAYSIZE(x) (sizeof(x) / sizeof(*x))

static const u32 evp_thunk_template[] = {
	0xe92d0007, //   STMFD   SP!, {R0-R2}
	0xe1a0200e, //   MOV     R2, LR
	0xe2422002, //   SUB     R2, R2, #2
	0xe5922000, //   LDR     R2, [R2]
	0xe20220ff, //   AND     R2, R2, #0xFF
	0xe1a02082, //   MOV     R2, R2,LSL#1
	0xe59f001c, //   LDR     R0, =evp_thunk_template
	0xe59f101c, //   LDR     R1, =thunk_end
	0xe0411000, //   SUB     R1, R1, R0
	0xe59f0018, //   LDR     R0, =iram_evp_thunks
	0xe0800001, //   ADD     R0, R0, R1
	0xe0822000, //   ADD     R2, R2, R0
	0xe3822001, //   ORR     R2, R2, #1
	0xe8bd0003, //   LDMFD   SP!, {R0,R1}
	0xe12fff12, //   BX      R2
	0x001007b0, // off_1007EC DCD evp_thunk_template
	0x001007f8, // off_1007F0 DCD thunk_end
	0x40004c30, // off_1007F4 DCD iram_evp_thunks
	// thunk_end is here
};
static const u32 evp_thunk_template_len = sizeof(evp_thunk_template);

// treated as 12bit values
static const u32 hash_vals[] = {1, 2, 4, 8, 0, 3, 5, 6, 7, 9, 10, 11};

void fuse_disable_program()
{
	FUSE(FUSE_DISABLEREGPROGRAM) = 1;
}

u32 fuse_read_odm(u32 idx)
{
	return FUSE(FUSE_RESERVED_ODMX(idx));
}

void fuse_wait_idle()
{
	u32 ctrl;
	do
	{
		ctrl = FUSE(FUSE_CTRL);
	} while (((ctrl >> 16) & 0x1f) != 4);
}

u32 fuse_read(u32 addr)
{
	FUSE(FUSE_ADDR) = addr;
	FUSE(FUSE_CTRL) = (FUSE(FUSE_ADDR) & ~FUSE_CMD_MASK) | FUSE_READ;
	fuse_wait_idle();
	return FUSE(FUSE_RDATA);
}

void fuse_read_array(u32 *words)
{
	for (u32 i = 0; i < 192; i++)
		words[i] = fuse_read(i);
}

static u32 _parity32_even(u32 *words, u32 count)
{
	u32 acc = words[0];
	for (u32 i = 1; i < count; i++)
	{
		acc ^= words[i];
	}
	u32 lo = ((acc & 0xffff) ^ (acc >> 16)) & 0xff;
	u32 hi = ((acc & 0xffff) ^ (acc >> 16)) >> 8;
	u32 x = hi ^ lo;
	lo = ((x & 0xf) ^ (x >> 4)) & 3;
	hi = ((x & 0xf) ^ (x >> 4)) >> 2;
	x = hi ^ lo;

	return (x & 1) ^ (x >> 1);
}

static int _patch_hash_one(u32 *word)
{
	u32 bits20_31 = *word & 0xfff00000;
	u32 parity_bit = _parity32_even(&bits20_31, 1);
	u32 hash = 0;
	for (u32 i = 0; i < 12; i++)
	{
		if (*word & (1 << (20 + i)))
		{
			hash ^= hash_vals[i];
		}
	}
	if (hash == 0)
	{
		if (parity_bit == 0)
		{
			return 0;
		}
		*word ^= 1 << 24;
		return 1;
	}
	if (parity_bit == 0)
	{
		return 3;
	}
	for (u32 i = 0; i < ARRAYSIZE(hash_vals); i++)
	{
		if (hash_vals[i] == hash)
		{
			*word ^= 1 << (20 + i);
			return 1;
		}
	}
	return 2;
}

static int _patch_hash_multi(u32 *words, u32 count)
{
	u32 parity_bit = _parity32_even(words, count);
	u32 bits0_14 = words[0] & 0x7fff;
	u32 bit15 = words[0] & 0x8000;
	u32 bits16_19 = words[0] & 0xf0000;

	u32 hash = 0;
	words[0] = bits16_19;
	for (u32 i = 0; i < count; i++)
	{
		u32 w = words[i];
		if (w)
		{
			for (u32 bitpos = 0; bitpos < 32; bitpos++)
			{
				if ((w >> bitpos) & 1)
				{
					hash ^= 0x4000 + i * 32 + bitpos;
				}
			}
		}
	}
	hash ^= bits0_14;
	// stupid but this is what original code does.
	// equivalent to original words[0] &= 0xfff00000
	words[0] = bits16_19 ^ bit15 ^ bits0_14;

	if (hash == 0)
	{
		if (parity_bit == 0)
		{
			return 0;
		}
		words[0] ^= 0x8000;
		return 1;
	}
	if (parity_bit == 0)
	{
		return 3;
	}
	u32 bitcount = hash - 0x4000;
	if (bitcount < 16 || bitcount >= count * 32)
	{
		u32 num_set = 0;
		for (u32 bitpos = 0; bitpos < 15; bitpos++)
		{
			if ((hash >> bitpos) & 1)
			{
				num_set++;
			}
		}
		if (num_set != 1)
		{
			return 2;
		}
		words[0] ^= hash;
		return 1;
	}
	words[bitcount / 32] ^= 1 << (hash & 0x1f);
	return 1;
}

int fuse_read_ipatch(void (*ipatch)(u32 offset, u32 value))
{
	u32 words[80];
	u32 word_count;
	u32 word_addr;
	u32 word0 = 0;
	u32 total_read = 0;

	word_count = FUSE(FUSE_FIRST_BOOTROM_PATCH_SIZE);
	word_count &= 0x7F;
	word_addr = 191;

	while (word_count)
	{
		total_read += word_count;
		if (total_read >= ARRAYSIZE(words))
		{
			break;
		}

		for (u32 i = 0; i < word_count; i++)
			words[i] = fuse_read(word_addr--);

		word0 = words[0];
		if (_patch_hash_multi(words, word_count) >= 2)
		{
			return 1;
		}
		u32 ipatch_count = (words[0] >> 16) & 0xF;
		if (ipatch_count)
		{
			for (u32 i = 0; i < ipatch_count; i++)
			{
				u32 word = words[i + 1];
				u32 addr = (word >> 16) * 2;
				u32 data = word & 0xFFFF;

				ipatch(addr, data);
			}
		}
		words[0] = word0;
		if ((word0 >> 25) == 0)
			break;
		if (_patch_hash_one(&word0) >= 2)
		{
			return 3;
		}
		word_count = word0 >> 25;
	}

	return 0;
}

int fuse_read_evp_thunk(u32 *iram_evp_thunks, u32 *iram_evp_thunks_len)
{
	u32 words[80];
	u32 word_count;
	u32 word_addr;
	u32 word0 = 0;
	u32 total_read = 0;
	int evp_thunk_written = 0;
	void *evp_thunk_dst_addr = 0;

	memset(iram_evp_thunks, 0, *iram_evp_thunks_len);

	word_count = FUSE(FUSE_FIRST_BOOTROM_PATCH_SIZE);
	word_count &= 0x7F;
	word_addr = 191;

	while (word_count)
	{
		total_read += word_count;
		if (total_read >= ARRAYSIZE(words))
		{
			break;
		}

		for (u32 i = 0; i < word_count; i++)
			words[i] = fuse_read(word_addr--);

		word0 = words[0];
		if (_patch_hash_multi(words, word_count) >= 2)
		{
			return 1;
		}
		u32 ipatch_count = (words[0] >> 16) & 0xF;
		u32 insn_count = word_count - ipatch_count - 1;
		if (insn_count)
		{
			if (!evp_thunk_written)
			{
				evp_thunk_dst_addr = (void *)iram_evp_thunks;

				memcpy(evp_thunk_dst_addr, (void *)evp_thunk_template, evp_thunk_template_len);
				evp_thunk_dst_addr += evp_thunk_template_len;
				evp_thunk_written = 1;
				*iram_evp_thunks_len = evp_thunk_template_len;

				//write32(TEGRA_EXCEPTION_VECTORS_BASE + 0x208, iram_evp_thunks);
			}

			u32 thunk_patch_len = insn_count * sizeof(u32);
			memcpy(evp_thunk_dst_addr, &words[ipatch_count + 1], thunk_patch_len);
			evp_thunk_dst_addr += thunk_patch_len;
			*iram_evp_thunks_len += thunk_patch_len;
		}
		words[0] = word0;
		if ((word0 >> 25) == 0)
			break;
		if (_patch_hash_one(&word0) >= 2)
		{
			return 3;
		}
		word_count = word0 >> 25;
	}

	return 0;
}

bool fuse_check_patched_rcm()
{
	// Check if XUSB in use.
	if (FUSE(FUSE_RESERVED_SW) & (1<<7))
		return true;

	// Check if RCM is ipatched.
	u32 word_count = FUSE(FUSE_FIRST_BOOTROM_PATCH_SIZE) & 0x7F;
	u32 word_addr = 191;

	while (word_count)
	{
		u32 word0 = fuse_read(word_addr);
		u32 ipatch_count = (word0 >> 16) & 0xF;

		for (u32 i = 0; i < ipatch_count; i++)
		{
			u32 word = fuse_read(word_addr - (i + 1));
			u32 addr = (word >> 16) * 2;
			if (addr == 0x769A)
				return true;
		}

		word_addr -= word_count;
		word_count = word0 >> 25;
	}

	return false;
}
