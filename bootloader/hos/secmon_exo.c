/*
 * Copyright (c) 2018-2020 CTCaer
 * Copyright (c) 2019 Atmosphère-NX
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

#include "hos.h"
#include "../config/config.h"
#include "../gfx/di.h"
#include "../gfx/gfx.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../soc/fuse.h"
#include "../storage/emummc.h"
#include "../storage/nx_sd.h"
#include "../storage/sdmmc.h"
#include "../utils/btn.h"
#include "../utils/util.h"
#include "../utils/types.h"

extern hekate_config h_cfg;

enum emuMMC_Type
{
	emuMMC_None = 0,
	emuMMC_Partition,
	emuMMC_File,
	emuMMC_MAX
};

/* "EFS0" */
#define EMUMMC_MAGIC 0x30534645
#define EMUMMC_FILE_PATH_MAX 0x80

typedef struct
{
	u32 magic;
	u32 type;
	u32 id;
	u32 fs_ver;
} emummc_base_config_t;

typedef struct
{
	u64 start_sector;
} emummc_partition_config_t;

typedef struct
{
	char path[EMUMMC_FILE_PATH_MAX];
} emummc_file_config_t;

typedef struct
{
	emummc_base_config_t base_cfg;
	union
	{
		emummc_partition_config_t partition_cfg;
		emummc_file_config_t file_cfg;
	};
	char nintendo_path[EMUMMC_FILE_PATH_MAX];
} exo_emummc_config_t;

typedef struct _exo_cfg_t
{
	u32 magic;
	u32 fwno;
	u32 flags;
	u32 reserved[5];
	exo_emummc_config_t emummc_cfg;
} exo_cfg_t;

typedef struct _atm_meta_t
{
	u32 magic;
	u32 fwno;
} wb_cfg_t;

// Atmosphère reboot-to-fatal-error.
typedef struct _atm_fatal_error_ctx
{
	u32 magic;
	u32 error_desc;
	u64 title_id;
	union
	{
		u64 gprs[32];
		struct
		{
			u64 _gprs[29];
			u64 fp;
			u64 lr;
			u64 sp;
		};
	};
	u64 pc;
	u64 module_base;
	u32 pstate;
	u32 afsr0;
	u32 afsr1;
	u32 esr;
	u64 far;
	u64 report_identifier; // Normally just system tick.
	u64 stack_trace_size;
	u64 stack_dump_size;
	u64 stack_trace[0x20];
	u8  stack_dump[0x100];
	u8  tls[0x100];
} atm_fatal_error_ctx;

#define ATM_FATAL_ERR_CTX_ADDR 0x4003E000
#define  ATM_FATAL_MAGIC 0x30454641 // AFE0

#define ATM_WB_HEADER_OFF 0x244
#define  ATM_WB_MAGIC 0x30544257

// Exosphère mailbox defines.
#define EXO_CFG_ADDR      0x8000F000
#define  EXO_MAGIC_VAL           0x304F5845
#define  EXO_FLAG_DBG_PRIV        (1 << 1)
#define  EXO_FLAG_DBG_USER        (1 << 2)
#define  EXO_FLAG_NO_USER_EXC     (1 << 3)
#define  EXO_FLAG_USER_PMU        (1 << 4)
#define  EXO_FLAG_CAL0_BLANKING   (1 << 5)
#define  EXO_FLAG_CAL0_WRITES_SYS (1 << 6)

