/*
 * Atmosphère Package 3 parser.
 *
 * Copyright (c) 2019-2025 CTCaer
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

#include "pkg3.h"
#include "hos.h"
#include "../config.h"
#include <libs/fatfs/ff.h>
#include "../storage/emummc.h"

//#define DPRINTF(...) gfx_printf(__VA_ARGS__)
#define DPRINTF(...)

extern bool is_ipl_updated(void *buf, const char *path, bool force);

#define PKG3_KIP_SKIP_MAX 16

// PKG3 Magic and Meta header offset.
#define PKG3_MAGIC 0x30535346 // FSS0.
#define PKG3_META_OFFSET 0x4
#define PKG3_VERSION_0_17_0 0x110000

// PKG3 Content Types.
#define CNT_TYPE_FSP 0
#define CNT_TYPE_EXO 1  // Exosphere (Secure Monitor).
#define CNT_TYPE_WBT 2  // Warmboot (SC7Exit fw).
#define CNT_TYPE_RBT 3  // Rebootstub (Warmboot based reboot fw).
#define CNT_TYPE_SP1 4  // Sept Primary (TSEC and Sept Secondary loader).
#define CNT_TYPE_SP2 5  // Sept Secondary (Acts as pkg11 and derives keys).
#define CNT_TYPE_KIP 6  // KIP1 (Used for replacement or addition).
#define CNT_TYPE_BMP 7
#define CNT_TYPE_EMC 8
#define CNT_TYPE_KLD 9  // Kernel Loader.
#define CNT_TYPE_KRN 10 // Kernel.
#define CNT_TYPE_EXF 11 // Exosphere Mariko fatal payload.
#define CNT_TYPE_TKG 12 // Tsec Keygen.

// PKG3 Content Flags.
#define CNT_FLAG0_EXPERIMENTAL BIT(0)

// PKG3 Meta Header.
typedef struct _pkg3_meta_t
{
	u32 magic;
	u32 size;
	u32 crt0_off;
	u32 cnt_off;
	u32 cnt_count;
	u32 hos_ver;
	u32 version;
	u32 git_rev;
} pkg3_meta_t;

// PKG3 Content Header.
typedef struct _pkg3_content_t
{
	u32 offset;
	u32 size;
	u8 type;
	u8 flags0;
	u8 flags1;
	u8 flags2;
	u32 rsvd1;
	char name[0x10];
} pkg3_content_t;

static void _pkg3_update_r2p()
{
	u8 *r2p_payload = sd_file_read("atmosphere/reboot_payload.bin", NULL);

	is_ipl_updated(r2p_payload, "atmosphere/reboot_payload.bin", h_cfg.updater2p ? true : false);

	free(r2p_payload);
}

static int _pkg3_kip1_skip(char ***pkg3_kip1_skip, u32 *pkg3_kip1_skip_num, char *value)
{
	int len = strlen(value);
	if (!len || (*pkg3_kip1_skip_num) >= PKG3_KIP_SKIP_MAX)
		return 0;

	// Allocate pointer list memory.
	if (!(*pkg3_kip1_skip))
		(*pkg3_kip1_skip) = calloc(PKG3_KIP_SKIP_MAX, sizeof(char *));

	// Set first kip name.
	(*pkg3_kip1_skip)[(*pkg3_kip1_skip_num)++] = value;

	// Check if more names are set separated by comma.
	for (char *c = value; *c != 0; c++)
	{
		if (*c == ',')
		{
			*c = 0; // Null termination.

			// Set next kip name to the list.
			(*pkg3_kip1_skip)[(*pkg3_kip1_skip_num)++] = c + 1;

			if ((*pkg3_kip1_skip_num) >= PKG3_KIP_SKIP_MAX)
				return 0;
		}
	}

	return 1;
}

int parse_pkg3(launch_ctxt_t *ctxt, const char *path)
{
	FIL fp;

	char **pkg3_kip1_skip = NULL;
	u32    pkg3_kip1_skip_num = 0;

	bool stock = false;
	bool experimental = false;

	// Skip if stock and Exosphere and warmboot are not needed.
	bool pkg1_old = ctxt->pkg1_id->mkey <= HOS_MKEY_VER_620; // Should check if t210b01?
	bool emummc_disabled = !emu_cfg.enabled || h_cfg.emummc_force_disable;

	LIST_FOREACH_ENTRY(ini_kv_t, kv, &ctxt->cfg->kvs, link)
	{
		if (!strcmp("stock", kv->key))
			if (kv->val[0] == '1')
				stock = true;

		if (!strcmp("pkg3ex", kv->key))
			if (kv->val[0] == '1')
				experimental = true;

		if (!strcmp("pkg3kip1skip", kv->key))
			_pkg3_kip1_skip(&pkg3_kip1_skip, &pkg3_kip1_skip_num, kv->val);
	}

#ifdef HOS_MARIKO_STOCK_SECMON
	if (stock && emummc_disabled && (pkg1_old || h_cfg.t210b01))
		return 1;
#else
	if (stock && emummc_disabled && pkg1_old)
		return 1;
#endif

	// Try to open PKG3.
	if (f_open(&fp, path, FA_READ) != FR_OK)
		return 0;

	void *pkg3 = malloc(f_size(&fp));

	// Read first 1024 bytes of the PKG3 file.
	f_read(&fp, pkg3, 1024, NULL);

	// Get PKG3 Meta header offset.
	u32 pkg3_meta_addr = *(u32 *)(pkg3 + PKG3_META_OFFSET);
	pkg3_meta_t *pkg3_meta = (pkg3_meta_t *)(pkg3 + pkg3_meta_addr);

	// Check if valid PKG3 and parse it.
	if (pkg3_meta->magic == PKG3_MAGIC)
	{
		gfx_printf("Atmosphere %d.%d.%d-%08x via PKG3\n"
			"Max HOS: %d.%d.%d\n"
			"Unpacking..  ",
			pkg3_meta->version >> 24, (pkg3_meta->version >> 16) & 0xFF, (pkg3_meta->version >> 8) & 0xFF, pkg3_meta->git_rev,
			pkg3_meta->hos_ver >> 24, (pkg3_meta->hos_ver >> 16) & 0xFF, (pkg3_meta->hos_ver >> 8) & 0xFF);

		ctxt->patch_krn_proc_id = true;
		ctxt->pkg3_hosver = pkg3_meta->hos_ver;

		// Parse PKG3 contents.
		pkg3_content_t *curr_pkg3_cnt = (pkg3_content_t *)(pkg3 + pkg3_meta->cnt_off);
		void *content;
		for (u32 i = 0; i < pkg3_meta->cnt_count; i++)
		{
			content = (void *)(pkg3 + curr_pkg3_cnt[i].offset);

			// Check if offset is inside limits.
			if ((curr_pkg3_cnt[i].offset + curr_pkg3_cnt[i].size) > pkg3_meta->size)
				continue;

			// If content is experimental and experimental config is not enabled, skip it.
			if ((curr_pkg3_cnt[i].flags0 & CNT_FLAG0_EXPERIMENTAL) && !experimental)
				continue;

			// Prepare content.
			switch (curr_pkg3_cnt[i].type)
			{
			case CNT_TYPE_KIP:
				if (stock)
					continue;

				bool should_skip = false;
				for (u32 k = 0; k < pkg3_kip1_skip_num; k++)
				{
					if (!strcmp(curr_pkg3_cnt[i].name, pkg3_kip1_skip[k]))
					{
						gfx_printf("Skipped %s.kip1 from PKG3\n", curr_pkg3_cnt[i].name);
						should_skip = true;
						break;
					}
				}
				if (should_skip)
					continue;

				merge_kip_t *mkip1 = (merge_kip_t *)malloc(sizeof(merge_kip_t));
				mkip1->kip1 = content;
				list_append(&ctxt->kip1_list, &mkip1->link);
				DPRINTF("Loaded %s.kip1 from PKG3 (size %08X)\n", curr_pkg3_cnt[i].name, curr_pkg3_cnt[i].size);
				break;

			case CNT_TYPE_KRN:
				if (stock)
					continue;
				ctxt->kernel_size = curr_pkg3_cnt[i].size;
				ctxt->kernel = content;
				break;

			case CNT_TYPE_EXO:
				ctxt->secmon_size = curr_pkg3_cnt[i].size;
				ctxt->secmon = content;
				break;

			case CNT_TYPE_EXF:
				ctxt->exofatal_size = curr_pkg3_cnt[i].size;
				ctxt->exofatal = content;
				break;

			case CNT_TYPE_WBT:
				if (h_cfg.t210b01)
					continue;
				ctxt->warmboot_size = curr_pkg3_cnt[i].size;
				ctxt->warmboot = content;
				break;

			default:
				continue;
			}

			// Load content to launch context.
			f_lseek(&fp, curr_pkg3_cnt[i].offset);
			f_read(&fp, content, curr_pkg3_cnt[i].size, NULL);
		}

		gfx_printf("Done!\n");
		f_close(&fp);

		ctxt->pkg3 = pkg3;

		// Update r2p if needed.
		_pkg3_update_r2p();

		free(pkg3_kip1_skip);

		return 1;
	}

	// Failed. Close and free all.
	f_close(&fp);

	free(pkg3_kip1_skip);
	free(pkg3);

	return 0;
}
