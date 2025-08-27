/*
 * Copyright (c) 2018 naehrwert
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

#include <bdk.h>

#include "hos.h"
#include "hos_config.h"
#include "pkg3.h"
#include <libs/fatfs/ff.h>

//#define DPRINTF(...) gfx_printf(__VA_ARGS__)
#define DPRINTF(...)

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

	if (value[strlen(value) - 1] == '*')
	{
		char *dir = (char *)malloc(256);
		strcpy(dir, value);

		u32 dirlen = 0;
		dir[strlen(dir) - 2] = 0;
		dirlist_t *filelist = dirlist(dir, "*.kip*", 0);

		strcat(dir, "/");
		dirlen = strlen(dir);

		u32 i = 0;
		if (filelist)
		{
			while (true)
			{
				if (!filelist->name[i])
					break;

				strcpy(dir + dirlen, filelist->name[i]);

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
	int len = strlen(value);
	if (!len)
		return 0;

	if (ctxt->kip1_patches == NULL)
	{
		ctxt->kip1_patches = malloc(len + 1);
		memcpy(ctxt->kip1_patches, value, len);
		ctxt->kip1_patches[len] = 0;
	}
	else
	{
		char *old_addr = ctxt->kip1_patches;
		int old_len = strlen(old_addr);

		ctxt->kip1_patches = malloc(old_len + 1 + len + 1);
		memcpy(ctxt->kip1_patches, old_addr, old_len);
		free(old_addr);

		ctxt->kip1_patches[old_len++] = ',';
		memcpy(&ctxt->kip1_patches[old_len], value, len);
		ctxt->kip1_patches[old_len + len] = 0;
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
		DPRINTF("Enabled stock mode\n");
		ctxt->stock = true;
	}
	return 1;
}

static int _config_emummc_forced(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Forced emuMMC\n");
		ctxt->emummc_forced = true;
	}
	return 1;
}

static int _config_kernel_proc_id(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Enabled kernel process id send/recv patching\n");
		ctxt->patch_krn_proc_id = true;
	}
	return 1;
}

static int _config_dis_exo_user_exceptions(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Disabled exosphere user exception handlers\n");
		ctxt->exo_ctx.no_user_exceptions = true;
	}
	return 1;
}

static int _config_exo_user_pmu_access(launch_ctxt_t *ctxt, const char *value)
{
	if (*value == '1')
	{
		DPRINTF("Enabled user access to PMU\n");
		ctxt->exo_ctx.user_pmu = true;
	}
	return 1;
}

static int _config_exo_usb3_force(launch_ctxt_t *ctxt, const char *value)
{
	// Override key found.
	ctxt->exo_ctx.usb3_force = zalloc(sizeof(bool));

	if (*value == '1')
	{
		DPRINTF("Enabled USB 3.0\n");
		*ctxt->exo_ctx.usb3_force = true;
	}
	return 1;
}

static int _config_exo_cal0_blanking(launch_ctxt_t *ctxt, const char *value)
{
	// Override key found.
	ctxt->exo_ctx.cal0_blank = zalloc(sizeof(bool));

	if (*value == '1')
	{
		DPRINTF("Enabled prodinfo blanking\n");
		*ctxt->exo_ctx.cal0_blank = true;
	}
	return 1;
}

static int _config_exo_cal0_writes_enable(launch_ctxt_t *ctxt, const char *value)
{
	// Override key found.
	ctxt->exo_ctx.cal0_allow_writes_sys = zalloc(sizeof(bool));

	if (*value == '1')
	{
		DPRINTF("Enabled prodinfo writes\n");
		*ctxt->exo_ctx.cal0_allow_writes_sys = true;
	}

	return 1;
}

static int _config_pkg3(launch_ctxt_t *ctxt, const char *value)
{
	return parse_pkg3(ctxt, value);
}

static int _config_exo_fatal_payload(launch_ctxt_t *ctxt, const char *value)
{
	ctxt->exofatal = sd_file_read(value, &ctxt->exofatal_size);
	if (!ctxt->exofatal)
		return 0;

	return 1;
}

static int _config_ucid(launch_ctxt_t *ctxt, const char *value)
{
	// Override uCID if set.
	ctxt->ucid = atoi(value);

	return 1;
}

typedef struct _cfg_handler_t
{
	const char *key;
	int (*handler)(launch_ctxt_t *ctxt, const char *value);
} cfg_handler_t;

static const cfg_handler_t _config_handlers[] = {
	{ "stock",            _config_stock },
	{ "warmboot",         _config_warmboot },
	{ "secmon",           _config_secmon },
	{ "kernel",           _config_kernel },
	{ "kip1",             _config_kip1 },
	{ "kip1patch",        config_kip1patch },
	{ "fullsvcperm",      _config_svcperm },
	{ "debugmode",        _config_debugmode },
	{ "kernelprocid",     _config_kernel_proc_id },

	// To override elements from PKG3, it should be set before others.
	{ "pkg3",             _config_pkg3 },
	{ "fss0",             _config_pkg3 },

	{ "exofatal",         _config_exo_fatal_payload},
	{ "emummcforce",      _config_emummc_forced },
	{ "nouserexceptions", _config_dis_exo_user_exceptions },
	{ "userpmu",          _config_exo_user_pmu_access },
	{ "usb3force",        _config_exo_usb3_force },
	{ "cal0blank",        _config_exo_cal0_blanking },
	{ "cal0writesys",     _config_exo_cal0_writes_enable },
	{ "ucid",             _config_ucid },
	{ NULL, NULL },
};

int parse_boot_config(launch_ctxt_t *ctxt)
{
	if (!ctxt->cfg)
		return 1;

	// Check each config key.
	LIST_FOREACH_ENTRY(ini_kv_t, kv, &ctxt->cfg->kvs, link)
	{
		for (u32 i = 0; _config_handlers[i].key; i++)
		{
			// If key matches, call its handler.
			if (!strcmp(_config_handlers[i].key, kv->key))
			{
				if (!_config_handlers[i].handler(ctxt, kv->val))
				{
					gfx_con.mute = false;
					EPRINTFARGS("Error while loading %s:\n%s", kv->key, kv->val);

					return 0;
				}

				break;
			}
		}
	}

	return 1;
}
