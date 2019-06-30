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

#ifndef _CLUSTER_H_
#define _CLUSTER_H_

#include "../utils/types.h"

/*! Flow controller registers. */
#define FLOW_CTLR_HALT_CPU0_EVENTS 0x0
#define FLOW_CTLR_HALT_CPU1_EVENTS 0x14
#define FLOW_CTLR_HALT_CPU2_EVENTS 0x1C
#define FLOW_CTLR_HALT_CPU3_EVENTS 0x24
#define FLOW_CTLR_HALT_COP_EVENTS 0x4
#define FLOW_CTLR_CPU0_CSR 0x8
#define FLOW_CTLR_CPU1_CSR 0x18
#define FLOW_CTLR_CPU2_CSR 0x20
#define FLOW_CTLR_CPU3_CSR 0x28
#define FLOW_CTLR_RAM_REPAIR 0x40
#define FLOW_CTLR_BPMP_CLUSTER_CONTROL 0x98

void cluster_boot_cpu0(u32 entry);

#endif
