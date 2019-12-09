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

#include <string.h>
#include "heap.h"
#include "../gfx/gfx.h"
#include "../../common/common_heap.h"

static void _heap_create(heap_t *heap, u32 start)
{
	heap->start = start;
	heap->first = NULL;
}

// Node info is before node address.
static u32 _heap_alloc(heap_t *heap, u32 size)
{
	hnode_t *node, *new;

	// Align to cache line size.
	size = ALIGN(size, sizeof(hnode_t));

	if (!heap->first)
	{
		node = (hnode_t *)heap->start;
		node->used = 1;
		node->size = size;
		node->prev = NULL;
		node->next = NULL;
		heap->first = node;

		return (u32)node + sizeof(hnode_t);
	}

	node = heap->first;
	while (true)
	{
		if (!node->used && (size <= node->size))
		{
			u32 new_size = node->size - size;
			new = (hnode_t *)((u32)node + sizeof(hnode_t) + size);

			// If there's aligned leftover space, create a new node.
			if (new_size >= (sizeof(hnode_t) << 2))
			{
				new->size = new_size - sizeof(hnode_t);
				new->used = 0;
				new->next = node->next;
				new->next->prev = new;
				new->prev = node;
				node->next = new;
			}
			else
				size += new_size;

			node->size = size;
			node->used = 1;

			return (u32)node + sizeof(hnode_t);
		}
		if (node->next)
			node = node->next;
		else
			break;
	}

	new = (hnode_t *)((u32)node + sizeof(hnode_t) + node->size);
	new->used = 1;
	new->size = size;
	new->prev = node;
	new->next = NULL;
	node->next = new;

	return (u32)new + sizeof(hnode_t);
}

static void _heap_free(heap_t *heap, u32 addr)
{
	hnode_t *node = (hnode_t *)(addr - sizeof(hnode_t));
	node->used = 0;
	node = heap->first;
	while (node)
	{
		if (!node->used)
		{
			if (node->prev && !node->prev->used)
			{
				node->prev->size += node->size + sizeof(hnode_t);
				node->prev->next = node->next;
				if (node->next)
					node->next->prev = node->prev;
			}
		}
		node = node->next;
	}
}

heap_t _heap;

void heap_init(u32 base)
{
	_heap_create(&_heap, base);
}

void *malloc(u32 size)
{
	return (void *)_heap_alloc(&_heap, size);
}

void *calloc(u32 num, u32 size)
{
	void *res = (void *)_heap_alloc(&_heap, num * size);
	memset(res, 0, num * size);
	return res;
}

void free(void *buf)
{
	if ((u32)buf >= _heap.start)
		_heap_free(&_heap, (u32)buf);
}

void heap_monitor(heap_monitor_t *mon, bool print_node_stats)
{
	u32 count = 0;
	memset(mon, 0, sizeof(heap_monitor_t));

	hnode_t *node = _heap.first;
	while (true)
	{
		if (node->used)
			mon->used += node->size + sizeof(hnode_t);
		else
			mon->total += node->size + sizeof(hnode_t);

		if (print_node_stats)
			gfx_printf("%3d - %d, addr: 0x%08X, size: 0x%X\n",
				count, node->used, (u32)node + sizeof(hnode_t), node->size);

		count++;
		
		if (node->next)
			node = node->next;
		else
			break;
	}
	mon->total += mon->used;
}
