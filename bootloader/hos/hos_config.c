/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 CTCaer
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
#include "fss.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../utils/dirlist.h"

#include "../gfx/gfx.h"

//#define DPRINTF(...) gfx_printf(__VA_ARGS__)
#define DPRINTF(...)

extern void *sd_file_read(const char *path, u32 *fsize);

static int _config_warmboot(launch_ctxt_t *ctxt, const char *value)
{
	ctxt->warmboot = sd_file_read(value, &ctxt->warmboot_size);
	if (!ctxt->warmboot)
		return 0;

	return 1;
}

static int _config_secmon(launch_ctxt_t *ctxt, const char *value)
{
	ctxt->secmon = sd_file_read(value, &ctxt->secmon_size);
	if (!ctxt->secmon)
		return 0;

	return 1;
}

static int _config_kernel(launch_ctxt_t *ctxt, const char *value)
{
	ctxt->kernel = sd_file_read(value, &ctxt->kernel_size);
	if (!ctxt->kernel)
		return 0;

	return 1;
}

static int _config_kip1(launch_ctxt_t *ctxt, const char *value)
{
	u32 size;

	if (!memcmp(value + strlen(value) - 1, "*", 1))
	{
		char *dir = (char *)malloc(256);
		strcpy(dir, value);

		u32 dirlen = 0;
		dir[strlen(dir) - 2] = 0;
		char *filelist = dirlist(dir, "*.kip*", false);

		strcat(dir, "/");
		dirlen = strlen(dir);

		u32 i = 0;
		if (filelist)
		{
			while (true)
			{
				if (!filelist[i * 256])
					break;

				strcpy(dir + dirlen, &filelist[i * 256]);

				merge_kip_t *mkip1 = (merge_kip_t *)malloc(sizeof(merge_kip_t));
				mkip1->kip1 = sd_file_read(dir, &size);
				if (!mkip1->kip1)
				{
					free(mkip1);
					free(dir);
					free(filelist);

					return 0;
				}
				DPRINTF("Loaded kip1 from SD (size %08X)\n", size);
				list_append(&ctxt->kip1_list, &mkip1->link);

				i++;
			}
		}

		free(dir);
		free(filelist);
	}
	else
	{
		merge_kip_t *mkip1 = (merge_kip_t *)malloc(sizeof(merge_kip_t));
		mkip1->kip1 = sd_file_read(value, &size);
		if (!mkip1->kip1)
		{
			free(mkip1);

			return 0;
		}
		DPRINTF("Loaded kip1 from SD (size %08X)\n", size);
		list_append(&ctxt->kip1_list, &mkip1->link);
	}

	return 1;
}

int config_kip1patch(launch_ctxt_t *ctxt, const char *value)
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

static int _config_stock(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Disabled all patching\n");
		ctxt->stock = true;
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

static int _config_dis_exo_user_exceptions(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Disabled exosphere user exception handlers\n");
		ctxt->exo_no_user_exceptions = true;
	}
	return 1;
}

static int _config_exo_user_pmu_access(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Enabled user access to PMU\n");
		ctxt->exo_user_pmu = true;
	}
	return 1;
}

static int _config_fss(launch_ctxt_t *ctxt, const char *value)
{
	return parse_fss(ctxt, value);
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
	{ "kip1patch", config_kip1patch },
	{ "fullsvcperm", _config_svcperm },
	{ "debugmode", _config_debugmode },
	{ "stock", _config_stock },
	{ "atmosphere", _config_atmosphere },
	{ "nouserexceptions", _config_dis_exo_user_exceptions },
	{ "userpmu", _config_exo_user_pmu_access },
	{ "fss0", _config_fss },
	{ NULL, NULL },
};

int parse_boot_config(launch_ctxt_t *ctxt)
{
	LIST_FOREACH_ENTRY(ini_kv_t, kv, &ctxt->cfg->kvs, link)
	{
		for(u32 i = 0; _config_handlers[i].key; i++)
		{
			if (!strcmp(_config_handlers[i].key, kv->key))
				if (!_config_handlers[i].handler(ctxt, kv->val))
				{
					EPRINTFARGS("Error while loading %s:\n%s", kv->key, kv->val);
					return 0;
				}
		}
	}

	return 1;
}