void config_exosphere(launch_ctxt_t *ctxt)
{
	u32 exoFwNo = 0;
	u32 exoFlags = 0;
	u32 kb = ctxt->pkg1_id->kb;
	bool user_debug = false;
	bool cal0_blanking = false;
	bool cal0_allow_writes_sys = false;

	memset((exo_cfg_t *)EXO_CFG_ADDR, 0, sizeof(exo_cfg_t));

	volatile exo_cfg_t *exo_cfg = (exo_cfg_t *)EXO_CFG_ADDR;

	switch (kb)
	{
	case KB_FIRMWARE_VERSION_100_200:
		if (!strcmp(ctxt->pkg1_id->id, "20161121183008"))
			exoFwNo = 1;
		else
			exoFwNo = 2;
		break;
	case KB_FIRMWARE_VERSION_300:
		exoFwNo = 3;
		break;
	default:
		exoFwNo = kb + 1;
		if (!strcmp(ctxt->pkg1_id->id, "20190314172056") || (kb >= KB_FIRMWARE_VERSION_810))
			exoFwNo++; // ATM_TARGET_FW_800/810/900/910.
		if (!strcmp(ctxt->pkg1_id->id, "20200303104606"))
			exoFwNo++; // ATM_TARGET_FW_1000.
		break;
	}

	if (!ctxt->stock)
	{
		// Parse exosphere.ini.
		LIST_INIT(ini_sections);
		if (ini_parse(&ini_sections, "exosphere.ini", false))
		{
			LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
			{
				// Only parse exosphere section.
				if (!(ini_sec->type == INI_CHOICE) || strcmp(ini_sec->name, "exosphere"))
					continue;

				LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
				{
					if (!strcmp("debugmode_user", kv->key))
						user_debug = atoi(kv->val);
					else if (emu_cfg.enabled && !h_cfg.emummc_force_disable)
					{
						if (!strcmp("blank_prodinfo_emummc", kv->key))
							cal0_blanking = atoi(kv->val);
					}
					else
					{
						if (!strcmp("blank_prodinfo_sysmmc", kv->key))
							cal0_blanking = atoi(kv->val);
						else if (!strcmp("allow_writing_to_cal_sysmmc", kv->key))
							cal0_allow_writes_sys = atoi(kv->val);
					}
				}
				break;
			}
		}
	}

	// To avoid problems, make private debug mode always on if not semi-stock.
	if (!ctxt->stock || (emu_cfg.enabled && !h_cfg.emummc_force_disable))
		exoFlags |= EXO_FLAG_DBG_PRIV;

	// Enable user debug.
	if (user_debug)
		exoFlags |= EXO_FLAG_DBG_USER;

	// Disable proper failure handling.
	if (ctxt->exo_cfg.no_user_exceptions)
		exoFlags |= EXO_FLAG_NO_USER_EXC;

	// Enable user access to PMU.
	if (ctxt->exo_cfg.user_pmu)
		exoFlags |= EXO_FLAG_USER_PMU;

	// Check if exo ini value is overridden and enable prodinfo blanking.
	if ((ctxt->exo_cfg.cal0_blank && *ctxt->exo_cfg.cal0_blank)
			|| (!ctxt->exo_cfg.cal0_blank && cal0_blanking))
		exoFlags |= EXO_FLAG_CAL0_BLANKING;

	// Check if exo ini value is overridden and allow prodinfo writes.
	if ((ctxt->exo_cfg.cal0_allow_writes_sys && *ctxt->exo_cfg.cal0_allow_writes_sys)
			|| (!ctxt->exo_cfg.cal0_allow_writes_sys && cal0_allow_writes_sys))
		exoFlags |= EXO_FLAG_CAL0_WRITES_SYS;

	// Set mailbox values.
	exo_cfg->magic = EXO_MAGIC_VAL;
	exo_cfg->fwno = exoFwNo;
	exo_cfg->flags = exoFlags;

	// If warmboot is lp0fw, add in RSA modulus.
	volatile wb_cfg_t *wb_cfg = (wb_cfg_t *)(ctxt->pkg1_id->warmboot_base + ATM_WB_HEADER_OFF);

	if (wb_cfg->magic == ATM_WB_MAGIC)
	{
		wb_cfg->fwno = exoFwNo;

		sdmmc_storage_t storage;
		sdmmc_t sdmmc;

		// Set warmboot binary rsa modulus.
		u8 *rsa_mod = (u8 *)malloc(512);

		sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_BUS_WIDTH_8, SDHCI_TIMING_MMC_HS400);
		sdmmc_storage_set_mmc_partition(&storage, EMMC_BOOT0);
		sdmmc_storage_read(&storage, 1, 1, rsa_mod);
		sdmmc_storage_end(&storage);

		// Patch AutoRCM out.
		if ((fuse_read_odm(4) & 3) != 3)
			rsa_mod[0x10] = 0xF7;
		else
			rsa_mod[0x10] = 0x37;

		memcpy((void *)(ctxt->pkg1_id->warmboot_base + 0x10), rsa_mod + 0x10, 0x100);
	}

	if (emu_cfg.enabled && !h_cfg.emummc_force_disable)
	{
		exo_cfg->emummc_cfg.base_cfg.magic = EMUMMC_MAGIC;
		exo_cfg->emummc_cfg.base_cfg.type = emu_cfg.sector ? emuMMC_Partition : emuMMC_File;
		exo_cfg->emummc_cfg.base_cfg.fs_ver = emu_cfg.fs_ver;
		exo_cfg->emummc_cfg.base_cfg.id = emu_cfg.id;

		if (emu_cfg.sector)
			exo_cfg->emummc_cfg.partition_cfg.start_sector = emu_cfg.sector;
		else
			strcpy((char *)exo_cfg->emummc_cfg.file_cfg.path, emu_cfg.path);

		if (emu_cfg.nintendo_path && !ctxt->stock)
			strcpy((char *)exo_cfg->emummc_cfg.nintendo_path, emu_cfg.nintendo_path);
		else if (ctxt->stock)
			strcpy((char *)exo_cfg->emummc_cfg.nintendo_path, "Nintendo");
		else
			exo_cfg->emummc_cfg.nintendo_path[0] = 0;
	}
}

