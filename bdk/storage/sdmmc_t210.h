/*
 * Copyright (c) 2018 naehrwert
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

#ifndef _SDMMC_T210_H_
#define _SDMMC_T210_H_

#include <assert.h>
#include <utils/types.h>

#define SDHCI_TEGRA_TUNING_TAP_HW_UPDATED BIT(17)
#define SDHCI_TEGRA_DLLCAL_CALIBRATE      BIT(31)
#define SDHCI_TEGRA_DLLCAL_ACTIVE         BIT(31)
#define SDHCI_TEGRA_PADCTRL_E_INPUT_PWRD  BIT(31)
#define SDHCI_TEGRA_PADCTRL_VREF_SEL_MASK 0xF
#define SDHCI_TEGRA_AUTOCAL_SLW_OVERRIDE  BIT(28)
#define SDHCI_TEGRA_AUTOCAL_ENABLE        BIT(29)
#define SDHCI_TEGRA_AUTOCAL_START         BIT(31)
#define SDHCI_TEGRA_AUTOCAL_ACTIVE        BIT(31)

typedef struct _t210_sdmmc_t
{
/*  0x00 */ vu32 sysad;  // sdma system address.
/*  0x04 */ vu16 blksize;
/*  0x06 */ vu16 blkcnt;
/*  0x08 */ vu32 argument;
/*  0x0C */ vu16 trnmod;
/*  0x0E */ vu16 cmdreg;
/*  0x10 */ vu32 rspreg[4];
/*  0x20 */ vu32 bdata; // Buffer data port.
/*  0x24 */ vu32 prnsts;
/*  0x28 */ vu8  hostctl;
/*  0x29 */ vu8  pwrcon;
/*  0x2A */ vu8  blkgap;
/*  0x2B */ vu8  wakcon;
/*  0x2C */ vu16 clkcon;
/*  0x2E */ vu8  timeoutcon;
/*  0x2F */ vu8  swrst;
/*  0x30 */ vu16 norintsts;   // Normal interrupt status.
/*  0x32 */ vu16 errintsts;   // Error  interrupt status.
/*  0x34 */ vu16 norintstsen; // Enable irq status.
/*  0x36 */ vu16 errintstsen; // Enable irq status.
/*  0x38 */ vu16 norintsigen; // Enable irq signal to LIC/GIC.
/*  0x3A */ vu16 errintsigen; // Enable irq signal to LIC/GIC.
/*  0x3C */ vu16 acmd12errsts;
/*  0x3E */ vu16 hostctl2;

// CAP0: 0x376CD08C.
//       12 MHz timeout clock. 208 MHz max base clock. 512B max block length. 8-bit support.
//       ADMA2 support. HS25 support. SDMA support. No suspend/resume support. 3.3/3.0/1.8V support.
//       64bit addressing for V3/V4 support. Async IRQ support. All report as removable.
/*  0x40 */ vu32 capareg;
// CAP1: 0x10002F73.
//       SDR50/SDR104 support. No DDR50 support. Drive A/B/C/D support.
//       Timer re-tuning info from other source. SDR50 requires re-tuning.
//       Tuning uses timer and transfers should be 4MB limited.
//       ADMA3 not supported. 1.8V VDD2 supported.
/*  0x44 */ vu32 capareg_hi;

/*  0x48 */ vu32 maxcurr;      // Get information by another method. Can be overriden via maxcurrover and maxcurrover_hi.
/*  0x4C */ vu32 maxcurr_hi;
/*  0x50 */ vu16 setacmd12err; // Force error in acmd12errsts.
/*  0x52 */ vu16 setinterr;
/*  0x54 */ vu8  admaerr;
/*  0x55 */ vu8  rsvd1[3];     // 55-57 reserved.
/*  0x58 */ vu32 admaaddr;
/*  0x5C */ vu32 admaaddr_hi;
/*  0x60 */ vu16 presets[11];
/*  0x76 */ vu16 rsvd2;
/*  0x78 */ vu32 adma3addr;
/*  0x7C */ vu32 adma3addr_hi;
/*  0x80 */ vu8  uhs2[124];    // 80-FB UHS-II.
/*  0xFC */ vu16 slotintsts;
/*  0xFE */ vu16 hcver;        // 0x303 (4.00).

/* UHS-II range. Used for Vendor registers here */
/* 0x100 */ vu32 venclkctl;
/* 0x104 */ vu32 vensysswctl;
/* 0x108 */ vu32 venerrintsts;
/* 0x10C */ vu32 vencapover;
/* 0x110 */ vu32 venbootctl;
/* 0x114 */ vu32 venbootacktout;
/* 0x118 */ vu32 venbootdattout;
/* 0x11C */ vu32 vendebouncecnt;
/* 0x120 */ vu32 venmiscctl;
/* 0x124 */ vu32 maxcurrover;
/* 0x128 */ vu32 maxcurrover_hi;
/* 0x12C */ vu32 unk0[32]; // 0x12C
/* 0x1AC */ vu32 veniotrimctl;
/* 0x1B0 */ vu32 vendllcalcfg;
/* 0x1B4 */ vu32 vendllctl0;
/* 0x1B8 */ vu32 vendllctl1;
/* 0x1BC */ vu32 vendllcalcfgsts;
/* 0x1C0 */ vu32 ventunctl0;
/* 0x1C4 */ vu32 ventunctl1;
/* 0x1C8 */ vu32 ventunsts0;
/* 0x1CC */ vu32 ventunsts1;
/* 0x1D0 */ vu32 venclkgatehystcnt;
/* 0x1D4 */ vu32 venpresetval0;
/* 0x1D8 */ vu32 venpresetval1;
/* 0x1DC */ vu32 venpresetval2;
/* 0x1E0 */ vu32 sdmemcmppadctl;
/* 0x1E4 */ vu32 autocalcfg;
/* 0x1E8 */ vu32 autocalintval;
/* 0x1EC */ vu32 autocalsts;
/* 0x1F0 */ vu32 iospare;
/* 0x1F4 */ vu32 mcciffifoctl;
/* 0x1F8 */ vu32 timeoutwcoal;
} t210_sdmmc_t;

static_assert(sizeof(t210_sdmmc_t) == 0x1FC, "T210 SDMMC REG size is wrong!");

#endif
