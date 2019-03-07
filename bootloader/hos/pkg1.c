/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 st4rk
 * Copyright (c) 2018-2019 CTCaer
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

#include "pkg1.h"
#include "../gfx/gfx.h"
#include "../sec/se.h"
#include "../utils/aarch64_util.h"

#define _NOPv7() 0xE320F000

#define SM_100_ADR 0x4002B020
PATCHSET_DEF(_secmon_1_patchset,
	// Patch the relocator to be able to run from SM_100_ADR.
	{ 0x1E0, _ADRP(0, 0x7C013000 - _PAGEOFF(SM_100_ADR)) },
	//Patch package2 decryption and signature/hash checks.
	{ 0x9F0 + 0xADC, _NOP() }, //Header signature.
	{ 0x9F0 + 0xB8C, _NOP() }, //Version.
	{ 0x9F0 + 0xBB0, _NOP() }  //Sections SHA2.
);

PATCHSET_DEF(_secmon_2_patchset,
	// Patch package2 decryption and signature/hash checks.
	{ 0xAC8 + 0xAAC, _NOP() }, //Header signature.
	{ 0xAC8 + 0xB3C, _NOP() }, //Version.
	{ 0xAC8 + 0xB58, _NOP() }  //Sections SHA2.
);

PATCHSET_DEF(_secmon_3_patchset,
	// Patch package2 decryption and signature/hash checks.
	{ 0xAC8 + 0xA30, _NOP() }, //Header signature.
	{ 0xAC8 + 0xAB4, _NOP() }, //package2 structure.
	{ 0xAC8 + 0xAC0, _NOP() }, //Version.
	{ 0xAC8 + 0xADC, _NOP() }  //Sections SHA2.
);

PATCHSET_DEF(_secmon_4_patchset,
	// Patch package2 decryption and signature/hash checks.
	{ 0x2300 + 0x5D80, _NOP() }, //package2 structure.
	{ 0x2300 + 0x5D8C, _NOP() }, //Version.
	{ 0x2300 + 0x5EFC, _NOP() }, //Header signature.
	{ 0xAC8 + 0xA2C,   _NOP() }  //Sections SHA2.
);

PATCHSET_DEF(_secmon_5_patchset,
	// Patch package2 decryption and signature/hash checks.
	{ 0xDA8 + 0x9D8, _NOP() }, //package2 structure.
	{ 0xDA8 + 0x9E4, _NOP() }, //Version.
	{ 0xDA8 + 0xC9C, _NOP() }, //Header signature.
	{ 0xDA8 + 0x1038, _NOP() } //Sections SHA2.
);

PATCHSET_DEF(_secmon_6_patchset,
	// Patch package2 decryption and signature/hash checks.
	{ 0xDC8 + 0x820, _NOP() },  //package2 structure.
	{ 0xDC8 + 0x82C, _NOP() },  //Version.
	{ 0xDC8 + 0xE90, _NOP() },  //Header signature.
	{ 0xDC8 + 0x112C, _NOP() }, //Sections SHA2.
	// Fix sleep mode for debug.
	{ 0x1A68 + 0x3854, 0x94000E45 }, //gpio_config_for_uart.
	{ 0x1A68 + 0x3858, 0x97FFFC0F }, //clkrst_reboot_uarta.
	{ 0x1A68 + 0x385C, 0x52A00021 }, //MOV W1, #0x10000 ; baudrate.
	{ 0x1A68 + 0x3860, 0x2A1F03E0 }, //MOV W0, WZR ; uart_port -> A.
	{ 0x1A68 + 0x3864, 0x72984001 }, //MOVK W1, #0xC200 ; baudrate.
	{ 0x1A68 + 0x3868, 0x94000C8C }, //uart_configure.
	{ 0x1A68 + 0x3A6C, _NOP() }      // warmboot UARTA cfg.
);

PATCHSET_DEF(_secmon_620_patchset,
	// Patch package2 decryption and signature/hash checks.
	{ 0xDC8 + 0x604, _NOP() }, //package2 structure.
	{ 0xDC8 + 0x610, _NOP() }, //Version.
	{ 0xDC8 + 0xC74, _NOP() }, //Header signature.
	{ 0xDC8 + 0xF10, _NOP() }, //Sections SHA2.
	// Fix sleep mode for debug.
	{ 0x2AC8 + 0x3854, 0x94000F42 }, //gpio_config_for_uart.
	{ 0x2AC8 + 0x3858, 0x97FFFC0F }, //clkrst_reboot_uarta.
	{ 0x2AC8 + 0x385C, 0x52A00021 }, //MOV W1, #0x10000 ; baudrate.
	{ 0x2AC8 + 0x3860, 0x2A1F03E0 }, //MOV W0, WZR ; uart_port -> A.
	{ 0x2AC8 + 0x3864, 0x72984001 }, //MOVK W1, #0xC200 ; baudrate.
	{ 0x2AC8 + 0x3868, 0x94000D89 }, //uart_configure.
	{ 0x2AC8 + 0x3A6C, _NOP() }      // warmboot UARTA cfg.
);

PATCHSET_DEF(_warmboot_1_patchset,
	{ 0x4DC, _NOPv7() } // Fuse check.
);

