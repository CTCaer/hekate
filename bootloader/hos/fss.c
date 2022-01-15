/*
 * Atmosphère Fusée Secondary Storage (Package3) parser.
 *
 * Copyright (c) 2019-2021 CTCaer
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

#include "fss.h"
#include "hos.h"
#include "../config.h"
#include <libs/fatfs/ff.h>
#include "../storage/emummc.h"

//#define DPRINTF(...) gfx_printf(__VA_ARGS__)
#define DPRINTF(...)

extern hekate_config h_cfg;

extern bool is_ipl_updated(void *buf, char *path, bool force);

// FSS0 Magic and Meta header offset.
#define FSS0_MAGIC 0x30535346
#define FSS0_META_OFFSET 0x4
#define FSS0_VERSION_0_17_0 0x110000

// FSS0 Content Types.
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

// FSS0 Content Flags.
#define CNT_FLAG0_EXPERIMENTAL BIT(0)

// FSS0 Meta Header.
typedef struct _fss_meta_t
{
	u32 magic;
	u32 size;
	u32 crt0_off;
	u32 cnt_off;
	u32 cnt_count;
	u32 hos_ver;
	u32 version;
	u32 git_rev;
} fss_meta_t;

// FSS0 Content Header.
typedef struct _fss_content_t
{
	u32 offset;
	u32 size;
	u8 type;
	u8 flags0;
	u8 flags1;
	u8 flags2;
	u32 rsvd1;
	char name[0x10];
} fss_content_t;

static void _set_fss_path_and_update_r2p(launch_ctxt_t *ctxt, const char *path)
{
	char *r2p_path = malloc(256);
	u32 path_len = strlen(path);

	strcpy(r2p_path, path);

	while(path_len)
	{
		if ((r2p_path[path_len - 1] == '/') || (r2p_path[path_len - 1] == '\\'))
		{
			r2p_path[path_len] = 0;
			strcat(r2p_path, "reboot_payload.bin");
			u8 *r2p_payload = sd_file_read(r2p_path, NULL);

			is_ipl_updated(r2p_payload, r2p_path, h_cfg.updater2p ? true : false);

			free(r2p_payload);

			// Save FSS0 parent path.
			r2p_path[path_len] = 0;
			ctxt->fss0_main_path = r2p_path;
			return;
		}
		path_len--;
	}

	free(r2p_path);
}

int parse_fss(launch_ctxt_t *ctxt, const char *path)
{
	FIL fp;

	bool stock = false;
	bool experimental = false;

	// Skip if stock and Exosphere and warmboot are not needed.
	bool pkg1_old = ctxt->pkg1_id->kb <= KB_FIRMWARE_VERSION_620; // Should check if t210b01?
	bool emummc_disabled = !emu_cfg.enabled || h_cfg.emummc_force_disable;

	LIST_FOREACH_ENTRY(ini_kv_t, kv, &ctxt->cfg->kvs, link)
	{
		if (!strcmp("stock", kv->key))
			if (kv->val[0] == '1')
				stock = true;

		if (!strcmp("fss0experimental", kv->key))
			if (kv->val[0] == '1')
				experimental = true;
	}

#ifdef HOS_MARIKO_STOCK_SECMON
	if (stock && emummc_disabled && (pkg1_old || h_cfg.t210b01))
		return 1;
#else
	if (stock && emummc_disabled && pkg1_old)
		return 1;
#endif

	// Try to open FSS0.
	if (f_open(&fp, path, FA_READ) != FR_OK)
		return 0;

	void *fss = malloc(f_size(&fp));

	// Read first 1024 bytes of the FSS0 file.
	f_read(&fp, fss, 1024, NULL);

	// Get FSS0 Meta header offset.
	u32 fss_meta_addr = *(u32 *)(fss + FSS0_META_OFFSET);
	fss_meta_t *fss_meta = (fss_meta_t *)(fss + fss_meta_addr);

	// Check if valid FSS0 and parse it.
	if (fss_meta->magic == FSS0_MAGIC)
	{
		gfx_printf("Found FSS/PK3, Atmosphere %d.%d.%d-%08x\n"
			"Max HOS: %d.%d.%d\n"
			"Unpacking..  ",
			fss_meta->version >> 24, (fss_meta->version >> 16) & 0xFF, (fss_meta->version >> 8) & 0xFF, fss_meta->git_rev,
			fss_meta->hos_ver >> 24, (fss_meta->hos_ver >> 16) & 0xFF, (fss_meta->hos_ver >> 8) & 0xFF);

		ctxt->atmosphere = true;
		ctxt->fss0_hosver = fss_meta->hos_ver;

		// Parse FSS0 contents.
		fss_content_t *curr_fss_cnt = (fss_content_t *)(fss + fss_meta->cnt_off);
		void *content;
		for (u32 i = 0; i < fss_meta->cnt_count; i++)
		{
			content = (void *)(fss + curr_fss_cnt[i].offset);

			// Check if offset is inside limits.
			if ((curr_fss_cnt[i].offset + curr_fss_cnt[i].size) > fss_meta->size)
				continue;

			// If content is experimental and experimental config is not enabled, skip it.
			if ((curr_fss_cnt[i].flags0 & CNT_FLAG0_EXPERIMENTAL) && !experimental)
				continue;

			// Prepare content.
			switch (curr_fss_cnt[i].type)
			{
			case CNT_TYPE_KIP:
				if (stock)
					continue;
				merge_kip_t *mkip1 = (merge_kip_t *)malloc(sizeof(merge_kip_t));
				mkip1->kip1 = content;
				list_append(&ctxt->kip1_list, &mkip1->link);
				DPRINTF("Loaded %s.kip1 from FSS0 (size %08X)\n", curr_fss_cnt[i].name, curr_fss_cnt[i].size);
				break;

			case CNT_TYPE_KRN:
				if (stock)
					continue;
				ctxt->kernel_size = curr_fss_cnt[i].size;
				ctxt->kernel = content;
				break;

			case CNT_TYPE_EXO:
				ctxt->secmon_size = curr_fss_cnt[i].size;
				ctxt->secmon = content;
				break;

			case CNT_TYPE_EXF:
				ctxt->exofatal_size = curr_fss_cnt[i].size;
				ctxt->exofatal = content;
				break;

			case CNT_TYPE_WBT:
				if (h_cfg.t210b01)
					continue;
				ctxt->warmboot_size = curr_fss_cnt[i].size;
				ctxt->warmboot = content;
				break;

			default:
				continue;
			}

			// Load content to launch context.
			f_lseek(&fp, curr_fss_cnt[i].offset);
			f_read(&fp, content, curr_fss_cnt[i].size, NULL);
		}

		gfx_printf("Done!\n");
		f_close(&fp);

		// Set FSS0 path and update r2p if needed.
		_set_fss_path_and_update_r2p(ctxt, path);

		return 1;
	}

	f_close(&fp);
	free(fss);

	return 0;
}
