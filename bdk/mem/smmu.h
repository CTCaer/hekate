/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2024 CTCaer
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

#include <utils/types.h>

#define MC_SMMU_AVPC_ASID            0x23C
#define MC_SMMU_TSEC_ASID            0x294

#define SMMU_PDE_NEXT_SHIFT 28
#define MC_SMMU_PTB_DATA_0_ASID_NONSECURE_SHIFT 29
#define MC_SMMU_PTB_DATA_0_ASID_WRITABLE_SHIFT  30
#define MC_SMMU_PTB_DATA_0_ASID_READABLE_SHIFT  31
#define _READABLE  (1 << MC_SMMU_PTB_DATA_0_ASID_READABLE_SHIFT)
#define _WRITABLE  (1 << MC_SMMU_PTB_DATA_0_ASID_WRITABLE_SHIFT)
#define _NONSECURE (1 << MC_SMMU_PTB_DATA_0_ASID_NONSECURE_SHIFT)
#define _PDE_NEXT  (1 << SMMU_PDE_NEXT_SHIFT)
#define _MASK_ATTR (_READABLE | _WRITABLE | _NONSECURE)
#define _PDIR_ATTR (_READABLE | _WRITABLE | _NONSECURE)
#define _PDE_ATTR  (_READABLE | _WRITABLE | _NONSECURE)
#define _PDE_VACANT(pdn) (((pdn) << 10) | _PDE_ATTR)
#define _PTE_ATTR  (_READABLE | _WRITABLE | _NONSECURE)
#define _PTE_VACANT(addr) (((addr) >> SMMU_PAGE_SHIFT) | _PTE_ATTR)

void *smmu_page_zalloc(u32 num);
void  smmu_flush_all();
void  smmu_init();
void  smmu_enable();
u32  *smmu_init_domain4(u32 dev_base, u32 asid);
u32  *smmu_get_pte(u32 *pdir, u32 iova);
void  smmu_map(u32 *pdir, u32 addr, u32 page, int cnt, u32 attr);
u32  *smmu_init_domain(u32 asid);
void  smmu_deinit_domain(u32 asid);
