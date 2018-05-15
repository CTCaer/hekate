/*
* Copyright (c) 2018 naehrwert
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
#include "se.h"

#define _ADRP(r, o) 0x90000000 | ((((o) >> 12) & 0x3) << 29) | ((((o) >> 12) & 0x1FFFFC) << 3) | ((r) & 0x1F)
#define _MOVZX(r, i, s) 0xD2800000 | (((s) & 0x30) << 17) | (((i) & 0xFFFF) << 5) | ((r) & 0x1F)
#define _MOVKX(r, i, s) 0xF2800000 | (((s) & 0x30) << 17) | (((i) & 0xFFFF) << 5) | ((r) & 0x1F)
#define _BL(a, o) 0x94000000 | (((o) - (a)) >> 2) & 0x3FFFFFF
#define _NOP() 0xD503201F

PATCHSET_DEF(_secmon_1_patchset,
	//Patch the relocator to be able to run from 0x4002D000.
	//{ 0x1E0, _ADRP(0, 0x7C013000 - 0x4002D000) }
	//Patch package2 decryption and signature/hash checks.
	{ 0x9F0 + 0xADC, _NOP() }, //Header signature.
	{ 0x9F0 + 0xB8C, _NOP() }, //Version.
	{ 0x9F0 + 0xBB0, _NOP() }  //Sections SHA2.
);

PATCHSET_DEF(_secmon_2_patchset,
	//Patch package2 decryption and signature/hash checks.
	{ 0xAC8 + 0xAAC, _NOP() }, //Header signature.
	{ 0xAC8 + 0xB3C, _NOP() }, //Version.
	{ 0xAC8 + 0xB58, _NOP() }  //Sections SHA2.
);

PATCHSET_DEF(_secmon_3_patchset,
	//Patch package2 decryption and signature/hash checks.
	{ 0xAC8 + 0xA30, _NOP() }, //Header signature.
	{ 0xAC8 + 0xAC0, _NOP() }, //Version.
	{ 0xAC8 + 0xADC, _NOP() }  //Sections SHA2.
);

PATCHSET_DEF(_secmon_5_patchset,
	//Patch package2 decryption and signature/hash checks.
	{ 0x1218 + 0x6E68, _NOP() }, //Header signature.
	{ 0x1218 + 0x6E74, _NOP() }, //Version.
	{ 0x1218 + 0x6FE4, _NOP() }, //Sections SHA2.
	{ 0x1218 + 0x2DC, _NOP() }   //Unknown.
);

PATCHSET_DEF(_secmon_6_patchset,
	//Patch package2 decryption and signature/hash checks.
	{ 0x12b0 + 0x4d0, _NOP() },
	{ 0x12b0 + 0x4dc, _NOP() },
	{ 0x12b0 + 0x794, _NOP() },
	{ 0x12b0 + 0xb30, _NOP() }//,
	//{ 0x12b0 + 0xa18 , _NOP() } // BootConfig Retail Check
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
*/

static const pkg1_id_t _pkg1_ids[] = {
	{ "20161121183008", 0, 0x1900, 0x3FE0, { 2, 1, 0 }, 0x40014020, _secmon_1_patchset }, //1.0.0
	{ "20170210155124", 0, 0x1900, 0x3FE0, { 0, 1, 2 }, 0x4002D000, _secmon_2_patchset }, //2.0.0
	{ "20170519101410", 1, 0x1A00, 0x3FE0, { 0, 1, 2 }, 0x4002D000, NULL }, //3.0.0
	{ "20170710161758", 2, 0x1A00, 0x3FE0, { 0, 1, 2 }, 0x4002D000, NULL }, //3.0.1
	{ "20170921172629", 3, 0x1800, 0x3FE0, { 1, 2, 0 }, 0x4002B000, _secmon_5_patchset }, //4.0.0
	{ "20180220163747", 4, 0x1900, 0x3FE0, { 1, 2, 0 }, 0x4002B000, _secmon_6_patchset }, //5.0.0
	{ NULL, 0, 0, 0, 0 } //End.
};


const pkg1_id_t *pkg1_identify(u8 *pkg1)
{
	for (u32 i = 0; _pkg1_ids[i].id; i++)
		if (!memcmp(pkg1 + 0x10, _pkg1_ids[i].id, 12))
			return &_pkg1_ids[i];
	return NULL;
}

void pkg1_decrypt(const pkg1_id_t *id, u8 *pkg1)
{
	//Decrypt package1.
	u8 *pkg11 = pkg1 + id->pkg11_off;
	u32 pkg11_size = *(u32 *)pkg11;
	se_aes_crypt_ctr(11, pkg11 + 0x20, pkg11_size, pkg11 + 0x20, pkg11_size, pkg11 + 0x10);
}

void pkg1_unpack(void *warmboot_dst, void *secmon_dst, const pkg1_id_t *id, u8 *pkg1)
{
	pk11_hdr_t *hdr = (pk11_hdr_t *)(pkg1 + id->pkg11_off + 0x20);

	u32 sec_size[3] = { hdr->wb_size, hdr->ldr_size, hdr->sm_size };
	//u32 sec_off[3] = { hdr->wb_off, hdr->ldr_off, hdr->sm_off };

	u8 *pdata = (u8 *)hdr + sizeof(pk11_hdr_t);
	for (u32 i = 0; i < 3; i++)
	{
		if (id->sec_map[i] == 0 && warmboot_dst)
			memcpy(warmboot_dst, pdata, sec_size[id->sec_map[i]]);
		else if (id->sec_map[i] == 2 && secmon_dst)
			memcpy(secmon_dst, pdata, sec_size[id->sec_map[i]]);
		pdata += sec_size[id->sec_map[i]];
	}
}
