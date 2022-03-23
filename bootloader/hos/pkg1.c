/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 st4rk
 * Copyright (c) 2018-2021 CTCaer
 * Copyright (c) 2018 balika011
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
#include "pkg1.h"
#include "../config.h"
#include <libs/compr/lz4.h>

extern hekate_config h_cfg;

// Secmon package2 signature/hash checks patches for Erista.
#define SM_100_ADR 0x4002B020 // Original: 0x40014020.
PATCHSET_DEF(_secmon_1_patchset,
	// Patch the relocator to be able to run from SM_100_ADR.
	{ 0x1E0, _ADRP(0, 0x7C013000 - _PAGEOFF(SM_100_ADR)) },
	// Patch package2 signature/hash checks.
	{ 0x9F0 + 0xADC, _NOP() }
);

PATCHSET_DEF(_secmon_2_patchset,
	// Patch package2 signature/hash checks.
	{ 0xAC8 + 0xAAC, _NOP() }
);

PATCHSET_DEF(_secmon_3_patchset,
	// Patch package2 signature/hash checks.
	{ 0xAC8 + 0xA30, _NOP() }
);

PATCHSET_DEF(_secmon_4_patchset,
	// Patch package2 signature/hash checks.
	{ 0x2300 + 0x5EFC, _NOP() }
);

PATCHSET_DEF(_secmon_5_patchset,
	// Patch package2 signature/hash checks.
	{ 0xDA8 + 0xC9C, _NOP() }
);

PATCHSET_DEF(_secmon_6_patchset,
	// Patch package2 signature/hash checks.
	{ 0xDC8 + 0xE90, _NOP() }
	// Fix sleep mode for debug.
	// { 0x1A68 + 0x3854, 0x94000E45 }, //gpio_config_for_uart.
	// { 0x1A68 + 0x3858, 0x97FFFC0F }, //clkrst_reboot_uarta.
	// { 0x1A68 + 0x385C, 0x52A00021 }, //MOV W1, #0x10000 ; baudrate.
	// { 0x1A68 + 0x3860, 0x2A1F03E0 }, //MOV W0, WZR ; uart_port -> A.
	// { 0x1A68 + 0x3864, 0x72984001 }, //MOVK W1, #0xC200 ; baudrate.
	// { 0x1A68 + 0x3868, 0x94000C8C }, //uart_configure.
	// { 0x1A68 + 0x3A6C, _NOP() }      // warmboot UARTA cfg.
);

PATCHSET_DEF(_secmon_62_patchset,
	// Patch package2 signature/hash checks.
	{ 0xDC8 + 0xC74, _NOP() }
	// Fix sleep mode for debug.
	// { 0x2AC8 + 0x3854, 0x94000F42 }, //gpio_config_for_uart.
	// { 0x2AC8 + 0x3858, 0x97FFFC0F }, //clkrst_reboot_uarta.
	// { 0x2AC8 + 0x385C, 0x52A00021 }, //MOV W1, #0x10000 ; baudrate.
	// { 0x2AC8 + 0x3860, 0x2A1F03E0 }, //MOV W0, WZR ; uart_port -> A.
	// { 0x2AC8 + 0x3864, 0x72984001 }, //MOVK W1, #0xC200 ; baudrate.
	// { 0x2AC8 + 0x3868, 0x94000D89 }, //uart_configure.
	// { 0x2AC8 + 0x3A6C, _NOP() }      // warmboot UARTA cfg.
);

// Secmon patches for Mariko.
#define TZRAM_PROG_ADDR               (TZRAM_BASE + 0x800)
#define TZRAM_COMPR_PROG_OFF          0xE04
#define TZRAM_PROG_PK2_SIG_PATCH      (TZRAM_PROG_ADDR + 0xC10)
#define TZRAM_PROG_PK2_SIG_PATCH_1000 (TZRAM_PROG_ADDR + 0xD70)
PATCHSET_DEF(_secmon_6_mariko_patchset,
	// Patch package2 decryption and signature/hash checks.
	{ 0xDC8 + 0xE94, _NOP() }
);

PATCHSET_DEF(_secmon_620_mariko_patchset,
	// Patch package2 decryption and signature/hash checks.
	{ 0xDC8 + 0xC78, _NOP() }
);

