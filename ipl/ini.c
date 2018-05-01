/*
* Copyright (c) 2018 naehrwert
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
#include "ff.h"
#include "heap.h"


static char *_strdup(char *str)
{
	char *res = malloc(strlen(str) + 1);
	strcpy(res, str);
	return res;
}

int ini_parse(link_t *dst, char *ini_path)
{
	u32 lblen;
	char lbuf[512];
	FIL fp;
	ini_sec_t *csec = NULL;

	if (f_open(&fp, ini_path, FA_READ) != FR_OK)
		return 0;

	do
	{
		//Fetch one line.
		lbuf[0] = 0;
		f_gets(lbuf, 512, &fp);
		lblen = strlen(lbuf);

		//Skip empty lines and comments.
		if (lblen <= 1 || lbuf[0] == '#')
			continue;

		//Remove trailing newline.
		if (lbuf[lblen - 1] == '\n')
			lbuf[lblen - 1] = 0;

		if (lblen > 2 && lbuf[0] == '[') //Create new section.
		{
			if (csec)
			{
				list_append(dst, &csec->link);
				csec = NULL;
			}

			u32 i;
			for (i = 0; i < lblen && lbuf[i] != '\n' && lbuf[i] != ']'; i++)
				;
			lbuf[i] = 0;

			csec = (ini_sec_t *)malloc(sizeof(ini_sec_t));
			csec->name = _strdup(&lbuf[1]);
			list_init(&csec->kvs);
		}
		else if (csec) //Extract key/value.
		{
			u32 i;
			for (i = 0; i < lblen && lbuf[i] != '\n' && lbuf[i] != '='; i++)
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

	return 1;
}
