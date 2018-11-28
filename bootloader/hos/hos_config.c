/*
 * Copyright (c) 2018 naehrwert
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

#include "hos.h"
#include "hos_config.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../utils/dirlist.h"

#include "../gfx/gfx.h"
extern gfx_con_t gfx_con;

//#define DPRINTF(...) gfx_printf(&gfx_con, __VA_ARGS__)
#define DPRINTF(...)

static int _config_warmboot(launch_ctxt_t *ctxt, const char *value)
{
	FIL fp;
	if (f_open(&fp, value, FA_READ) != FR_OK)
		return 0;
	ctxt->warmboot_size = f_size(&fp);
	ctxt->warmboot = malloc(ctxt->warmboot_size);
	f_read(&fp, ctxt->warmboot, ctxt->warmboot_size, NULL);
	f_close(&fp);
	return 1;
}

static int _config_secmon(launch_ctxt_t *ctxt, const char *value)
{
	FIL fp;
	if (f_open(&fp, value, FA_READ) != FR_OK)
		return 0;
	ctxt->secmon_size = f_size(&fp);
	ctxt->secmon = malloc(ctxt->secmon_size);
	f_read(&fp, ctxt->secmon, ctxt->secmon_size, NULL);
	f_close(&fp);
	return 1;
}

static int _config_kernel(launch_ctxt_t *ctxt, const char *value)
{
	FIL fp;
	if (f_open(&fp, value, FA_READ) != FR_OK)
		return 0;
	ctxt->kernel_size = f_size(&fp);
	ctxt->kernel = malloc(ctxt->kernel_size);
	f_read(&fp, ctxt->kernel, ctxt->kernel_size, NULL);
	f_close(&fp);
	return 1;
}

static int _config_kip1(launch_ctxt_t *ctxt, const char *value)
{
	FIL fp;

	if (!memcmp(value + strlen(value) - 1, "*", 1))
	{
		char *dir = (char *)malloc(256);
		memcpy(dir, value, strlen(value) + 1);

		u32 dirlen = 0;
		dir[strlen(dir) - 2] = 0;
		char *filelist = dirlist(dir, "*.kip*", false);

		memcpy(dir + strlen(dir), "/", 2);
		dirlen = strlen(dir);

		u32 i = 0;
		if (filelist)
		{
			while (true)
			{
				if (!filelist[i * 256])
					break;

				memcpy(dir + dirlen, &filelist[i * 256], strlen(&filelist[i * 256]) + 1);
				if (f_open(&fp, dir, FA_READ))
				{
					free(dir);
					free(filelist);

					return 0;
				}
				merge_kip_t *mkip1 = (merge_kip_t *)malloc(sizeof(merge_kip_t));
				mkip1->kip1 = malloc(f_size(&fp));
				f_read(&fp, mkip1->kip1, f_size(&fp), NULL);
				DPRINTF("Loaded kip1 from SD (size %08X)\n", f_size(&fp));
				f_close(&fp);
				list_append(&ctxt->kip1_list, &mkip1->link);

				i++;
			}
		}

		free(dir);
		free(filelist);
	}
	else
	{
		if (f_open(&fp, value, FA_READ))
			return 0;
		merge_kip_t *mkip1 = (merge_kip_t *)malloc(sizeof(merge_kip_t));
		mkip1->kip1 = malloc(f_size(&fp));
		f_read(&fp, mkip1->kip1, f_size(&fp), NULL);
		DPRINTF("Loaded kip1 from SD (size %08X)\n", f_size(&fp));
		f_close(&fp);
		list_append(&ctxt->kip1_list, &mkip1->link);
	}

	return 1;
}

static int _config_svcperm(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Disabled SVC verification\n");
		ctxt->svcperm = true;
	}
	return 1;
}

static int _config_debugmode(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Enabled Debug mode\n");
		ctxt->debugmode = true;
	}
	return 1;
}

static int _config_atmosphere(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Enabled atmosphere patching\n");
		ctxt->atmosphere = true;
	}
	return 1;
}

static int _config_kip1patch(launch_ctxt_t *ctxt, const char *value)
{
	if (value == NULL)
		return 0;

	int valueLen = strlen(value);
	if (!valueLen)
		return 0;

	if (ctxt->kip1_patches == NULL)
	{
		ctxt->kip1_patches = malloc(valueLen + 1);
		memcpy(ctxt->kip1_patches, value, valueLen);
		ctxt->kip1_patches[valueLen] = 0;
	}
	else
	{
		char *oldAlloc = ctxt->kip1_patches;
		int oldSize = strlen(oldAlloc);
		ctxt->kip1_patches = malloc(oldSize + 1 + valueLen + 1);
		memcpy(ctxt->kip1_patches, oldAlloc, oldSize);
		free(oldAlloc);
		oldAlloc = NULL;
		ctxt->kip1_patches[oldSize++] = ',';
		memcpy(&ctxt->kip1_patches[oldSize], value, valueLen);
		ctxt->kip1_patches[oldSize + valueLen] = 0;
	}
	return 1;
}

typedef struct _cfg_handler_t
{
	const char *key;
	int (*handler)(launch_ctxt_t *ctxt, const char *value);
} cfg_handler_t;

static const cfg_handler_t _config_handlers[] = {
	{ "warmboot", _config_warmboot },
	{ "secmon", _config_secmon },
	{ "kernel", _config_kernel },
	{ "kip1", _config_kip1 },
	{ "kip1patch", _config_kip1patch },
	{ "fullsvcperm", _config_svcperm },
	{ "debugmode", _config_debugmode },
	{ "atmosphere", _config_atmosphere },
	{ NULL, NULL },
};

int _parse_boot_config(launch_ctxt_t *ctxt, ini_sec_t *cfg)
{
	LIST_FOREACH_ENTRY(ini_kv_t, kv, &cfg->kvs, link)
	{
		for(u32 i = 0; _config_handlers[i].key; i++)
		{
			if (!strcmp(_config_handlers[i].key, kv->key))
				if (!_config_handlers[i].handler(ctxt, kv->val))
					return 0;
		}
	}

	return 1;
}