// From 7.0.0 and above secmon is compressed.
PATCHSET_DEF(_secmon_7_mariko_patchset,
	// Patch out decompression of program payload.
	{ 0x82C, _NOP() }
);

const u16 _secmon_mariko_prog_comp_size[] = {
	0x6B03, // 7.0.0.  Patch offset: 0xC10.
	0x6B16, // 7.0.1.  Patch offset: 0xC10.
	0x6B23, // 8.0.0.  Patch offset: 0xC10.
	0x6B84, // 8.1.0.  Patch offset: 0xC10.
	0x6C90, // 9.0.0.  Patch offset: 0xC10.
	0x6CE5, // 9.1.0.  Patch offset: 0xC10.
	0x6EE9, // 10.0.0. Patch offset: 0xD70.
};

// Erista fuse check warmboot patches.
#define _NOPv7() 0xE320F000
PATCHSET_DEF(_warmboot_1_patchset,
	{ 0x4DC, _NOPv7() } // Fuse check.
);

PATCHSET_DEF(_warmboot_3_patchset,
	{ 0x4DC, _NOPv7() }, // Fuse check.
	{ 0x4F0, _NOPv7() }  // Segment id check.
);

PATCHSET_DEF(_warmboot_4_patchset,
	{ 0x544, _NOPv7() }, // Fuse check.
	{ 0x558, _NOPv7() }  // Segment id check.
);

/*
 * package1.1 header: <wb, ldr, sm>
 * package1.1 layout:
 * 1.0:  {sm, ldr, wb} { 2, 1, 0 }
 * 2.0+: {wb, ldr, sm} { 0, 1, 2 }
 * 4.0+: {ldr, sm, wb} { 1, 2, 0 }
 */

static const u8 sec_map_100[3] = { PK11_SECTION_SM, PK11_SECTION_LD, PK11_SECTION_WB };
static const u8 sec_map_2xx[3] = { PK11_SECTION_WB, PK11_SECTION_LD, PK11_SECTION_SM };
static const u8 sec_map_4xx[3] = { PK11_SECTION_LD, PK11_SECTION_SM, PK11_SECTION_WB };

    // ID (Timestamp), KB, Fuses, TSEC, PK11,   SECMON,     Warmboot.
