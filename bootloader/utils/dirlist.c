/*
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

#include <string.h>
#include <stdlib.h>

#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../utils/types.h"

char *dirlist(const char *directory, const char *pattern, bool includeHiddenFiles)
{
	u8 max_entries = 61;

	int res = 0;
	u32 i = 0, j = 0, k = 0;
	DIR dir;
	static FILINFO fno;
	
	char *dir_entries = (char *)calloc(max_entries, 256);
	char *temp = (char *)calloc(1, 256);

	if (!pattern && !f_opendir(&dir, directory))
	{
		for (;;)
		{
			res = f_readdir(&dir, &fno);
			if (res || !fno.fname[0])
				break;
			if (!(fno.fattrib & AM_DIR) && (fno.fname[0] != '.') && (includeHiddenFiles || !(fno.fattrib & AM_HID)))
			{
				memcpy(dir_entries + (k * 256), fno.fname, strlen(fno.fname) + 1);
				k++;
				if (k > (max_entries - 1))
					break;
			}
		}
		f_closedir(&dir);
	}
	else if (pattern && !f_findfirst(&dir, &fno, directory, pattern) && fno.fname[0])
	{
		do
		{
			if (!(fno.fattrib & AM_DIR) && (fno.fname[0] != '.') && (includeHiddenFiles || !(fno.fattrib & AM_HID)))
			{
				memcpy(dir_entries + (k * 256), fno.fname, strlen(fno.fname) + 1);
				k++;
				if (k > (max_entries - 1))
					break;
			}
			res = f_findnext(&dir, &fno);
		} while (fno.fname[0] && !res);
		f_closedir(&dir);
	}

	if (!k)
	{
		free(temp);
		free(dir_entries);

		return NULL;
	}

	// Reorder ini files by ASCII ordering.
	for (i = 0; i < k - 1 ; i++)
	{
		for (j = i + 1; j < k; j++)
		{
			if (strcmp(&dir_entries[i * 256], &dir_entries[j * 256]) > 0) 
			{
				memcpy(temp, &dir_entries[i * 256], strlen(&dir_entries[i * 256]) + 1);
				memcpy(&dir_entries[i * 256], &dir_entries[j * 256], strlen(&dir_entries[j * 256]) + 1);
				memcpy(&dir_entries[j * 256], temp, strlen(temp) + 1);
			}
		}
	}

	free(temp);

	return dir_entries;
}
