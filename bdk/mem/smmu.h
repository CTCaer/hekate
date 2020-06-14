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

#include <utils/types.h>

#define SMMU_HEAP_ADDR 0xA0000000

#define MC_INTSTATUS                 0x0
#define MC_INTMASK                   0x4
#define MC_ERR_STATUS                0x8
#define MC_ERR_ADR                   0xc
#define MC_SMMU_CONFIG               0x10
#define MC_SMMU_TLB_CONFIG           0x14
#define MC_SMMU_PTC_CONFIG           0x18
#define MC_SMMU_PTB_ASID             0x1c
#define MC_SMMU_PTB_DATA             0x20
#define MC_SMMU_TLB_FLUSH            0x30
#define MC_SMMU_PTC_FLUSH            0x34
#define MC_SMMU_ASID_SECURITY        0x38
#define MC_SMMU_TSEC_ASID            0x294
#define MC_SMMU_TRANSLATION_ENABLE_0 0x228
#define MC_SMMU_TRANSLATION_ENABLE_1 0x22c
#define MC_SMMU_TRANSLATION_ENABLE_2 0x230
#define MC_SMMU_TRANSLATION_ENABLE_3 0x234
#define MC_SMMU_TRANSLATION_ENABLE_4 0xb98

#define SMMU_PDE_NEXT_SHIFT 28
#define MC_SMMU_PTB_DATA_0_ASID_NONSECURE_SHIFT 29
#define MC_SMMU_PTB_DATA_0_ASID_WRITABLE_SHIFT  30
#define MC_SMMU_PTB_DATA_0_ASID_READABLE_SHIFT  31
#define SMMU_PAGE_SHIFT 12
#define SMMU_PAGE_SIZE  (1 << SMMU_PAGE_SHIFT)
#define SMMU_PDIR_COUNT 1024
#define SMMU_PDIR_SIZE  (sizeof(u32) * SMMU_PDIR_COUNT)
#define SMMU_PTBL_COUNT 1024
#define SMMU_PTBL_SIZE  (sizeof(u32) * SMMU_PTBL_COUNT)
#define SMMU_PDIR_SHIFT 12
#define SMMU_PDE_SHIFT  12
#define SMMU_PTE_SHIFT  12
#define SMMU_PFN_MASK   0x000FFFFF
#define SMMU_ADDR_TO_PFN(addr) ((addr) >> 12)
#define SMMU_ADDR_TO_PDN(addr) ((addr) >> 22)
#define SMMU_PDN_TO_ADDR(addr) ((pdn) << 22)
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
#define SMMU_MK_PDIR(page, attr) (((page) >> SMMU_PDIR_SHIFT) | (attr))
#define SMMU_MK_PDE(page, attr)  (((page) >> SMMU_PDE_SHIFT) | (attr))

void *page_alloc(u32 num);
u32 *smmu_alloc_pdir();
void smmu_flush_regs();
void smmu_flush_all();
void smmu_init(u32 secmon_base);
void smmu_enable();
bool smmu_is_used();
void smmu_exit();
u32 *smmu_init_domain4(u32 dev_base, u32 asid);
u32 *smmu_get_pte(u32 *pdir, u32 iova);
void smmu_map(u32 *pdir, u32 addr, u32 page, int cnt, u32 attr);
u32 *smmu_init_for_tsec();
void smmu_deinit_for_tsec();
