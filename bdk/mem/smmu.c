/*
 * Copyright (c) 2018 naehrwert
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

#include <soc/ccplex.h>
#include <soc/t210.h>
#include <mem/mc_t210.h>
#include <mem/smmu.h>
#include <utils/util.h>
#include <utils/aarch64_util.h>

bool smmu_used = false;
u8 *_pageheap = (u8 *)SMMU_HEAP_ADDR;

//Enabling SMMU requires a TZ secure write: MC(MC_SMMU_CONFIG) = 1;
u8 smmu_payload[] __attribute__((aligned(16))) = {
	0x41, 0x01, 0x00, 0x58, // 0x00: LDR  X1, =0x70019010
	0x20, 0x00, 0x80, 0xD2, // 0x04: MOV  X0, #0x1
	0x20, 0x00, 0x00, 0xB9, // 0x08: STR  W0, [X1]
	0x1F, 0x71, 0x08, 0xD5, // 0x0C: IC   IALLUIS
	0x9F, 0x3B, 0x03, 0xD5, // 0x10: DSB  ISH
	0xFE, 0xFF, 0xFF, 0x17, // 0x14: B    loop
	0x00, 0x00, 0x80, 0xD2, // 0x18: MOV  X0, #0x0
	0x20, 0x00, 0x00, 0xB9, // 0x1C: STR  W0, [X1]
	0x80, 0x00, 0x00, 0x58, // 0x20: LDR  X0, =0x4002B000
	0x00, 0x00, 0x1F, 0xD6, // 0x28: BR   X0
	0x10, 0x90, 0x01, 0x70, // 0x28: MC_SMMU_CONFIG
	0x00, 0x00, 0x00, 0x00, // 0x2C:
	0x00, 0x00, 0x00, 0x00, // 0x30: secmon address
	0x00, 0x00, 0x00, 0x00  // 0x34:
};

void *page_alloc(u32 num)
{
	u8 *res = _pageheap;
	_pageheap += SZ_PAGE * num;
	memset(res, 0, SZ_PAGE * num);
	return res;
}

u32 *smmu_alloc_pdir()
{
	u32 *pdir = (u32 *)page_alloc(1);
	for (int pdn = 0; pdn < SMMU_PDIR_COUNT; pdn++)
		pdir[pdn] = _PDE_VACANT(pdn);
	return pdir;
}

void smmu_flush_regs()
{
	(void)MC(MC_SMMU_PTB_DATA);
}

void smmu_flush_all()
{
	MC(MC_SMMU_PTC_FLUSH) = 0;
	smmu_flush_regs();
	MC(MC_SMMU_TLB_FLUSH) = 0;
	smmu_flush_regs();
}

void smmu_init(u32 secmon_base)
{
	MC(MC_SMMU_PTB_ASID) = 0;
	MC(MC_SMMU_PTB_DATA) = 0;
	MC(MC_SMMU_TLB_CONFIG) = 0x30000030;
	MC(MC_SMMU_PTC_CONFIG) = 0x28000F3F;
	MC(MC_SMMU_PTC_FLUSH) = 0;
	MC(MC_SMMU_TLB_FLUSH) = 0;

	// Set the secmon address
	*(u32 *)(smmu_payload + 0x30) = secmon_base;
}

void smmu_enable()
{
	if (smmu_used)
		return;

	ccplex_boot_cpu0((u32)smmu_payload);
	smmu_used = true;
	msleep(150);

	smmu_flush_all();
}

bool smmu_is_used()
{
	return smmu_used;
}

void smmu_exit()
{
	*(u32 *)(smmu_payload + 0x14) = _NOP();
}

u32 *smmu_init_domain4(u32 dev_base, u32 asid)
{
	u32 *pdir = smmu_alloc_pdir();

	MC(MC_SMMU_PTB_ASID) = asid;
	MC(MC_SMMU_PTB_DATA) = SMMU_MK_PDIR((u32)pdir, _PDIR_ATTR);
	smmu_flush_regs();

	MC(dev_base) = 0x80000000 | (asid << 24) | (asid << 16) | (asid << 8) | (asid);
	smmu_flush_regs();

	return pdir;
}

u32 *smmu_get_pte(u32 *pdir, u32 iova)
{
	u32 ptn = SMMU_ADDR_TO_PFN(iova);
	u32 pdn = SMMU_ADDR_TO_PDN(iova);
	u32 *ptbl;

	if (pdir[pdn] != _PDE_VACANT(pdn))
		ptbl = (u32 *)((pdir[pdn] & SMMU_PFN_MASK) << SMMU_PDIR_SHIFT);
	else
	{
		ptbl = (u32 *)page_alloc(1);
		u32 addr = SMMU_PDN_TO_ADDR(pdn);
		for (int pn = 0; pn < SMMU_PTBL_COUNT; pn++, addr += SMMU_PAGE_SIZE)
			ptbl[pn] = _PTE_VACANT(addr);
		pdir[pdn] = SMMU_MK_PDE((u32)ptbl, _PDE_ATTR | _PDE_NEXT);
		smmu_flush_all();
	}

	return &ptbl[ptn % SMMU_PTBL_COUNT];
}

void smmu_map(u32 *pdir, u32 addr, u32 page, int cnt, u32 attr)
{
	for (int i = 0; i < cnt; i++)
	{
		u32 *pte = smmu_get_pte(pdir, addr);
		*pte = SMMU_ADDR_TO_PFN(page) | attr;
		addr += SZ_PAGE;
		page += SZ_PAGE;
	}
	smmu_flush_all();
}

u32 *smmu_init_for_tsec()
{
	return smmu_init_domain4(MC_SMMU_TSEC_ASID, 1);
}

void smmu_deinit_for_tsec()
{
	MC(MC_SMMU_PTB_ASID) = 1;
	MC(MC_SMMU_PTB_DATA) = 0;
	MC(MC_SMMU_TSEC_ASID) = 0;
	smmu_flush_regs();
}

