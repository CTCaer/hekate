/*
 * Copyright (c) 2018 naehrwert
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

#include <string.h>

#include "ini.h"
#include <libs/fatfs/ff.h>
#include <mem/heap.h>
#include <utils/dirlist.h>
#include <utils/util.h>

u32 _find_section_name(char *lbuf, u32 lblen, char schar)
{
	u32 i;
	// Depends on 'FF_USE_STRFUNC 2' that removes \r.
	for (i = 0; i < lblen  && lbuf[i] != schar && lbuf[i] != '\n'; i++)
		;
	lbuf[i] = 0;

	return i;
}

ini_sec_t *_ini_create_section(link_t *dst, ini_sec_t *csec, char *name, u8 type)
{
	if (csec)
		list_append(dst, &csec->link);

	// Calculate total allocation size.
	u32 len = name ? strlen(name) + 1 : 0;
	char *buf = zalloc(sizeof(ini_sec_t) + len);

	csec = (ini_sec_t *)buf;
	csec->name = strcpy_ns(buf + sizeof(ini_sec_t), name);
	csec->type = type;

	// Initialize list.
	list_init(&csec->kvs);

	return csec;
}

int ini_parse(link_t *dst, const char *ini_path, bool is_dir)
{
	FIL fp;
	u32 lblen;
	u32 k = 0;
	u32 pathlen = strlen(ini_path);
	ini_sec_t *csec = NULL;

	char *lbuf     = NULL;
	dirlist_t *filelist = NULL;
	char *filename = (char *)malloc(256);

	strcpy(filename, ini_path);

	// Get all ini filenames.
	if (is_dir)
	{
		filelist = dirlist(filename, "*.ini", DIR_ASCII_ORDER);
		if (!filelist)
		{
			free(filename);
			return 0;
		}
		strcpy(filename + pathlen, "/");
		pathlen++;
	}

	do
	{
		// Copy ini filename in path string.
		if (is_dir)
		{
			if (filelist->name[k])
			{
				strcpy(filename + pathlen, filelist->name[k]);
				k++;
			}
			else
				break;
		}

		// Open ini.
		if (f_open(&fp, filename, FA_READ) != FR_OK)
		{
			free(filelist);
			free(filename);

			return 0;
		}

		lbuf = malloc(512);

		do
		{
			// Fetch one line.
			lbuf[0] = 0;
			f_gets(lbuf, 512, &fp);
			lblen = strlen(lbuf);

			// Remove trailing newline. Depends on 'FF_USE_STRFUNC 2' that removes \r.
			if (lblen && lbuf[lblen - 1] == '\n')
				lbuf[lblen - 1] = 0;

			if (lblen > 2 && lbuf[0] == '[') // Create new section.
			{
				_find_section_name(lbuf, lblen, ']');

				csec = _ini_create_section(dst, csec, &lbuf[1], INI_CHOICE);
			}
			else if (lblen > 1 && lbuf[0] == '{') // Create new caption. Support empty caption '{}'.
			{
				_find_section_name(lbuf, lblen, '}');

				csec = _ini_create_section(dst, csec, &lbuf[1], INI_CAPTION);
				csec->color = 0xFF0AB9E6;
			}
			else if (lblen > 2 && lbuf[0] == '#') // Create comment.
			{
				csec = _ini_create_section(dst, csec, &lbuf[1], INI_COMMENT);
			}
			else if (lblen < 2) // Create empty line.
			{
				csec = _ini_create_section(dst, csec, NULL, INI_NEWLINE);
			}
			else if (csec && csec->type == INI_CHOICE) // Extract key/value.
			{
				u32 i = _find_section_name(lbuf, lblen, '=');

				// Calculate total allocation size.
				u32 klen  = strlen(&lbuf[0]) + 1;
				u32 vlen  = strlen(&lbuf[i + 1]) + 1;
				char *buf = zalloc(sizeof(ini_kv_t) + klen + vlen);

				ini_kv_t *kv = (ini_kv_t *)buf;
				buf += sizeof(ini_kv_t);
				kv->key = strcpy_ns(buf, &lbuf[0]);
				buf += klen;
				kv->val = strcpy_ns(buf, &lbuf[i + 1]);
				list_append(&csec->kvs, &kv->link);
			}
		} while (!f_eof(&fp));

		free(lbuf);

		f_close(&fp);

		if (csec)
		{
			list_append(dst, &csec->link);
			if (is_dir)
				csec = NULL;
		}
	} while (is_dir);

	free(filename);
	free(filelist);

	return 1;
}

char *ini_check_special_section(ini_sec_t *cfg)
{
	if (cfg == NULL)
		return NULL;

	LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg->kvs, link)
	{
		if (!strcmp("l4t",          kv->key))
			return ((kv->val[0] == '1') ? (char *)-1 : NULL);
		else if (!strcmp("payload", kv->key))
			return kv->val;
	}

	return NULL;
}

void ini_free(link_t *src)
{
	ini_sec_t *prev_sec = NULL;

	// Parse and free all ini sections.
	LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, src, link)
	{
		ini_kv_t *prev_kv  = NULL;

		// Free all ini key allocations if they exist.
		LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
		{
			// Free previous key.
			if (prev_kv)
				free(prev_kv);

			// Set next key to free.
			prev_kv = kv;
		}

		// Free last key.
		if (prev_kv)
			free(prev_kv);

		// Free previous section.
		if (prev_sec)
			free(prev_sec);

		// Set next section to free.
		prev_sec = ini_sec;
	}

	// Free last section.
	if (prev_sec)
		free(prev_sec);
}
