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

#ifndef _TYPES_H_
#define _TYPES_H_

#define NULL ((void *)0)

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define OFFSET_OF(t, m) ((u32)&((t *)NULL)->m)
#define CONTAINER_OF(mp, t, mn) ((t *)((u32)mp - OFFSET_OF(t, mn)))

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long int s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long int u64;
typedef volatile unsigned char vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;

typedef struct _patch_t
{
	u32 off;
	u32 val;
} patch_t;

#endif
