/*
* Copyright (c) 2018 CTCaer
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

#ifndef _TSEC_T210_H_
#define _TSEC_T210_H_

#define TSEC_BOOTKEYVER    0x1040
#define TSEC_STATUS        0x1044
#define TSEC_ITFEN         0x1048
#define  TSEC_ITFEN_CTXEN         BIT(0)
#define  TSEC_ITFEN_MTHDEN        BIT(1)
#define TSEC_IRQMSET       0x1010
#define  TSEC_IRQMSET_WDTMR       BIT(1)
#define  TSEC_IRQMSET_HALT        BIT(4)
#define  TSEC_IRQMSET_EXTERR      BIT(5)
#define  TSEC_IRQMSET_SWGEN0      BIT(6)
#define  TSEC_IRQMSET_SWGEN1      BIT(7)
#define  TSEC_IRQMSET_EXT(val)    (((val) & 0xFF) << 8)
#define TSEC_IRQDEST       0x101C
#define  TSEC_IRQDEST_HALT        BIT(4)
#define  TSEC_IRQDEST_EXTERR      BIT(5)
#define  TSEC_IRQDEST_SWGEN0      BIT(6)
#define  TSEC_IRQDEST_SWGEN1      BIT(7)
#define  TSEC_IRQDEST_EXT(val)    (((val) & 0xFF) << 8)
#define TSEC_CPUCTL       0x1100
#define  TSEC_CPUCTL_STARTCPU     BIT(1)
#define TSEC_BOOTVEC      0x1104
#define TSEC_DMACTL       0x110C
#define TSEC_DMATRFBASE   0x1110
#define TSEC_DMATRFMOFFS  0x1114
#define TSEC_DMATRFCMD    0x1118
#define  TSEC_DMATRFCMD_IDLE      BIT(1)
#define  TSEC_DMATRFCMD_IMEM      BIT(4)
#define  TSEC_DMATRFCMD_SIZE_256B (6 << 8)
#define TSEC_DMATRFFBOFFS 0x111C

#endif
