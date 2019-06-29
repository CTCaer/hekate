/*
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

#include "hos.h"
#include "sept.h"
#include "../config/config.h"
#include "../gfx/di.h"
#include "../ianos/ianos.h"
#include "../libs/fatfs/ff.h"
#include "../mem/heap.h"
#include "../soc/pmc.h"
#include "../soc/t210.h"
#include "../storage/nx_emmc.h"
#include "../storage/sdmmc.h"
#include "../utils/btn.h"
#include "../utils/types.h"

#include "../gfx/gfx.h"

#define RELOC_META_OFF   0x7C
#define PATCHED_RELOC_SZ 0x94

#define WB_RST_ADDR 0x40010ED0
#define WB_RST_SIZE 0x30

u8 warmboot_reboot[] = {
	0x14, 0x00, 0x9F, 0xE5, // LDR R0, =0x7000E450
	0x01, 0x10, 0xB0, 0xE3, // MOVS R1, #1
	0x00, 0x10, 0x80, 0xE5, // STR R1, [R0]
	0x0C, 0x00, 0x9F, 0xE5, // LDR R0, =0x7000E400 
	0x10, 0x10, 0xB0, 0xE3, // MOVS R1, #0x10
	0x00, 0x10, 0x80, 0xE5, // STR R1, [R0]
	0xFE, 0xFF, 0xFF, 0xEA, // LOOP
	0x50, 0xE4, 0x00, 0x70, // #0x7000E450
	0x00, 0xE4, 0x00, 0x70  // #0x7000E400
};

#define SEPT_PRI_ADDR   0x4003F000

#define SEPT_PK1T_ADDR  0xC0400000
#define SEPT_TCSZ_ADDR  (SEPT_PK1T_ADDR - 0x4)
#define SEPT_STG1_ADDR  (SEPT_PK1T_ADDR + 0x2E100)
#define SEPT_STG2_ADDR  (SEPT_PK1T_ADDR + 0x60E0)
#define SEPT_PKG_SZ     (0x2F100 + WB_RST_SIZE)

extern boot_cfg_t b_cfg;
extern hekate_config h_cfg;
extern const volatile ipl_ver_meta_t ipl_ver;

extern void *sd_file_read(char *path);
extern void sd_mount();
extern void sd_unmount();
extern bool is_ipl_updated(void *buf);
extern void reloc_patcher(u32 payload_dst, u32 payload_src, u32 payload_size);

void check_sept()
{
	// Check if non-hekate payload is used for sept and restore it.
	if (h_cfg.sept_run && !f_stat("sept/payload.bak", NULL))
	{
		f_unlink("sept/payload.bin");
		f_rename("sept/payload.bak", "sept/payload.bin");
		return;
	}

	u8 *pkg1 = (u8 *)calloc(1, 0x40000);

	sdmmc_storage_t storage;
	sdmmc_t sdmmc;
	if (!sdmmc_storage_init_mmc(&storage, &sdmmc, SDMMC_4, SDMMC_BUS_WIDTH_8, 4))
	{
		EPRINTF("Failed to init eMMC.");
		goto out_free;
	}
	sdmmc_storage_set_mmc_partition(&storage, 1);

	// Read package1.
	sdmmc_storage_read(&storage, 0x100000 / NX_EMMC_BLOCKSIZE, 0x40000 / NX_EMMC_BLOCKSIZE, pkg1);
	const pkg1_id_t *pkg1_id = pkg1_identify(pkg1);
	if (!pkg1_id)
	{
		gfx_con.fntsz = 16;
		EPRINTF("Unknown pkg1 version.");
		goto out_free;
	}

	if (pkg1_id->kb >= KB_FIRMWARE_VERSION_700 && !h_cfg.sept_run)
	{
		sdmmc_storage_end(&storage);
		reboot_to_sept((u8 *)pkg1 + pkg1_id->tsec_off, pkg1_id->kb);
	}

out_free:
	free(pkg1);
	sdmmc_storage_end(&storage);
}

int reboot_to_sept(const u8 *tsec_fw, const u32 kb)
{
	FIL fp;

	// Copy warmboot reboot code and TSEC fw.
	memcpy((u8 *)(SEPT_PK1T_ADDR - WB_RST_SIZE), (u8 *)warmboot_reboot, sizeof(warmboot_reboot));
	memcpy((void *)SEPT_PK1T_ADDR, tsec_fw, kb == KB_FIRMWARE_VERSION_700 ? 0x3000 : 0x3300);
	*(vu32 *)SEPT_TCSZ_ADDR = kb == KB_FIRMWARE_VERSION_700 ? 0x3000 : 0x3300;
	
	// Copy sept-primary.
	if (f_open(&fp, "sept/sept-primary.bin", FA_READ))
		goto error;

	if (f_read(&fp, (u8 *)SEPT_STG1_ADDR, f_size(&fp), NULL))
	{
		f_close(&fp);
		goto error;
	}
	f_close(&fp);

	// Copy sept-secondary.
	if ((kb == 7) && f_open(&fp, "sept/sept-secondary_00.enc", FA_READ))
		goto error;
	else if ((kb == 8) && f_open(&fp, "sept/sept-secondary_01.enc", FA_READ))
		goto error;

	if (f_read(&fp, (u8 *)SEPT_STG2_ADDR, f_size(&fp), NULL))
	{
		f_close(&fp);
		goto error;
	}
	f_close(&fp);

	b_cfg.boot_cfg |= (BOOT_CFG_AUTOBOOT_EN | BOOT_CFG_SEPT_RUN);

	bool update_sept_payload = true;
	if (!f_open(&fp, "sept/payload.bin", FA_READ | FA_WRITE))
	{
		ipl_ver_meta_t tmp_ver;
		f_lseek(&fp, PATCHED_RELOC_SZ + sizeof(boot_cfg_t));
		f_read(&fp, &tmp_ver, sizeof(ipl_ver_meta_t), NULL);

		if (tmp_ver.magic == ipl_ver.magic)
		{
			if (tmp_ver.version == ipl_ver.version)
			{
				// Save auto boot config to sept payload, if any.
				boot_cfg_t *tmp_cfg = malloc(sizeof(boot_cfg_t));
				memcpy(tmp_cfg, &b_cfg, sizeof(boot_cfg_t));
				f_lseek(&fp, PATCHED_RELOC_SZ);
				f_write(&fp, tmp_cfg, sizeof(boot_cfg_t), NULL);
				f_close(&fp);
				update_sept_payload = false;
			}
		}
		else
			f_rename("sept/payload.bin", "sept/payload.bak"); // Backup foreign payload.

		f_close(&fp);
	}

	if (update_sept_payload)
	{
		volatile reloc_meta_t *reloc = (reloc_meta_t *)(IPL_LOAD_ADDR + RELOC_META_OFF);
		f_open(&fp, "sept/payload.bin", FA_WRITE | FA_CREATE_ALWAYS);
		f_write(&fp, (u8 *)reloc->start, reloc->end - reloc->start, NULL);
		f_close(&fp);
	}

	sd_unmount();

	u32 pk1t_sept = SEPT_PK1T_ADDR - (ALIGN(PATCHED_RELOC_SZ, 0x10) + WB_RST_SIZE);

	void (*sept)() = (void *)pk1t_sept;

	reloc_patcher(WB_RST_ADDR, pk1t_sept, SEPT_PKG_SZ);

	// Patch SDRAM init to perform an SVC immediately after second write.
	PMC(APBDEV_PMC_SCRATCH45) = 0x2E38DFFF;
	PMC(APBDEV_PMC_SCRATCH46) = 0x6001DC28;
	// Set SVC handler to jump to sept-primary in IRAM.
	PMC(APBDEV_PMC_SCRATCH33) = SEPT_PRI_ADDR;
	PMC(APBDEV_PMC_SCRATCH40) = 0x6000F208;

	display_end();

	(*sept)();

error:
	EPRINTF("Failed to run sept\n");
	display_backlight_brightness(h_cfg.backlight, 1000);

	btn_wait();

	return 0;
}