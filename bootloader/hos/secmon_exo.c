/*
 * Copyright (c) 2018-2019 CTCaer
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
#include "../gfx/di.h"
#include "../gfx/gfx.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../soc/fuse.h"
#include "../storage/sdmmc.h"
#include "../utils/btn.h"
#include "../utils/util.h"
#include "../utils/types.h"

extern bool sd_mount();
extern int sd_save_to_file(void *buf, u32 size, const char *filename);

typedef struct _exo_cfg_t
{
	vu32 magic;
	vu32 fwno;
	vu32 flags;
	vu32 rsvd;
} exo_cfg_t;

typedef struct _atm_meta_t
{
	uint32_t magic;
	uint32_t fwno;
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
	u64 padding;
	u32 pstate;
	u32 afsr0;
	u32 afsr1;
	u32 esr;
	u64 far;
	u64 report_identifier; // Normally just system tick.
} atm_fatal_error_ctx;

#define ATM_FATAL_ERR_CTX_ADDR 0x4003E000
#define  ATM_FATAL_MAGIC 0x30454641 // AFE0

#define ATM_WB_HEADER_OFF 0x244
#define  ATM_WB_MAGIC 0x30544257

// Exosphère mailbox defines.
#define EXO_CFG_DEPR_ADDR 0x40002E40 // Deprecated.
#define EXO_CFG_ADDR      0x8000F000
#define  EXO_MAGIC_DEPR_VAL 0x31434258
#define  EXO_MAGIC_VAL      0x304F5845
#define  EXO_FLAG_620_KGN   (1 << 0)
#define  EXO_FLAG_DBG_PRIV  (1 << 1)
#define  EXO_FLAG_DBG_USER  (1 << 2)

void config_exosphere(const char *id, u32 kb, void *warmboot, bool stock)
{
	u32 exoFwNo = 0;
	u32 exoFlags = 0;

	volatile exo_cfg_t *exo_cfg_depr = (exo_cfg_t *)EXO_CFG_DEPR_ADDR;
	volatile exo_cfg_t *exo_cfg = (exo_cfg_t *)EXO_CFG_ADDR;

	switch (kb)
	{
	case KB_FIRMWARE_VERSION_100_200:
		if (!strcmp(id, "20161121183008"))
			exoFwNo = 1;
		else
			exoFwNo = 2;
		break;
	case KB_FIRMWARE_VERSION_300:
		exoFwNo = 3;
		break;
	default:
		exoFwNo = kb + 1;
		break;
	}

	if (kb == KB_FIRMWARE_VERSION_620)
		exoFlags |= EXO_FLAG_620_KGN;

	// To avoid problems, make private debug mode always on if not semi-stock.
	if (!stock)
		exoFlags |= EXO_FLAG_DBG_PRIV;

	// Set mailbox values.
	exo_cfg_depr->magic = EXO_MAGIC_VAL;
	exo_cfg->magic = EXO_MAGIC_VAL;

	exo_cfg_depr->fwno = exoFwNo;
	exo_cfg->fwno = exoFwNo;

	exo_cfg_depr->flags = exoFlags;
	exo_cfg->flags = exoFlags;

	// If warmboot is lp0fw, add in RSA modulus.
	volatile wb_cfg_t *wb_cfg = (wb_cfg_t *)(warmboot + ATM_WB_HEADER_OFF);

	if (wb_cfg->magic == ATM_WB_MAGIC)
	{
		wb_cfg->fwno = exoFwNo;

		sdmmc_storage_t storage;
		sdmmc_t sdmmc;

		// Set warmboot binary rsa modulus.
		u8 *rsa_mod = (u8 *)malloc(512);

		sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4);
		sdmmc_storage_set_mmc_partition(&storage, 1);
		sdmmc_storage_read(&storage, 1, 1, rsa_mod);
		sdmmc_storage_end(&storage);

		// Patch AutoRCM out.
		if ((fuse_read_odm(4) & 3) != 3)
			rsa_mod[0x10] = 0xF7;
		else
			rsa_mod[0x10] = 0x37;

		memcpy(warmboot + 0x10, rsa_mod + 0x10, 0x100);
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
	default:
		return "Unknown";
	}
}

void secmon_exo_check_panic()
{
	volatile atm_fatal_error_ctx *rpt = (atm_fatal_error_ctx *)ATM_FATAL_ERR_CTX_ADDR;

	if (rpt->magic != ATM_FATAL_MAGIC)
		return;

	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	WPRINTF("Panic occurred while running Atmosphere.\n\n");
	WPRINTFARGS("Title ID:   %08X%08X", (u32)((u64)rpt->title_id >> 32), (u32)rpt->title_id);
	WPRINTFARGS("Error Desc: %s (0x%x)\n", get_error_desc(rpt->error_desc), rpt->error_desc);

	if (sd_mount())
	{
		// Save context to the SD card.
		char filepath[0x40];
		f_mkdir("atmosphere/fatal_errors");
		memcpy(filepath, "/atmosphere/fatal_errors/report_", 33);
		itoa((u32)((u64)rpt->report_identifier >> 32), filepath + strlen(filepath), 16);
		itoa((u32)(rpt->report_identifier), filepath + strlen(filepath), 16);
		memcpy(filepath + strlen(filepath), ".bin", 5);

		sd_save_to_file((void *)rpt, sizeof(atm_fatal_error_ctx), filepath);

		gfx_con.fntsz = 8;
		WPRINTFARGS("Report saved to %s\n", filepath);
	}

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
