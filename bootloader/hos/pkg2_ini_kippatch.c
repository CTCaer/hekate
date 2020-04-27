#include <string.h>
#include <stdlib.h>

#include "pkg2_ini_kippatch.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"

#define KPS(x) ((u32)(x) << 29)

static u8 *_htoa(u8 *result, const char *ptr, u8 byte_len)
{
	char ch = *ptr;
	u32 ascii_len = byte_len * 2;
	if (!result)
		result = malloc(byte_len);
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

static char *_strdup(char *str)
{
	if (!str)
		return NULL;
	if (str[0] == ' ' && (strlen(str) > 1))
		str++;
	char *res = (char *)malloc(strlen(str) + 1);
	strcpy(res, str);
	if (res[strlen(res) - 1] == ' ' && (strlen(res) > 1))
		res[strlen(res) - 1] = 0;

	return res;
}

static u32 _find_patch_section_name(char *lbuf, u32 lblen, char schar)
{
	u32 i;
	for (i = 0; i < lblen  && lbuf[i] != schar && lbuf[i] != '\n' && lbuf[i] != '\r'; i++)
		;
	lbuf[i] = 0;

	return i;
}

static ini_kip_sec_t *_ini_create_kip_section(link_t *dst, ini_kip_sec_t *ksec, char *name)
{
	if (ksec)
		list_append(dst, &ksec->link);

	ksec = (ini_kip_sec_t *)calloc(sizeof(ini_kip_sec_t), 1);
	u32 i = _find_patch_section_name(name, strlen(name), ':') + 1;
	ksec->name = _strdup(name);

	// Get hash section.
	_htoa(ksec->hash, &name[i], 8);

	return ksec;
}

int ini_patch_parse(link_t *dst, char *ini_path)
{
	u32 lblen;
	char lbuf[512];
	FIL fp;
	ini_kip_sec_t *ksec = NULL;

	// Open ini.
	if (f_open(&fp, ini_path, FA_READ) != FR_OK)
		return 0;

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

			ksec = _ini_create_kip_section(dst, ksec, &lbuf[1]);
			list_init(&ksec->pts);
		}
		else if (ksec && lbuf[0] == '.') //Extract key/value.
		{
			u32 tmp = 0;
			u32 i = _find_patch_section_name(lbuf, lblen, '=');

			ini_patchset_t *pt = (ini_patchset_t *)calloc(sizeof(ini_patchset_t), 1);

			pt->name = _strdup(&lbuf[1]);

			u8 kip_sidx = lbuf[i + 1] - '0';

			if (kip_sidx < 6)
			{
				pt->offset = KPS(kip_sidx);
				tmp = _find_patch_section_name(&lbuf[i + 3], lblen, ':');
				pt->offset |= strtol(&lbuf[i + 3], NULL, 16);

				i += tmp + 4;

				tmp = _find_patch_section_name(&lbuf[i], lblen, ':');
				pt->length = strtol(&lbuf[i], NULL, 16);

				i += tmp + 1;

				tmp = _find_patch_section_name(&lbuf[i], lblen, ',');
				pt->srcData = _htoa(NULL, &lbuf[i], pt->length);
				i += tmp + 1;
				pt->dstData = _htoa(NULL, &lbuf[i], pt->length);
			}

			list_append(&ksec->pts, &pt->link);
		}
	} while (!f_eof(&fp));

	f_close(&fp);

	if (ksec)
		list_append(dst, &ksec->link);

	return 1;
}
