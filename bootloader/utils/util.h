/*
 * Copyright (c) 2018 naehrwert
 * Copyright (C) 2018 CTCaer
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

#ifndef _UTIL_H_
#define _UTIL_H_

#include "types.h"

#define byte_swap_32(val) __builtin_bswap32(val)

typedef struct _cfg_op_t
{
	u32 off;
	u32 val;
} cfg_op_t;

u32 get_tmr_us();
u32 get_tmr_ms();
u32 get_tmr_s();
void usleep(u32 ticks);
void msleep(u32 milliseconds);
void exec_cfg(u32 *base, const cfg_op_t *ops, u32 num_ops);
u32 crc32c(const void *buf, u32 len);

/* This is a faster implementation of memcmp that checks two u32 values */
/* every 128 Bytes block. Intented only for Backup and Restore          */
u32 memcmp32sparse(const u32 *buf1, const u32 *buf2, u32 len);

char *itoa(int value, char *str, int base);

#endif
