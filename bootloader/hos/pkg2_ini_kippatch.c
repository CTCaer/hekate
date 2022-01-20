/*
 * Copyright (c) 2019-2022 CTCaer
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

#include <bdk.h>

#include "pkg2_ini_kippatch.h"
#include <libs/fatfs/ff.h>

#define KPS(x) ((u32)(x) << 29)

static u8 *_htoa(u8 *result, const char *ptr, u8 byte_len, u8 *buf)
{
	char ch = *ptr;
	u32 ascii_len = byte_len * 2;
	if (!result)
		result = buf;
	u8 *dst = result;

	while (ch == ' ' || ch == '\t')
		ch = *(++ptr);

	bool shift = true;
	while (ascii_len)
	{
		u8 tmp = 0;
		if (ch >= '0' && ch <= '9')
			tmp = (ch - '0');
		else if (ch >= 'A' && ch <= 'F')
			tmp = (ch - 'A' + 10);
		else if (ch >= 'a' && ch <= 'f')
			tmp = (ch - 'a' + 10);

		if (shift)
			*dst = (tmp << 4) & 0xF0;
		else
		{
			*dst |= (tmp & 0x0F);
			dst++;
		}

		ascii_len--;
		ch = *(++ptr);
		shift = !shift;
	}

	return result;
}

static u32 _find_patch_section_name(char *lbuf, u32 lblen, char schar)
{
	u32 i;
	// Depends on 'FF_USE_STRFUNC 2' that removes \r.
	for (i = 0; i < lblen  && lbuf[i] != schar && lbuf[i] != '\n'; i++)
		;
	lbuf[i] = 0;

	return i;
}

static ini_kip_sec_t *_ini_create_kip_section(link_t *dst, ini_kip_sec_t *ksec, char *name)
{
	if (ksec)
		list_append(dst, &ksec->link);

	// Calculate total allocation size.
	u32 len = strlen(name);
	char *buf = calloc(sizeof(ini_kip_sec_t) + len + 1, 1);

	ksec = (ini_kip_sec_t *)buf;
	u32 i = _find_patch_section_name(name, len, ':') + 1;
	ksec->name = strcpy_ns(buf + sizeof(ini_kip_sec_t), name);

	// Get hash section.
	_htoa(ksec->hash, &name[i], 8, NULL);

	return ksec;
}

int ini_patch_parse(link_t *dst, char *ini_path)
{
	FIL fp;
	u32 lblen;
	char *lbuf;
	ini_kip_sec_t *ksec = NULL;

	// Open ini.
	if (f_open(&fp, ini_path, FA_READ) != FR_OK)
		return 0;

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
			_find_patch_section_name(lbuf, lblen, ']');

			// Set patchset kip name and hash.
			ksec = _ini_create_kip_section(dst, ksec, &lbuf[1]);
			list_init(&ksec->pts);
		}
		else if (ksec && lbuf[0] == '.') // Extract key/value.
		{
			u32 str_start = 0;
			u32 pos = _find_patch_section_name(lbuf, lblen, '=');

			// Calculate total allocation size.
			char *buf = calloc(sizeof(ini_patchset_t) + strlen(&lbuf[1]) + 1, 1);
			ini_patchset_t *pt = (ini_patchset_t *)buf;

			// Set patch name.
			pt->name = strcpy_ns(buf + sizeof(ini_patchset_t), &lbuf[1]);

			u8 kip_sidx = lbuf[pos + 1] - '0';
			pos += 3;

			if (kip_sidx < 6)
			{
				// Set patch offset.
				pt->offset = KPS(kip_sidx);
				str_start = _find_patch_section_name(&lbuf[pos], lblen - pos, ':');
				pt->offset |= strtol(&lbuf[pos], NULL, 16);
				pos += str_start + 1;

				// Set patch size.
				str_start = _find_patch_section_name(&lbuf[pos], lblen - pos, ':');
				pt->length = strtol(&lbuf[pos], NULL, 16);
				pos += str_start + 1;

				u8 *buf = malloc(pt->length * 2);

				// Set patch source data.
				str_start = _find_patch_section_name(&lbuf[pos], lblen - pos, ',');
				pt->srcData = _htoa(NULL, &lbuf[pos], pt->length, buf);
				pos += str_start + 1;

				// Set patch destination data.
				pt->dstData = _htoa(NULL, &lbuf[pos], pt->length, buf + pt->length);
			}

			list_append(&ksec->pts, &pt->link);
		}
	} while (!f_eof(&fp));

	f_close(&fp);

	if (ksec)
		list_append(dst, &ksec->link);

	free(lbuf);

	return 1;
}
