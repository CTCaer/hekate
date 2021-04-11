/*
 * Common Module Header
 * Copyright (c) 2018 M4xw
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

#ifndef _MODULE_H_
#define _MODULE_H_

#include <stddef.h>
#include <mem/heap.h>

#define IANOS_EXT0 0x304E4149

// Module Callback
typedef void (*cbMainModule_t)(const char *s);
typedef void (*memcpy_t)(void *, void *, size_t);
typedef void (*memset_t)(void *, int, size_t);
typedef int  (*reg_voltage_set_t)(u32, u32);

typedef struct _bdkParams_t
{
	void *gfxCon;
	void *gfxCtx;
	heap_t *sharedHeap;
	memcpy_t memcpy;
	memset_t memset;
	u32 extension_magic;
	reg_voltage_set_t reg_voltage_set;
} *bdkParams_t;

// Module Entrypoint
typedef void (*moduleEntrypoint_t)(void *, bdkParams_t);

#endif