static const pkg1_id_t _pkg1_ids[] = {
	{ "20161121183008",  0,  1, 0x1900, 0x3FE0, SM_100_ADR, 0x8000D000, _secmon_1_patchset }, // 1.0.0 (Patched relocator).
	{ "20170210155124",  0,  2, 0x1900, 0x3FE0, 0x4002D000, 0x8000D000, _secmon_2_patchset }, // 2.0.0 - 2.3.0.
	{ "20170519101410",  1,  3, 0x1A00, 0x3FE0, 0x4002D000, 0x8000D000, _secmon_3_patchset }, // 3.0.0.
	{ "20170710161758",  2,  4, 0x1A00, 0x3FE0, 0x4002D000, 0x8000D000, _secmon_3_patchset }, // 3.0.1 - 3.0.2.
	{ "20170921172629",  3,  5, 0x1800, 0x3FE0, 0x4002B000, 0x4003B000, _secmon_4_patchset }, // 4.0.0 - 4.1.0.
	{ "20180220163747",  4,  6, 0x1900, 0x3FE0, 0x4002B000, 0x4003B000, _secmon_5_patchset }, // 5.0.0 - 5.1.0.
	{ "20180802162753",  5,  7, 0x1900, 0x3FE0, 0x4002B000, 0x4003D800, _secmon_6_patchset }, // 6.0.0 - 6.1.0.
	{ "20181107105733",  6,  8, 0x0E00, 0x6FE0, 0x4002B000, 0x4003D800, _secmon_62_patchset}, // 6.2.0.
	{ "20181218175730",  7,  9, 0x0F00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, //  7.0.0.
	{ "20190208150037",  7,  9, 0x0F00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, //  7.0.1.
	{ "20190314172056",  7,  9, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, //  8.0.0 - 8.0.1.
	{ "20190531152432",  8, 10, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, //  8.1.0 - 8.1.1.
	{ "20190809135709",  9, 11, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, //  9.0.0 - 9.0.1.
	{ "20191021113848", 10, 12, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, //  9.1.0 - 9.2.0.
	{ "20200303104606", 10, 13, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, // 10.0.0 - 10.2.0.
	{ "20201030110855", 10, 14, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, // 11.0.0 - 11.0.1.
	{ "20210129111626", 10, 14, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, // 12.0.0 - 12.0.1.
	{ "20210422145837", 10, 15, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, // 12.0.2 - 12.0.3.
	{ "20210607122020", 11, 15, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, // 12.1.0.
	{ "20210805123730", 12, 15, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, // 13.0.0 - 13.2.0.
	{ "20220105094454", 12, 16, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, // 13.2.1.
	{ "20220209100018", 13, 16, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000, NULL }, // 14.0.0+
};

const pkg1_id_t *pkg1_get_latest()
{
	return &_pkg1_ids[ARRAY_SIZE(_pkg1_ids) - 1];
}

const pkg1_id_t *pkg1_identify(u8 *pkg1)
{
	char build_date[15];
	pk1_hdr_t *hdr = (pk1_hdr_t *)pkg1;

	memcpy(build_date, hdr->timestamp, 14);
	build_date[14] = 0;
	gfx_printf("Found pkg1 ('%s').\n\n", build_date);

	for (int i = ARRAY_SIZE(_pkg1_ids) - 1; i >= 0; i--)
		if (!memcmp(hdr->timestamp, _pkg1_ids[i].id, 8))
			return &_pkg1_ids[i];

	return NULL;
}

int pkg1_decrypt(const pkg1_id_t *id, u8 *pkg1)
{
	pk11_hdr_t *hdr;

	// Decrypt package1.
	if (!h_cfg.t210b01)
	{
		u8 *pkg11 = pkg1 + id->pkg11_off;
		u32 pkg11_size = *(u32 *)pkg11;
		hdr = (pk11_hdr_t *)(pkg11 + 0x20);
		se_aes_crypt_ctr(11, hdr, pkg11_size, hdr, pkg11_size, pkg11 + 0x10);
	}
	else
	{
		bl_hdr_t210b01_t *oem_hdr = (bl_hdr_t210b01_t *)pkg1;
		pkg1 += sizeof(bl_hdr_t210b01_t);
		hdr = (pk11_hdr_t *)(pkg1 + id->pkg11_off + 0x20);

		// Use BEK for T210B01.
		// Additionally, skip 0x20 bytes from decryption to maintain the header.
		se_aes_iv_clear(13);
		se_aes_crypt_cbc(13, DECRYPT, pkg1 + 0x20, oem_hdr->size - 0x20, pkg1 + 0x20, oem_hdr->size - 0x20);
	}

	// Return if header is valid.
	return (hdr->magic == PKG1_MAGIC);
}

const u8 *pkg1_unpack(void *wm_dst, u32 *wb_sz, void *sm_dst, void *ldr_dst, const pkg1_id_t *id, u8 *pkg1)
{
	const u8 *sec_map;
	const pk11_hdr_t *hdr = (pk11_hdr_t *)(pkg1 + id->pkg11_off + 0x20);

	u32 sec_size[3] = { hdr->wb_size, hdr->ldr_size, hdr->sm_size };
	//u32 sec_off[3] = { hdr->wb_off, hdr->ldr_off, hdr->sm_off };

	// Get correct header mapping.
	if (id->fuses == 1)                        // 1.0.0.
		sec_map = sec_map_100;
	else if (id->fuses >= 2 && id->fuses <= 4) // 2.0.0 - 3.0.2.
		sec_map = sec_map_2xx;
	else                                       // 4.0.0+
		sec_map = sec_map_4xx;

	// Copy secmon, warmboot and nx bootloader payloads.
	u8 *pdata = (u8 *)hdr + sizeof(pk11_hdr_t);
	for (u32 i = 0; i < 3; i++)
	{
		if (sec_map[i] == PK11_SECTION_WB && wm_dst)
		{
			memcpy(wm_dst, pdata, sec_size[sec_map[i]]);
			if (wb_sz)
				*wb_sz = sec_size[sec_map[i]];
		}
		else if (sec_map[i] == PK11_SECTION_LD && ldr_dst)
			memcpy(ldr_dst, pdata, sec_size[sec_map[i]]);
		else if (sec_map[i] == PK11_SECTION_SM && sm_dst)
			memcpy(sm_dst, pdata, sec_size[sec_map[i]]);
		pdata += sec_size[sec_map[i]];
	}

	return sec_map;
}

void pkg1_secmon_patch(void *hos_ctxt, u32 secmon_base, bool t210b01)
{
	patch_t *secmon_patchset;
	launch_ctxt_t *ctxt = (launch_ctxt_t *)hos_ctxt;

	// Patch Secmon to allow for an unsigned package2 and patched kernel.
	if (!t210b01 && ctxt->pkg1_id->secmon_patchset)
	{
		// For T210 till 6.2.0 the patching is used as is, because of no compression.
		secmon_patchset = ctxt->pkg1_id->secmon_patchset;
	}
#ifdef HOS_MARIKO_STOCK_SECMON
	else if (t210b01)
	{
		// For T210B01 we patch 6.X.X as is. Otherwise we decompress the program payload.
		if (ctxt->pkg1_id->kb == KB_FIRMWARE_VERSION_600)
			secmon_patchset = _secmon_6_mariko_patchset;
		else if (ctxt->pkg1_id->kb == KB_FIRMWARE_VERSION_620)
			secmon_patchset = _secmon_620_mariko_patchset;
		else
		{
			// Patch uncompress of program payload clear TZRAM.
			secmon_patchset = _secmon_7_mariko_patchset;
			memset((void *)TZRAM_PROG_ADDR, 0, 0x38800);

			// Get size of compressed program payload and set patch offset.
			u32 idx = ctxt->pkg1_id->kb - KB_FIRMWARE_VERSION_700;
			u32 patch_offset = TZRAM_PROG_PK2_SIG_PATCH;
			if (ctxt->pkg1_id->kb > KB_FIRMWARE_VERSION_910 || !memcmp(ctxt->pkg1_id->id, "20200303104606", 8)) //TODO: Add 11.0.0 support.
			{
				idx++;
				patch_offset = TZRAM_PROG_PK2_SIG_PATCH_1000;
			}

			// Uncompress directly to TZRAM.
			LZ4_decompress_fast((const char*)(secmon_base + TZRAM_COMPR_PROG_OFF),
				(char *)TZRAM_PROG_ADDR, _secmon_mariko_prog_comp_size[idx]);

			// Patch package2 signature/hash checks.
			*(vu32 *)patch_offset = _NOP();
		}
	}
#endif
	else
		return;

	// Patch Secmon.
	gfx_printf("%kPatching Secure Monitor%k\n", 0xFFFFBA00, 0xFFCCCCCC);
	for (u32 i = 0; secmon_patchset[i].off != 0xFFFFFFFF; i++)
		*(vu32 *)(secmon_base + secmon_patchset[i].off) = secmon_patchset[i].val;
}

void pkg1_warmboot_patch(void *hos_ctxt)
{
	launch_ctxt_t *ctxt = (launch_ctxt_t *)hos_ctxt;
	patch_t *warmboot_patchset;

	// Patch warmboot on T210 to allow downgrading.
	switch (ctxt->pkg1_id->kb)
	{
	case 0:
		warmboot_patchset = _warmboot_1_patchset;
		break;
	case 1 ... 2:
		warmboot_patchset = _warmboot_3_patchset;
		break;
	default: // 4.0.0 - 6.2.0.
		warmboot_patchset = _warmboot_4_patchset;
		break;
	}
	gfx_printf("%kPatching Warmboot%k\n", 0xFFFFBA00, 0xFFCCCCCC);
	for (u32 i = 0; warmboot_patchset[i].off != 0xFFFFFFFF; i++)
		*(vu32 *)(ctxt->pkg1_id->warmboot_base + warmboot_patchset[i].off) = warmboot_patchset[i].val;
}

static void _warmboot_filename(char *out, u32 fuses)
{
	if (fuses < 16)
	{
		out[19] = '0';
		itoa(fuses, &out[19 + 1], 16);
	}
	else
		itoa(fuses, &out[19], 16);
	strcat(out, ".bin");
}

int pkg1_warmboot_config(void *hos_ctxt, u32 warmboot_base)
{
	launch_ctxt_t *ctxt = (launch_ctxt_t *)hos_ctxt;
	u32 kb = ctxt->pkg1_id->kb;
	int res = 1;

	// Set warmboot address in PMC if required.
	if (kb <= KB_FIRMWARE_VERSION_301)
		PMC(APBDEV_PMC_SCRATCH1) = warmboot_base;

	if (h_cfg.t210b01)
	{
		u32 pa_id;
		u32 fuses_max = 32; // Current ODM7 max.
		u32 fuses_fw = ctxt->pkg1_id->fuses;
		u8  burnt_fuses = bit_count(fuse_read_odm(7));

		// Save current warmboot in storage cache (MWS) and check if another one is needed.
		if (!ctxt->warmboot)
		{
			char path[128];
			f_mkdir("warmboot_mariko");
			strcpy(path, "warmboot_mariko/wb_");
			_warmboot_filename(path, fuses_fw);
			if (f_stat(path, NULL))
				sd_save_to_file((void *)warmboot_base, ctxt->warmboot_size, path);

			// Load warmboot fw from storage (MWS) if not matched.
			if (burnt_fuses > fuses_fw)
			{
				u32 tmp_fuses = burnt_fuses;
				while (true)
				{
					_warmboot_filename(path, burnt_fuses);
					if (!f_stat(path, NULL))
					{
						ctxt->warmboot = sd_file_read(path, &ctxt->warmboot_size);
						burnt_fuses = tmp_fuses;
						break;
					}
					if (tmp_fuses >= fuses_max)
						break;
					tmp_fuses++;
				}

				// Check if proper warmboot firmware was found.
				if (!ctxt->warmboot)
					res = 0;
			}
			else // Replace burnt fuses with higher count.
				burnt_fuses = fuses_fw;
		}

		// Configure Warmboot parameters. Anything lower is not supported.
		switch (burnt_fuses)
		{
		case 7:
			pa_id = 0x87;
			break;
		case 8: // 0x21 raise.
			pa_id = 0xA8;
			break;
		default: // From 7.0.0 and up PA id raises by 0x21 with a static base.
			pa_id = 0x108;
			pa_id += 0x21 * (burnt_fuses - 8);
			break;
		}

		// Set Warmboot Physical Address ID and lock SECURE_SCRATCH32 register.
		PMC(APBDEV_PMC_SECURE_SCRATCH32) = pa_id;
		PMC(APBDEV_PMC_SEC_DISABLE3) |= BIT(16);
	}
	else
	{
		// Set Warmboot Physical Address ID for 3.0.0 - 3.0.2.
		if (kb == KB_FIRMWARE_VERSION_300)
			PMC(APBDEV_PMC_SECURE_SCRATCH32) = 0xE3;  // Warmboot 3.0.0 PA address id.
		else if (kb == KB_FIRMWARE_VERSION_301)
			PMC(APBDEV_PMC_SECURE_SCRATCH32) = 0x104; // Warmboot 3.0.1/.2 PA address id.
	}

	return res;
}

void pkg1_warmboot_rsa_mod(u32 warmboot_base)
{
	// Set warmboot binary rsa modulus.
	u8 *rsa_mod = (u8 *)malloc(512);

	sdmmc_storage_set_mmc_partition(&emmc_storage, EMMC_BOOT0);

	u32 sector;
	u8  mod0, mod1;

	// Get the correct RSA modulus byte masks.
	nx_emmc_get_autorcm_masks(&mod0, &mod1);

	// Iterate BCTs.
	for (u32 i = 0; i < 4; i++)
	{
		sector = 1 + (32 * i); // 0x4000 bct + 0x200 offset.
		sdmmc_storage_read(&emmc_storage, sector, 1, rsa_mod);

		// Check if 2nd byte of modulus is correct.
		if (rsa_mod[0x11] != mod1)
			continue;

		// Patch AutoRCM out.
		rsa_mod[0x10] = mod0;

		break;
	}

	memcpy((void *)(warmboot_base + 0x10), rsa_mod + 0x10, 0x100);
}
