/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 balika011
 * Copyright (c) 2018-2025 CTCaer
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

#include <soc/bpmp.h>
#include <soc/ccplex.h>
#include <soc/timer.h>
#include <soc/t210.h>
#include <mem/mc_t210.h>
#include <mem/smmu.h>
#include <memory_map.h>

/*! SMMU register defines */
#define SMMU_ASID(asid)          (((asid) << 24u) | ((asid) << 16u) | ((asid) << 8u) | (asid))
#define SMMU_ENABLE              BIT(31)
#define SMMU_TLB_ACTIVE_LINES(l) ((l) << 0u)
#define SMMU_TLB_RR_ARBITRATION  BIT(28)
#define SMMU_TLB_HIT_UNDER_MISS  BIT(29)
#define SMMU_TLB_STATS_ENABLE    BIT(31)
#define SMUU_PTC_INDEX_MAP(m)    ((m) << 0u)
#define SMUU_PTC_LINE_MASK(m)    ((m) << 8u)
#define SMUU_PTC_REQ_LIMIT(l)    ((l) << 24u)
#define SMUU_PTC_CACHE_ENABLE    BIT(29)
#define SMUU_PTC_STATS_ENABLE    BIT(31)

/*! Page table defines */
#define SMMU_4MB_REGION 0
#define SMMU_PAGE_TABLE 1
#define SMMU_PDIR_COUNT 1024
#define SMMU_PTBL_COUNT 1024
#define SMMU_PAGE_SHIFT 12u
#define SMMU_PTN_SHIFT  SMMU_PAGE_SHIFT
#define SMMU_PDN_SHIFT  22u
#define SMMU_ADDR_TO_PFN(addr) ((addr) >> SMMU_PAGE_SHIFT)
#define SMMU_ADDR_TO_PTN(addr) ((addr) >> SMMU_PTN_SHIFT)
#define SMMU_ADDR_TO_PDN(addr) ((addr) >> SMMU_PDN_SHIFT)
#define SMMU_PTN_TO_ADDR(ptn)  ((ptn)  << SMMU_PTN_SHIFT)
#define SMMU_PDN_TO_ADDR(pdn)  ((pdn)  << SMMU_PDN_SHIFT)
#define SMMU_PTB(page, attr) (((attr) << 29u) | ((page) >> SMMU_PAGE_SHIFT))

#define SMMU_PAYLOAD_EN_SHIFT 4
#define SMMU_PAYLOAD_EN_SET   0x20
#define SMMU_PAYLOAD_EN_UNSET 0x00

// Enabling SMMU requires a TZ (EL3) secure write. MC(MC_SMMU_CONFIG) = 1;
static u8 smmu_enable_payload[] = {
	0xC1, 0x00, 0x00, 0x18, // 0x00: LDR  W1, =0x70019010
	0x20, 0x00, 0x80, 0xD2, // 0x04: MOV  X0, #0x1
	0x20, 0x00, 0x00, 0xB9, // 0x08: STR  W0, [X1]
	0x1F, 0x71, 0x08, 0xD5, // 0x0C: IC   IALLUIS
	0x9F, 0x3B, 0x03, 0xD5, // 0x10: DSB  ISH
	0xFE, 0xFF, 0xFF, 0x17, // 0x14: B    loop
	0x10, 0x90, 0x01, 0x70, // 0x18: MC_SMMU_CONFIG
};

static void *smmu_heap    = (void *)SMMU_HEAP_ADDR;
static bool  smmu_enabled = false;

void *smmu_page_zalloc(u32 num)
{
	void *page = smmu_heap;
	memset(page, 0, SZ_PAGE * num);

	smmu_heap += SZ_PAGE * num;

	return page;
}

static pde_t *_smmu_pdir_alloc()
{
	pde_t *pdir = (pde_t *)smmu_page_zalloc(1);

	// Initialize pdes with no permissions.
	for (u32 pdn = 0; pdn < SMMU_PDIR_COUNT; pdn++)
		pdir[pdn].huge.page = pdn;

	return pdir;
}

static void _smmu_flush_regs()
{
	(void)MC(MC_SMMU_PTB_DATA);
}

void smmu_flush_all()
{
	// Flush the entire page table cache.
	MC(MC_SMMU_PTC_FLUSH) = 0;
	_smmu_flush_regs();

	// Flush the entire table.
	MC(MC_SMMU_TLB_FLUSH) = 0;
	_smmu_flush_regs();
}

void smmu_init()
{
	MC(MC_SMMU_PTB_ASID)   = 0;
	MC(MC_SMMU_PTB_DATA)   = 0;
	MC(MC_SMMU_TLB_CONFIG) = SMMU_TLB_HIT_UNDER_MISS | SMMU_TLB_RR_ARBITRATION | SMMU_TLB_ACTIVE_LINES(48);
	MC(MC_SMMU_PTC_CONFIG) = SMUU_PTC_CACHE_ENABLE | SMUU_PTC_REQ_LIMIT(8) | SMUU_PTC_LINE_MASK(0xF) | SMUU_PTC_INDEX_MAP(0x3F);
	MC(MC_SMMU_PTC_FLUSH)  = 0;
	MC(MC_SMMU_TLB_FLUSH)  = 0;
}

