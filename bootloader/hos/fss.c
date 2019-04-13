/*
 * Atmosphère Fusée Secondary Storage parser.
 *
 * Copyright (c) 2019 CTCaer
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

#include "fss.h"
#include "hos.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"

#include "../gfx/gfx.h"
#define DPRINTF(...)

#define FSS0_MAGIC 0x30535346
#define CNT_TYPE_FSP 0
#define CNT_TYPE_EXO 1
#define CNT_TYPE_WBT 2
#define CNT_TYPE_RBT 3
#define CNT_TYPE_SP1 4
#define CNT_TYPE_SP2 5
#define CNT_TYPE_KIP 6
#define CNT_TYPE_BMP 7

typedef struct _fss_t
{
	u32 magic;
	u32 size;
	u32 crt0_off;
	u32 cnt_off;
	u32 cnt_count;
	u32 hos_ver;
	u32 version;
	u32 git_rev;
} fss_t;

typedef struct _fss_content_t
{
	u32 offset;
	u32 size;
	u32 type;
	u32 rsvd1;
	char name[0x10];
} fss_content_t;

int parse_fss(launch_ctxt_t *ctxt, const char *value)
{
	FIL fp;

	bool stock = false;

	LIST_FOREACH_ENTRY(ini_kv_t, kv, &ctxt->cfg->kvs, link)
	{
		if (!strcmp("stock", kv->key))
			if (kv->val[0] == '1')
				stock = true;
	}

	if (stock && ctxt->pkg1_id->kb <= KB_FIRMWARE_VERSION_620)
		return 1;

	if (f_open(&fp, value, FA_READ) != FR_OK)
		return 0;

	ctxt->atmosphere = true;

	void *fss = malloc(f_size(&fp));
	// Read header.
	f_read(&fp, fss, 0x400, NULL);

	u32 fss_entry = *(u32 *)(fss + 4);
	fss_t *fss_meta = (fss_t *)(fss + fss_entry);

	if (fss_meta->magic == FSS0_MAGIC)
	{
		gfx_printf("Found FSS0, Atmosphere %d.%d.%d-%08x\n"
			"Max HOS supported: %d.%d.%d\n"
			"Unpacking and loading components..  ",
			fss_meta->version >> 24, (fss_meta->version >> 16) & 0xFF, (fss_meta->version >> 8) & 0xFF, fss_meta->git_rev,
			fss_meta->hos_ver >> 24, (fss_meta->hos_ver >> 16) & 0xFF, (fss_meta->hos_ver >> 8) & 0xFF);

		fss_content_t *curr_fss_cnt = (fss_content_t *)(fss + fss_meta->cnt_off);
		void *content;
		for (int i = 0; i < fss_meta->cnt_count; i++)
		{
			content = (void *)(fss + curr_fss_cnt[i].offset);
			if ((curr_fss_cnt[i].offset + curr_fss_cnt[i].size) > fss_meta->size)
				continue;

			// Load content to launch context.
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
			case CNT_TYPE_EXO:
				ctxt->secmon_size = curr_fss_cnt[i].size;
				ctxt->secmon = content;
				break;
			case CNT_TYPE_WBT:
				ctxt->warmboot_size = curr_fss_cnt[i].size;
				ctxt->warmboot = content;
				break;
			default:
				continue;
			// TODO: add more types?
			// case CNT_TYPE_SP1:
			// case CNT_TYPE_SP2:
			}

			f_lseek(&fp, curr_fss_cnt[i].offset);
			f_read(&fp, content, curr_fss_cnt[i].size, NULL);
		}

		gfx_printf("Done!\n");
		f_close(&fp);

		return 1;
	}

	f_close(&fp);
	free(fss);

	return 0;
}