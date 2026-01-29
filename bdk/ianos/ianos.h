/*
 * Copyright (c) 2018 M4xw
 * Copyright (c) 2018-2026 CTCaer
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

#ifndef IANOS_H
#define IANOS_H

#include <utils/types.h>
#include <module.h>

typedef enum
{
	IA_DRAM_LIB   = 0, // DRAM library.
	IA_IRAM_LIB   = 1, // IRAM library. No support for now.
	IA_AUTO_LIB   = 2, // AUTO library. Defaults to DRAM for now.
	IA_SHARED_LIB = BIT(7) // Shared library mask. No support for now.
} ianos_type_t;

typedef struct _ianos_lib_t
{
	uintptr_t epaddr;
	void *buf;
	void *private;
	ianos_type_t type;
	bdk_params_t *bdk;
} ianos_lib_t;

int       ianos_loader(ianos_lib_t *lib, char *path);
uintptr_t ianos_static_module(char *path, void *private); // Session-lived DRAM lib.

#endif
