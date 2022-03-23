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

#include <bdk.h>

#include "hos.h"
#include "pkg1.h"
#include "../config.h"

extern hekate_config h_cfg;

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

    // ID (Timestamp),  KB, TSEC,   PK11,   SECMON,     Warmboot.
static const pkg1_id_t _pkg1_ids[] = {
	{ "20161121183008",  0, 0x1900, 0x3FE0, 0x40014020, 0x8000D000 }, //  1.0.0.
	{ "20170210155124",  0, 0x1900, 0x3FE0, 0x4002D000, 0x8000D000 }, //  2.0.0 - 2.3.0.
	{ "20170519101410",  1, 0x1A00, 0x3FE0, 0x4002D000, 0x8000D000 }, //  3.0.0.
	{ "20170710161758",  2, 0x1A00, 0x3FE0, 0x4002D000, 0x8000D000 }, //  3.0.1 - 3.0.2.
	{ "20170921172629",  3, 0x1800, 0x3FE0, 0x4002B000, 0x4003B000 }, //  4.0.0 - 4.1.0.
	{ "20180220163747",  4, 0x1900, 0x3FE0, 0x4002B000, 0x4003B000 }, //  5.0.0 - 5.1.0.
	{ "20180802162753",  5, 0x1900, 0x3FE0, 0x4002B000, 0x4003D800 }, //  6.0.0 - 6.1.0.
	{ "20181107105733",  6, 0x0E00, 0x6FE0, 0x4002B000, 0x4003D800 }, //  6.2.0.
	{ "20181218175730",  7, 0x0F00, 0x6FE0, 0x40030000, 0x4003E000 }, //  7.0.0.
	{ "20190208150037",  7, 0x0F00, 0x6FE0, 0x40030000, 0x4003E000 }, //  7.0.1.
	{ "20190314172056",  7, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, //  8.0.0 - 8.0.1.
	{ "20190531152432",  8, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, //  8.1.0 - 8.1.1.
	{ "20190809135709",  9, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, //  9.0.0 - 9.0.1.
	{ "20191021113848", 10, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, //  9.1.0 - 9.2.0.
	{ "20200303104606", 10, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, // 10.0.0 - 10.2.0.
	{ "20201030110855", 10, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, // 11.0.0 - 11.0.1.
	{ "20210129111626", 10, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, // 12.0.0 - 12.0.1.
	{ "20210422145837", 10, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, // 12.0.2 - 12.0.3.
	{ "20210607122020", 11, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, // 12.1.0.
	{ "20210805123730", 12, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, // 13.0.0 - 13.2.0
	{ "20220105094454", 12, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, // 13.2.1.
	{ "20220209100018", 13, 0x0E00, 0x6FE0, 0x40030000, 0x4003E000 }, // 14.0.0+
};

const pkg1_id_t *pkg1_identify(u8 *pkg1, char *build_date)
{
	if (build_date)
	{
		memcpy(build_date, (char *)(pkg1 + 0x10), 14);
		build_date[14] = 0;
	}

	for (u32 i = 0; i < ARRAY_SIZE(_pkg1_ids); i++)
		if (!memcmp(pkg1 + 0x10, _pkg1_ids[i].id, 8))
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

const u8 *pkg1_unpack(void *wm_dst, void *sm_dst, void *ldr_dst, const pkg1_id_t *id, u8 *pkg1)
{
	const u8 *sec_map;
	const pk11_hdr_t *hdr = (pk11_hdr_t *)(pkg1 + id->pkg11_off + 0x20);

	u32 sec_size[3] = { hdr->wb_size, hdr->ldr_size, hdr->sm_size };
	//u32 sec_off[3] = { hdr->wb_off, hdr->ldr_off, hdr->sm_off };

	// Get correct header mapping.
	if (id->kb == KB_FIRMWARE_VERSION_100 && !strcmp(id->id, "20161121183008"))
		sec_map = sec_map_100;
	else if (id->kb >= KB_FIRMWARE_VERSION_100 && id->kb <= KB_FIRMWARE_VERSION_301)
		sec_map = sec_map_2xx;
	else
		sec_map = sec_map_4xx;

	// Copy secmon, warmboot and nx bootloader payloads.
	u8 *pdata = (u8 *)hdr + sizeof(pk11_hdr_t);
	for (u32 i = 0; i < 3; i++)
	{
		if (sec_map[i] == PK11_SECTION_WB && wm_dst)
			memcpy(wm_dst, pdata, sec_size[sec_map[i]]);
		else if (sec_map[i] == PK11_SECTION_LD && ldr_dst)
			memcpy(ldr_dst, pdata, sec_size[sec_map[i]]);
		else if (sec_map[i] == PK11_SECTION_SM && sm_dst)
			memcpy(sm_dst, pdata, sec_size[sec_map[i]]);
		pdata += sec_size[sec_map[i]];
	}

	return sec_map;
}
