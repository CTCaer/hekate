/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2020 CTCaer
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
#include <gfx_utils.h>

static void _heap_create(heap_t *heap, u32 start)
{
	heap->start = start;
	heap->first = NULL;
}

// Node info is before node address.
static u32 _heap_alloc(heap_t *heap, u32 size)
{
	hnode_t *node, *new_node;

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
		// Check if there's available unused node.
		if (!node->used && (size <= node->size))
		{
			// Size and offset of the new unused node.
			u32 new_size = node->size - size;
			new_node = (hnode_t *)((u32)node + sizeof(hnode_t) + size);

			// If there's aligned unused space from the old node,
			// create a new one and set the leftover size.
			if (new_size >= (sizeof(hnode_t) << 2))
			{
				new_node->size = new_size - sizeof(hnode_t);
				new_node->used = 0;
				new_node->next = node->next;

				// Check that we are not on first node.
				if (new_node->next)
					new_node->next->prev = new_node;

				new_node->prev = node;
				node->next = new_node;
			}
			else // Unused node size is just enough.
				size += new_size;

			node->size = size;
			node->used = 1;

			return (u32)node + sizeof(hnode_t);
		}

		// No unused node found, try the next one.
		if (node->next)
			node = node->next;
		else
			break;
	}

	// No unused node found, create a new one.
	new_node = (hnode_t *)((u32)node + sizeof(hnode_t) + node->size);
	new_node->used = 1;
	new_node->size = size;
	new_node->prev = node;
	new_node->next = NULL;
	node->next = new_node;

	return (u32)new_node + sizeof(hnode_t);
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

void heap_copy(heap_t *heap)
{
	memcpy(&_heap, heap, sizeof(heap_t));
}

void *malloc(u32 size)
{
	return (void *)_heap_alloc(&_heap, size);
}

void *calloc(u32 num, u32 size)
{
	void *res = (void *)_heap_alloc(&_heap, num * size);
	memset(res, 0, ALIGN(num * size, sizeof(hnode_t))); // Clear the aligned size.
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