PATCHSET_DEF(_warmboot_2_patchset,
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
 * 1.0: {sm, ldr, wb} { 2, 1, 0 }
 * 2.0: {wb, ldr, sm} { 0, 1, 2 }
 * 3.0: {wb, ldr, sm} { 0, 1, 2 }
 * 3.1: {wb, ldr, sm} { 0, 1, 2 }
 * 4.0: {ldr, sm, wb} { 1, 2, 0 }
 * 5.0: {ldr, sm, wb} { 1, 2, 0 }
 * 6.0: {ldr, sm, wb} { 1, 2, 0 }
 * 6.2: {ldr, sm, wb} { 1, 2, 0 }
 * 7.0: {ldr, sm, wb} { 1, 2, 0 }
 */

static const pkg1_id_t _pkg1_ids[] = {
	{ "20161121183008", 0, 0x1900, 0x3FE0, { 2, 1, 0 }, SM_100_ADR, 0x8000D000, true,  _secmon_1_patchset, _warmboot_1_patchset },   //1.0.0 (Patched relocator)
	{ "20170210155124", 0, 0x1900, 0x3FE0, { 0, 1, 2 }, 0x4002D000, 0x8000D000, true,  _secmon_2_patchset, _warmboot_2_patchset },   //2.0.0 - 2.3.0
	{ "20170519101410", 1, 0x1A00, 0x3FE0, { 0, 1, 2 }, 0x4002D000, 0x8000D000, true,  _secmon_3_patchset, _warmboot_3_patchset },   //3.0.0
	{ "20170710161758", 2, 0x1A00, 0x3FE0, { 0, 1, 2 }, 0x4002D000, 0x8000D000, true,  _secmon_3_patchset, _warmboot_3_patchset },   //3.0.1 - 3.0.2
	{ "20170921172629", 3, 0x1800, 0x3FE0, { 1, 2, 0 }, 0x4002B000, 0x4003B000, false, _secmon_4_patchset, _warmboot_4_patchset },   //4.0.0 - 4.1.0
	{ "20180220163747", 4, 0x1900, 0x3FE0, { 1, 2, 0 }, 0x4002B000, 0x4003B000, false, _secmon_5_patchset, _warmboot_4_patchset },   //5.0.0 - 5.1.0
	{ "20180802162753", 5, 0x1900, 0x3FE0, { 1, 2, 0 }, 0x4002B000, 0x4003D800, false, _secmon_6_patchset, _warmboot_4_patchset },   //6.0.0 - 6.1.0
	{ "20181107105733", 6, 0x0E00, 0x6FE0, { 1, 2, 0 }, 0x4002B000, 0x4003D800, false, _secmon_620_patchset, _warmboot_4_patchset }, //6.2.0
	{ "20181218175730", 7, 0x0F00, 0x6FE0, { 1, 2, 0 }, 0x40030000, 0x4003E000, false, NULL, _warmboot_4_patchset },                 //7.0.0
	{ "20190208150037", 7, 0x0F00, 0x6FE0, { 1, 2, 0 }, 0x40030000, 0x4003E000, false, NULL, _warmboot_4_patchset },                 //7.0.1
	{ NULL } //End.
};

const pkg1_id_t *pkg1_identify(u8 *pkg1)
{
	char build_date[15];
	memcpy(build_date, (char *)(pkg1 + 0x10), 14);
	build_date[14] = 0;
	gfx_printf(&gfx_con, "Found pkg1 ('%s').\n\n", build_date);

	for (u32 i = 0; _pkg1_ids[i].id; i++)
		if (!memcmp(pkg1 + 0x10, _pkg1_ids[i].id, 12))
			return &_pkg1_ids[i];
	return NULL;
}

void pkg1_decrypt(const pkg1_id_t *id, u8 *pkg1)
{
	// Decrypt package1.
	u8 *pkg11 = pkg1 + id->pkg11_off;
	u32 pkg11_size = *(u32 *)pkg11;
	se_aes_crypt_ctr(11, pkg11 + 0x20, pkg11_size, pkg11 + 0x20, pkg11_size, pkg11 + 0x10);
}

void pkg1_unpack(void *warmboot_dst, void *secmon_dst, void *ldr_dst, const pkg1_id_t *id, u8 *pkg1)
{
	pk11_hdr_t *hdr = (pk11_hdr_t *)(pkg1 + id->pkg11_off + 0x20);

	u32 sec_size[3] = { hdr->wb_size, hdr->ldr_size, hdr->sm_size };
	//u32 sec_off[3] = { hdr->wb_off, hdr->ldr_off, hdr->sm_off };

	u8 *pdata = (u8 *)hdr + sizeof(pk11_hdr_t);
	for (u32 i = 0; i < 3; i++)
	{
		if (id->sec_map[i] == 0 && warmboot_dst)
			memcpy(warmboot_dst, pdata, sec_size[id->sec_map[i]]);
		else if (id->sec_map[i] == 1 && ldr_dst)
			memcpy(ldr_dst, pdata, sec_size[id->sec_map[i]]);
		else if (id->sec_map[i] == 2 && secmon_dst)
			memcpy(secmon_dst, pdata, sec_size[id->sec_map[i]]);
		pdata += sec_size[id->sec_map[i]];
	}
}
