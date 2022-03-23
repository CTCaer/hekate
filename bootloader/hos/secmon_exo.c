/*
 * Copyright (c) 2018-2022 CTCaer
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

#include <bdk.h>

#include "hos.h"
#include "../config.h"
#include <libs/fatfs/ff.h>
#include "../storage/emummc.h"

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
	u32 flags[2];
	u16 display_id;
	u8  uart_port;
	u8  uart_invert;
	u32 uart_baudrate;
	u32 rsvd1[2];
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
#define  ATM_FATAL_MAGIC       0x30454641 // AFE0

#define ATM_EXO_FATAL_ADDR     0x80020000
#define  ATM_EXO_FATAL_SIZE    SZ_128K

#define ATM_WB_HEADER_OFF      0x244
#define  ATM_WB_MAGIC          0x30544257 // WBT0

// Exosphère mailbox defines.
#define EXO_CFG_ADDR             0x8000F000
#define  EXO_MAGIC_VAL           0x304F5845
#define  EXO_FLAG_DBG_PRIV        BIT(1)
#define  EXO_FLAG_DBG_USER        BIT(2)
#define  EXO_FLAG_NO_USER_EXC     BIT(3)
#define  EXO_FLAG_USER_PMU        BIT(4)
#define  EXO_FLAG_CAL0_BLANKING   BIT(5)
#define  EXO_FLAG_CAL0_WRITES_SYS BIT(6)
#define  EXO_FLAG_ENABLE_USB3     BIT(7)

#define EXO_FW_VER(mj, mn, rv) (((mj) << 24) | ((mn) << 16) | ((rv) << 8))

void config_exosphere(launch_ctxt_t *ctxt, u32 warmboot_base)
{
	u32 exo_fw_no;
	u32 exo_flags = 0;
	bool usb3_force = false;
	bool user_debug = false;
	bool cal0_blanking = false;
	bool cal0_allow_writes_sys = false;

	memset((exo_cfg_t *)EXO_CFG_ADDR, 0, sizeof(exo_cfg_t));

	volatile exo_cfg_t *exo_cfg = (exo_cfg_t *)EXO_CFG_ADDR;

	//! TODO: Replace current HOS version decoding (as it's bound to break in the future).

	// Old exosphere target versioning. Use fuses for a simpler decoding.
	if (ctxt->pkg1_id->fuses <= 3 || ctxt->pkg1_id->fuses >= 10) // 1.0.0 - 3.0.0, 8.1.0+.
		exo_fw_no = ctxt->pkg1_id->fuses;
	else
		exo_fw_no = ctxt->pkg1_id->fuses - 1;                    // 3.0.1 - 7.0.1, 8.0.x.

	// Handle versions that change API and do not burn new fuse.
	if (!memcmp(ctxt->pkg1_id->id, "20190314172056", 8) || //  8.0.x, same fuses with  7.0.1.
		!memcmp(ctxt->pkg1_id->id, "20210129111626", 8) || // 12.0.0, same fuses with 11.0.0.
		!memcmp(ctxt->pkg1_id->id, "20210805123730", 8) || // 13.0.0, same fuses with 12.1.0.
		!memcmp(ctxt->pkg1_id->id, "20220209100018", 8)    // 14.0.0, same fuses with 13.2.1.
	   )
		exo_fw_no++;

	// Feed old exosphere target versioning to new.
	switch (exo_fw_no)
	{
	case 1 ... 4:
	case 6:
		exo_fw_no = EXO_FW_VER(exo_fw_no, 0, 0);
		break;
	case 5:
		exo_fw_no = EXO_FW_VER(5, ctxt->exo_ctx.hos_revision, 0);
		break;
	case 7:
		exo_fw_no = EXO_FW_VER(6, 2, 0);
		break;
	case 8 ... 9:
		exo_fw_no = EXO_FW_VER(exo_fw_no - 1, 0, 0);
		break;
	case 10:
		exo_fw_no = EXO_FW_VER(8, 1, 0);
		break;
	case 11:
		exo_fw_no = EXO_FW_VER(9, 0, 0);
		break;
	case 12:
		exo_fw_no = EXO_FW_VER(9, 1, 0);
		break;
	case 13 ... 17: //!TODO: Update on API changes. 17: 14.0.0.
		exo_fw_no = EXO_FW_VER(exo_fw_no - 3, ctxt->exo_ctx.hos_revision, 0);
		break;
	}

	// Parse exosphere.ini.
	if (!ctxt->stock)
	{
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
					else if (!strcmp("log_port", kv->key))
						exo_cfg->uart_port = atoi(kv->val);
					else if (!strcmp("log_inverted", kv->key))
						exo_cfg->uart_invert = atoi(kv->val);
					else if (!strcmp("log_baud_rate", kv->key))
						exo_cfg->uart_baudrate = atoi(kv->val);
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

		// Parse usb mtim settings. Avoid parsing if it's overridden.
		if (ctxt->fss0_main_path && !ctxt->exo_ctx.usb3_force)
		{
			char settings_path[256];
			strcpy(settings_path, ctxt->fss0_main_path);
			strcat(settings_path, "config/system_settings.ini");
			LIST_INIT(sys_settings);
			if (ini_parse(&ini_sections, settings_path, false))
			{
				LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
				{
					// Only parse usb section.
					if (!(ini_sec->type == INI_CHOICE) || strcmp(ini_sec->name, "usb"))
						continue;

					LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
					{
						if (!strcmp("usb30_force_enabled", kv->key))
						{
							usb3_force = !strcmp("u8!0x1", kv->val);
							break; // Only parse usb30_force_enabled key.
						}
					}
					break;
				}
			}
		}
	}

	// To avoid problems, make private debug mode always on if not semi-stock.
	if (!ctxt->stock || (emu_cfg.enabled && !h_cfg.emummc_force_disable))
		exo_flags |= EXO_FLAG_DBG_PRIV;

	// Enable user debug.
	if (user_debug)
		exo_flags |= EXO_FLAG_DBG_USER;

	// Disable proper failure handling.
	if (ctxt->exo_ctx.no_user_exceptions)
		exo_flags |= EXO_FLAG_NO_USER_EXC;

	// Enable user access to PMU.
	if (ctxt->exo_ctx.user_pmu)
		exo_flags |= EXO_FLAG_USER_PMU;

	// Enable USB 3.0. Check if system_settings ini value is overridden. If not, check if enabled in ini.
	if ((ctxt->exo_ctx.usb3_force && *ctxt->exo_ctx.usb3_force)
			|| (!ctxt->exo_ctx.usb3_force && usb3_force))
		exo_flags |= EXO_FLAG_ENABLE_USB3;

	// Enable prodinfo blanking. Check if exo ini value is overridden. If not, check if enabled in exo ini.
	if ((ctxt->exo_ctx.cal0_blank && *ctxt->exo_ctx.cal0_blank)
			|| (!ctxt->exo_ctx.cal0_blank && cal0_blanking))
		exo_flags |= EXO_FLAG_CAL0_BLANKING;

	// Allow prodinfo writes. Check if exo ini value is overridden. If not, check if enabled in exo ini.
	if ((ctxt->exo_ctx.cal0_allow_writes_sys && *ctxt->exo_ctx.cal0_allow_writes_sys)
			|| (!ctxt->exo_ctx.cal0_allow_writes_sys && cal0_allow_writes_sys))
		exo_flags |= EXO_FLAG_CAL0_WRITES_SYS;

	// Set mailbox values.
	exo_cfg->magic = EXO_MAGIC_VAL;
	exo_cfg->fwno = exo_fw_no;
	exo_cfg->flags[0] = exo_flags;

	// If warmboot is lp0fw, add in RSA modulus.
	volatile wb_cfg_t *wb_cfg = (wb_cfg_t *)(warmboot_base + ATM_WB_HEADER_OFF);

	if (wb_cfg->magic == ATM_WB_MAGIC)
	{
		wb_cfg->fwno = exo_fw_no;

		// Set warmboot binary rsa modulus.
		pkg1_warmboot_rsa_mod(warmboot_base);
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

	// Copy over exosphere fatal for Mariko.
	if (h_cfg.t210b01)
	{
		memset((void *)ATM_EXO_FATAL_ADDR, 0, ATM_EXO_FATAL_SIZE);
		if (ctxt->exofatal)
			memcpy((void *)ATM_EXO_FATAL_ADDR, ctxt->exofatal, ctxt->exofatal_size);

		// Set display id.
		exo_cfg->display_id = display_get_decoded_panel_id();
	}

#ifdef DEBUG_UART_PORT
	// Ovverride logging parameters if set in compile time.
	if (!ctxt->stock)
	{
		exo_cfg->uart_port = DEBUG_UART_PORT;
		exo_cfg->uart_invert = DEBUG_UART_INVERT;
		exo_cfg->uart_baudrate = DEBUG_UART_BAUDRATE;
	}
#endif
}

static const char *get_error_desc(u32 error_desc)
{
	switch (error_desc)
	{
	case 0x100:
		return "IABRT"; // Instruction Abort.
	case 0x101:
		return "DABRT"; // Data Abort.
	case 0x102:
		return "IUA";   // Instruction Unaligned Access.
	case 0x103:
		return "DUA";   // Data Unaligned Access.
	case 0x104:
		return "UDF";   // Undefined Instruction.
	case 0x106:
		return "SYS";   // System Error.
	case 0x301:
		return "SVC";   // Bad arguments or unimplemented SVC.
	case 0xF00:
		return "KRNL";  // Kernel panic.
	case 0xFFD:
		return "SO";    // Stack Overflow.
	case 0xFFE:
		return "std::abort";
	default:
		return "UNK";
	}
}

#define HOS_PID_BOOT2 0x8

void secmon_exo_check_panic()
{
	volatile atm_fatal_error_ctx *rpt = (atm_fatal_error_ctx *)ATM_FATAL_ERR_CTX_ADDR;

	// Mask magic to maintain compatibility with any AFE version, thanks to additive struct members.
	if ((rpt->magic & 0xF0FFFFFF) != ATM_FATAL_MAGIC)
		return;

	gfx_clear_grey(0x1B);
	gfx_con_setpos(0, 0);

	WPRINTF("Panic occurred while running Atmosphere.\n\n");
	WPRINTFARGS("Title ID: %08X%08X", (u32)((u64)rpt->title_id >> 32), (u32)rpt->title_id);
	WPRINTFARGS("Error:    %s (0x%x)\n", get_error_desc(rpt->error_desc), rpt->error_desc);

	// Check if mixed atmosphere sysmodules.
	if ((u32)rpt->title_id == HOS_PID_BOOT2)
		WPRINTF("Mismatched Atmosphere files?\n");

	// Save context to the SD card.
	char filepath[0x40];
	f_mkdir("atmosphere/fatal_errors");
	strcpy(filepath, "/atmosphere/fatal_errors/report_");
	itoa((u32)((u64)rpt->report_identifier >> 32), filepath + strlen(filepath), 16);
	itoa((u32)(rpt->report_identifier), filepath + strlen(filepath), 16);
	strcat(filepath, ".bin");

	if (!sd_save_to_file((void *)rpt, sizeof(atm_fatal_error_ctx), filepath))
	{
		gfx_con.fntsz = 8;
		WPRINTFARGS("Report saved to %s\n", filepath);
		gfx_con.fntsz = 16;
	}

	// Change magic to invalid, to prevent double-display of error/bootlooping.
	rpt->magic = 0;

	gfx_printf("\n\nPress POWER to continue.\n");

	display_backlight_brightness(100, 1000);
	msleep(1000);

	while (!(btn_wait() & BTN_POWER))
		;

	display_backlight_brightness(0, 1000);
	gfx_con_setpos(0, 0);
}
