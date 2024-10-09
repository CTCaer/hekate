/*
 * Copyright (c) 2018-2024 CTCaer
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

#include <utils/types.h>

#define DIR_MAX_ENTRIES 64

typedef struct _dirlist_t
{
	char *name[DIR_MAX_ENTRIES];
	char  data[DIR_MAX_ENTRIES * 256];
} dirlist_t;

dirlist_t *dirlist(const char *directory, const char *pattern, bool includeHiddenFiles, bool parse_dirs);
