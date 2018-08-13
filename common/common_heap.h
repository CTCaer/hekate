/*
 * Copyright (c) 2018 naehrwert
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

#pragma once
//TODO: Move it to BDK
#include "../bootloader/utils/types.h"

typedef struct _hnode
{
	int used;
	u32 size;
	struct _hnode *prev;
	struct _hnode *next;
} hnode_t;

typedef struct _heap
{
	u32 start;
	hnode_t *first;
} heap_t;
