/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2020 CTCaer
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

#ifndef _LIST_H_
#define _LIST_H_

#include <utils/types.h>

/*! Initialize list. */
#define LIST_INIT(name) link_t name = {&name, &name}

/*! Initialize static list. */
#define LIST_INIT_STATIC(name) static link_t name = {&name, &name}

/*! Iterate over all list links. */
#define LIST_FOREACH(iter, list) \
	for(link_t *iter = (list)->next; iter != (list); iter = iter->next)

/*! Safely iterate over all list links. */
#define LIST_FOREACH_SAFE(iter, list) \
	for(link_t *iter = (list)->next, *safe = iter->next; iter != (list); iter = safe, safe = iter->next)

/*! Iterate over all list members and make sure that the list has at least one entry. */
#define LIST_FOREACH_ENTRY(etype, iter, list, mn) \
	if ((list)->next != (list)) \
		for(etype *iter = CONTAINER_OF((list)->next, etype, mn); &iter->mn != (list); iter = CONTAINER_OF(iter->mn.next, etype, mn))

typedef struct _link_t
{
	struct _link_t *prev;
	struct _link_t *next;
} link_t;

static inline void link_init(link_t *l)
{
	l->prev = NULL;
	l->next = NULL;
}

static inline int link_used(link_t *l)
{
	if(l->next == NULL)
		return 1;
	return 0;
}

static inline void list_init(link_t *lh)
{
	lh->prev = lh;
	lh->next = lh;
}

static inline void list_prepend(link_t *lh, link_t *l)
{
	l->next = lh->next;
	l->prev = lh;
	lh->next->prev = l;
	lh->next = l;
}

static inline void list_append(link_t *lh, link_t *l)
{
	l->prev = lh->prev;
	l->next = lh;
	lh->prev->next = l;
	lh->prev = l;
}

static inline void list_remove(link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
	link_init(l);
}

static inline int list_empty(link_t *lh)
{
	if(lh->next == lh)
		return 1;
	return 0;
}

#endif