void smmu_enable()
{
	if (smmu_enabled)
		return;

	// Launch payload on CCPLEX in order to set SMMU enable bit.
	ccplex_boot_cpu0((u32)smmu_enable_payload, false);
	msleep(100);
	ccplex_powergate_cpu0();

	smmu_flush_all();

	smmu_enabled = true;
}

void smmu_disable()
{
	if (!smmu_enabled)
		return;

	// Set payload to disable SMMU.
	smmu_enable_payload[SMMU_PAYLOAD_EN_SHIFT] = SMMU_PAYLOAD_EN_UNSET;

	smmu_flush_all();
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);

	// Launch payload on CCPLEX in order to set SMMU enable bit.
	ccplex_boot_cpu0((u32)smmu_enable_payload, false);
	msleep(100);
	ccplex_powergate_cpu0();

	smmu_flush_all();

	// Restore payload to SMMU enable.
	smmu_enable_payload[SMMU_PAYLOAD_EN_SHIFT] = SMMU_PAYLOAD_EN_SET;

	smmu_enabled = false;
}

void smmu_reset_heap()
{
	smmu_heap = (void *)SMMU_HEAP_ADDR;
}

void *smmu_init_domain(u32 dev_base, u32 asid)
{
	void *ptb = _smmu_pdir_alloc();

	MC(MC_SMMU_PTB_ASID) = asid;
	MC(MC_SMMU_PTB_DATA) = SMMU_PTB((u32)ptb, SMMU_ATTR_ALL);
	_smmu_flush_regs();

	// Use the same macro for both quad and single domains. Reserved bits are not set anyway.
	MC(dev_base) = SMMU_ENABLE | SMMU_ASID(asid);
	_smmu_flush_regs();

	return ptb;
}

void smmu_deinit_domain(u32 dev_base, u32 asid)
{
	MC(MC_SMMU_PTB_ASID) = asid;
	MC(MC_SMMU_PTB_DATA) = 0;
	MC(dev_base)         = 0;
	_smmu_flush_regs();
}

void smmu_domain_bypass(u32 dev_base, bool bypass)
{
	if (bypass)
	{
		smmu_flush_all();
		bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);
		MC(dev_base) &= ~SMMU_ENABLE;
	}
	else
	{
		bpmp_mmu_maintenance(BPMP_MMU_MAINT_CLN_INV_WAY, false);
		MC(dev_base) |=  SMMU_ENABLE;
		smmu_flush_all();
	}
	_smmu_flush_regs();
}

static pte_t *_smmu_get_pte(pde_t *pdir, u32 iova)
{
	u32 pdn = SMMU_ADDR_TO_PDN(iova);
	pte_t *ptbl;

	// Get 4MB page table or initialize one.
	if (pdir[pdn].tbl.attr)
		ptbl = (pte_t *)(SMMU_PTN_TO_ADDR(pdir[pdn].tbl.table));
	else
	{
		// Allocate page table.
		ptbl = (pte_t *)smmu_page_zalloc(1);

		// Get address.
		u32 addr = SMMU_PDN_TO_ADDR(pdn);

		// Initialize page table with no permissions.
		for (u32 pn = 0; pn < SMMU_PTBL_COUNT; pn++, addr += SZ_PAGE)
			ptbl[pn].page = SMMU_ADDR_TO_PFN(addr);

		// Set page table to the page directory.
		pdir[pdn].tbl.table = SMMU_ADDR_TO_PTN((u32)ptbl);
		pdir[pdn].tbl.next  = SMMU_PAGE_TABLE;
		pdir[pdn].tbl.attr  = SMMU_ATTR_ALL;

		smmu_flush_all();
	}

	return &ptbl[SMMU_ADDR_TO_PTN(iova) % SMMU_PTBL_COUNT];
}

void smmu_map(void *ptb, u32 iova, u64 iopa, u32 pages, u32 attr)
{
	// Map pages to page table entries. VA/PA should be aligned to 4KB.
	for (u32 i = 0; i < pages; i++)
	{
		pte_t *pte = _smmu_get_pte((pde_t *)ptb, iova);

		pte->page = SMMU_ADDR_TO_PFN(iopa);
		pte->attr = attr;

		iova += SZ_PAGE;
		iopa += SZ_PAGE;
	}

	smmu_flush_all();
}

void smmu_map_huge(void *ptb, u32 iova, u64 iopa, u32 regions, u32 attr)
{
	pde_t *pdir = (pde_t *)ptb;

	// Map 4MB regions to page directory entries. VA/PA should be aligned to 4MB.
	for (u32 i = 0; i < regions; i++)
	{
		u32 pdn = SMMU_ADDR_TO_PDN(iova);
		pdir[pdn].huge.page = SMMU_ADDR_TO_PDN(iopa);
		pdir[pdn].huge.next = SMMU_4MB_REGION;
		pdir[pdn].huge.attr = attr;

		iova += SZ_4M;
		iopa += SZ_4M;
	}

	smmu_flush_all();
}
