/*
 * Copyright (c) 2018 M4xw
 * Copyright (c) 2018 CTCaer
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

typedef enum
{
	DRAM_LIB = 0, // DRAM library.
	EXEC_ELF = 1, // Executable elf that does not return.
	DR64_LIB = 2, // AARCH64 DRAM library.
	AR64_ELF = 3, // Executable elf that does not return.
	KEEP_IN_RAM = (1 << 31)  // Shared library mask.
} elfType_t;

uintptr_t ianos_loader(char *path, elfType_t type, void* config);

#endif