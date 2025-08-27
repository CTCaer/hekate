/*
 * Copyright (c) 2018-2025 CTCaer
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
#include <stdlib.h>

#include "dirlist.h"
#include <libs/fatfs/ff.h>
#include <mem/heap.h>
#include <utils/types.h>

dirlist_t *dirlist(const char *directory, const char *pattern, u32 flags)
{
	int res = 0;
	u32 k = 0;
	DIR dir;
	FILINFO fno;
	bool show_hidden = !!(flags & DIR_SHOW_HIDDEN);
	bool show_dirs   = !!(flags & DIR_SHOW_DIRS);
	bool ascii_order = !!(flags & DIR_ASCII_ORDER);

	dirlist_t *dir_entries = (dirlist_t *)malloc(sizeof(dirlist_t));

	// Setup pointer tree.
	for (u32 i = 0; i < DIR_MAX_ENTRIES; i++)
		dir_entries->name[i] = &dir_entries->data[i * 256];

	if (!pattern && !f_opendir(&dir, directory))
	{
		for (;;)
		{
			res = f_readdir(&dir, &fno);
			if (res || !fno.fname[0])
				break;

			bool curr_parse = show_dirs ? (fno.fattrib & AM_DIR) : !(fno.fattrib & AM_DIR);

			if (curr_parse)
			{
				if ((fno.fname[0] != '.') && (show_hidden || !(fno.fattrib & AM_HID)))
				{
					strcpy(&dir_entries->data[k * 256], fno.fname);
					if (++k >= DIR_MAX_ENTRIES)
						break;
				}
			}
		}
		f_closedir(&dir);
	}
	else if (pattern && !f_findfirst(&dir, &fno, directory, pattern) && fno.fname[0])
	{
		do
		{
			if (!(fno.fattrib & AM_DIR) && (fno.fname[0] != '.') && (show_hidden || !(fno.fattrib & AM_HID)))
			{
				strcpy(&dir_entries->data[k * 256], fno.fname);
				if (++k >= DIR_MAX_ENTRIES)
					break;
			}
			res = f_findnext(&dir, &fno);
		} while (fno.fname[0] && !res);
		f_closedir(&dir);
	}

	if (!k)
	{
		free(dir_entries);

		return NULL;
	}

	// Terminate name list.
	dir_entries->name[k] = NULL;

	// Choose list ordering.
	int (*strcmpex)(const char* str1, const char* str2) = ascii_order ? strcmp : strcasecmp;

	// Reorder ini files Alphabetically.
	for (u32 i = 0; i < k - 1 ; i++)
	{
		for (u32 j = i + 1; j < k; j++)
		{
			if (strcmpex(dir_entries->name[i], dir_entries->name[j]) > 0)
			{
				char *tmp = dir_entries->name[i];
				dir_entries->name[i] = dir_entries->name[j];
				dir_entries->name[j] = tmp;
			}
		}
	}

	return dir_entries;
}
