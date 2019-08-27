/*
 * Copyright (c) 2018 naehrwert
 * Copyright (C) 2018-2019 CTCaer
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
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../utils/dirlist.h"

static char *_strdup(char *str)
{
	if (!str)
		return NULL;

	// Remove starting space.
	if (str[0] == ' ' && strlen(str))
		str++;

	char *res = (char *)malloc(strlen(str) + 1);
	strcpy(res, str);

	// Remove trailing space.
	if (strlen(res) && res[strlen(res) - 1] == ' ')
		res[strlen(res) - 1] = 0;

	return res;
}

u32 _find_section_name(char *lbuf, u32 lblen, char schar)
{
	u32 i;
	for (i = 0; i < lblen  && lbuf[i] != schar && lbuf[i] != '\n' && lbuf[i] != '\r'; i++)
		;
	lbuf[i] = 0;

	return i;
}

ini_sec_t *_ini_create_section(link_t *dst, ini_sec_t *csec, char *name, u8 type)
{
	if (csec)
	{
		list_append(dst, &csec->link);
		csec = NULL;
	}

	csec = (ini_sec_t *)malloc(sizeof(ini_sec_t));
	csec->name = _strdup(name);
	csec->type = type;

	return csec;
}

int ini_parse(link_t *dst, char *ini_path, bool is_dir)
{
	u32 lblen;
	u32 pathlen = strlen(ini_path);
	u32 k = 0;
	char lbuf[512];
	char *filelist = NULL;
	FIL fp;
	ini_sec_t *csec = NULL;

	char *filename = (char *)malloc(256);

	memcpy(filename, ini_path, pathlen + 1);

	// Get all ini filenames.
	if (is_dir)
	{
		filelist = dirlist(filename, "*.ini", false);
		if (!filelist)
		{
			free(filename);
			return 0;
		}
		memcpy(filename + pathlen, "/", 2);
		pathlen++;
	}

	do
	{
		// Copy ini filename in path string.
		if (is_dir)
		{
			if (filelist[k * 256])
			{
				memcpy(filename + pathlen, &filelist[k * 256], strlen(&filelist[k * 256]) + 1);
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

		do
		{
			// Fetch one line.
			lbuf[0] = 0;
			f_gets(lbuf, 512, &fp);
			lblen = strlen(lbuf);

			// Remove trailing newline.
			if (lbuf[lblen - 1] == '\n' || lbuf[lblen - 1] == '\r')
				lbuf[lblen - 1] = 0;

			if (lblen > 2 && lbuf[0] == '[') // Create new section.
			{
				_find_section_name(lbuf, lblen, ']');

				csec = _ini_create_section(dst, csec, &lbuf[1], INI_CHOICE);
				list_init(&csec->kvs);
			}
			else if (lblen > 2 && lbuf[0] == '{') //Create new caption.
			{
				_find_section_name(lbuf, lblen, '}');

				csec = _ini_create_section(dst, csec, &lbuf[1], INI_CAPTION);
				csec->color = 0xFF0AB9E6;
			}
			else if (lblen > 2 && lbuf[0] == '#') //Create empty lines and comments.
			{
				_find_section_name(lbuf, lblen, '\0');

				csec = _ini_create_section(dst, csec, &lbuf[1], INI_COMMENT);
			}
			else if (lblen < 2)
			{
				csec = _ini_create_section(dst, csec, NULL, INI_NEWLINE);
			}
			else if (csec && csec->type == INI_CHOICE) //Extract key/value.
			{
				u32 i = _find_section_name(lbuf, lblen, '=');

				ini_kv_t *kv = (ini_kv_t *)malloc(sizeof(ini_kv_t));
				kv->key = _strdup(&lbuf[0]);
				kv->val = _strdup(&lbuf[i + 1]);
				list_append(&csec->kvs, &kv->link);
			}
		} while (!f_eof(&fp));

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

char *ini_check_payload_section(ini_sec_t *cfg)
{
	if (cfg == NULL)
		return NULL;

	LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg->kvs, link)
	{
		if (!strcmp("payload", kv->key))
			return kv->val;
	}

	return NULL;
}