static const char *get_error_desc(u32 error_desc)
{
	switch (error_desc)
	{
	case 0x100:
		return "Instruction Abort";
	case 0x101:
		return "Data Abort";
	case 0x102:
		return "PC Misalignment";
	case 0x103:
		return "SP Misalignment";
	case 0x104:
		return "Trap";
	case 0x106:
		return "SError";
	case 0x301:
		return "Bad SVC";
	case 0xFFE:
		return "std::abort()";
	default:
		return "Unknown";
	}
}

void secmon_exo_check_panic()
{
	volatile atm_fatal_error_ctx *rpt = (atm_fatal_error_ctx *)ATM_FATAL_ERR_CTX_ADDR;

	// Mask magic to maintain compatibility with any AFE version, thanks to additive struct members.
	if ((rpt->magic & 0xF0FFFFFF) != ATM_FATAL_MAGIC)
		return;

	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	WPRINTF("Panic occurred while running Atmosphere.\n\n");
	WPRINTFARGS("Title ID:   %08X%08X", (u32)((u64)rpt->title_id >> 32), (u32)rpt->title_id);
	WPRINTFARGS("Error Desc: %s (0x%x)\n", get_error_desc(rpt->error_desc), rpt->error_desc);

	// Save context to the SD card.
	char filepath[0x40];
	f_mkdir("atmosphere/fatal_errors");
	strcpy(filepath, "/atmosphere/fatal_errors/report_");
	itoa((u32)((u64)rpt->report_identifier >> 32), filepath + strlen(filepath), 16);
	itoa((u32)(rpt->report_identifier), filepath + strlen(filepath), 16);
	strcat(filepath, ".bin");

	sd_save_to_file((void *)rpt, sizeof(atm_fatal_error_ctx), filepath);

	gfx_con.fntsz = 8;
	WPRINTFARGS("Report saved to %s\n", filepath);

	// Change magic to invalid, to prevent double-display of error/bootlooping.
	rpt->magic = 0x0;

	gfx_con.fntsz = 16;
	gfx_printf("\n\nPress POWER to continue.\n");

	display_backlight_brightness(100, 1000);
	msleep(1000);

	u32 btn = btn_wait();
	while (!(btn & BTN_POWER))
		btn = btn_wait();

	display_backlight_brightness(0, 1000);
	gfx_con_setpos(0, 0);
}
