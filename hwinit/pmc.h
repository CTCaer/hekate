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

#ifndef _PMC_H_
#define _PMC_H_

/*! PMC registers. */
#define APBDEV_PMC_PWRGATE_TOGGLE 0x30
#define APBDEV_PMC_PWRGATE_STATUS 0x38
#define APBDEV_PMC_NO_IOPOWER 0x44
#define APBDEV_PMC_SCRATCH20 0xA0
#define APBDEV_PMC_DDR_PWR 0xE8
#define APBDEV_PMC_CRYPTO_OP 0xF4
#define APBDEV_PMC_OSC_EDPD_OVER 0x1A4
#define APBDEV_PMC_IO_DPD_REQ 0x1B8
#define APBDEV_PMC_IO_DPD2_REQ 0x1C0
#define APBDEV_PMC_VDDP_SEL 0x1CC
#define APBDEV_PMC_TSC_MULT 0x2B4
#define APBDEV_PMC_REG_SHORT 0x2CC
#define APBDEV_PMC_WEAK_BIAS 0x2C8
#define APBDEV_PMC_SECURE_SCRATCH21 0x334
#define APBDEV_PMC_CNTRL2 0x440
#define APBDEV_PMC_IO_DPD4_REQ 0x464
#define APBDEV_PMC_DDR_CNTRL 0x4E4
#define APBDEV_PMC_SCRATCH188 0x810
#define APBDEV_PMC_SCRATCH190 0x818
#define APBDEV_PMC_SCRATCH200 0x840

#endif
