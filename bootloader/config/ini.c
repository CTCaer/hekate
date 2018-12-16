/*
 * Copyright (c) 2018 naehrwert
 * Copyright (C) 2018 CTCaer
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
	char *res = (char *)malloc(strlen(str) + 1);
	strcpy(res, str);
	return res;
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
				if (csec)
				{
					list_append(dst, &csec->link);
					csec = NULL;
				}

				u32 i;
				for (i = 0; i < lblen  && lbuf[i] != ']' && lbuf[i] != '\n' && lbuf[i] != '\r'; i++)
					;
				lbuf[i] = 0;

				csec = (ini_sec_t *)malloc(sizeof(ini_sec_t));
				csec->name = _strdup(&lbuf[1]);
				csec->type = INI_CHOICE;
				list_init(&csec->kvs);
			}
			else if (lblen > 2 && lbuf[0] == '{') //Create new caption.
			{
				if (csec)
				{
					list_append(dst, &csec->link);
					csec = NULL;
				}

				u32 i;
				for (i = 0; i < lblen && lbuf[i] != '}' && lbuf[i] != '\n' && lbuf[i] != '\r'; i++)
					;
				lbuf[i] = 0;

				csec = (ini_sec_t *)malloc(sizeof(ini_sec_t));
				csec->name = _strdup(&lbuf[1]);
				csec->type = INI_CAPTION;
				csec->color = 0xFF0AB9E6;
			}
			else if (lblen > 2 && lbuf[0] == '#') //Create empty lines and comments.
			{
				if (csec)
				{
					list_append(dst, &csec->link);
					csec = NULL;
				}

				u32 i;
				for (i = 0; i < lblen && lbuf[i] != '\n' && lbuf[i] != '\r'; i++)
					;
				lbuf[i] = 0;

				csec = (ini_sec_t *)malloc(sizeof(ini_sec_t));
				csec->name = _strdup(&lbuf[1]);
				csec->type = INI_COMMENT;
			}
			else if (lblen < 2)
			{
				if (csec)
				{
					list_append(dst, &csec->link);
					csec = NULL;
				}

				csec = (ini_sec_t *)malloc(sizeof(ini_sec_t));
				csec->name = NULL;
				csec->type = INI_NEWLINE;
			}
			else if (csec && csec->type == INI_CHOICE) //Extract key/value.
			{
				u32 i;
				for (i = 0; i < lblen && lbuf[i] != '=' && lbuf[i] != '\n' && lbuf[i] != '\r'; i++)
					;
				lbuf[i] = 0;

				ini_kv_t *kv = (ini_kv_t *)malloc(sizeof(ini_kv_t));
				kv->key = _strdup(&lbuf[0]);
				kv->val = _strdup(&lbuf[i + 1]);
				list_append(&csec->kvs, &kv->link);
			}
		} while (!f_eof(&fp));

		f_close(&fp);

		if (csec)
			list_append(dst, &csec->link);

	} while (is_dir);

	free(filename);
	free(filelist);

	return 1;
}

void ini_free(link_t *dst)
{
	if (!dst->prev || !dst->next)
		return;

	LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, dst, link)
	{
		if (ini_sec->type == INI_CHOICE)
		{
			LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
			{
				free(kv->key);
				free(kv->val);
				//free(kv);
			}
		}
		free(ini_sec->name);
		//free(ini_sec);
	}

	list_init(dst);
}

ini_sec_t *ini_clone_section(ini_sec_t *cfg)
{
	if (cfg == NULL)
		return NULL;

	ini_sec_t *csec = (ini_sec_t *)malloc(sizeof(ini_sec_t));
	list_init(&csec->kvs);

	LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg->kvs, link)
	{
		ini_kv_t *kvcfg = (ini_kv_t *)malloc(sizeof(ini_kv_t));
		kvcfg->key = _strdup(kv->key);
		kvcfg->val = _strdup(kv->val);
		list_append(&csec->kvs, &kvcfg->link);
	}

	return csec;
}

void ini_free_section(ini_sec_t *cfg)
{
	if (cfg == NULL)
		return;

	LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg->kvs, link)
	{
		free(kv->key);
		free(kv->val);
		//free(kv);
	}
	//free(cfg);

	cfg = NULL;
}

char *ini_check_payload_section(ini_sec_t *cfg)
{
	char *path = NULL;

	if (cfg == NULL)
		return NULL;

	LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg->kvs, link)
	{
		if (!strcmp("payload", kv->key))
		{
			if (!path)
				path = _strdup(kv->val);
		}
	}

	if (path)
		return path;
	else
		return NULL;
}
