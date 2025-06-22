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

#include <assert.h>

#include <utils/types.h>

#define MC_SMMU_AVPC_ASID            0x23C
#define MC_SMMU_TSEC_ASID            0x294

#define SMMU_NS    BIT(0)
#define SMMU_WRITE BIT(1)
#define SMMU_READ  BIT(2)
#define SMMU_ATTR_ALL (SMMU_READ | SMMU_WRITE | SMMU_NS)

typedef struct _pde_t {
	union {
		union {
			struct {
				u32 table:22;
				u32 rsvd:6;
				u32 next:1;
				u32 attr:3;
			} tbl;

			struct {
				u32 rsvd_:10;
				u32 page:12;
				u32 rsvd:6;
				u32 next:1;
				u32 attr:3;
			} huge;
		};

		u32 pde;
	};
} pde_t;

typedef struct _pte_t {
	u32 page:22;
	u32 rsvd:7;
	u32 attr:3;
} pte_t;

static_assert(sizeof(pde_t) == sizeof(u32), "pde_t size is wrong!");
static_assert(sizeof(pte_t) == sizeof(u32), "pte_t size is wrong!");

void *smmu_page_zalloc(u32 num);
void  smmu_flush_all();
void  smmu_init();
void  smmu_enable();
void  smmu_disable();
void  smmu_reset_heap();
void *smmu_init_domain(u32 dev_base, u32 asid);
void  smmu_deinit_domain(u32 dev_base, u32 asid);
void  smmu_domain_bypass(u32 dev_base, bool bypass);
void  smmu_map(void *ptb, u32 iova, u64 iopa, u32 pages, u32 attr);
void  smmu_map_huge(void *ptb, u32 iova, u64 iopa, u32 regions, u32 attr);
