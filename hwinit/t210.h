#ifndef _T210_H_
#define _T210_H_

#include "types.h"

#define HOST1X_BASE 0x50000000
#define DISPLAY_A_BASE 0x54200000
#define DSI_BASE 0x54300000
#define VIC_BASE 0x54340000
#define TMR_BASE 0x60005000
#define CLOCK_BASE 0x60006000
#define SYSREG_BASE 0x6000C000
#define GPIO_BASE 0x6000D000
#define GPIO_1_BASE (GPIO_BASE)
#define GPIO_2_BASE (GPIO_BASE + 0x100)
#define GPIO_3_BASE (GPIO_BASE + 0x200)
#define GPIO_6_BASE (GPIO_BASE + 0x500)
#define PINMUX_AUX_BASE 0x70003000
#define UART_BASE 0x70006000
#define PMC_BASE 0x7000E400
#define SYSCTR0_BASE 0x7000F000
#define FUSE_BASE 0x7000F800
#define MC_BASE 0x70019000
#define EMC_BASE 0x7001B000
#define MIPI_CAL_BASE 0x700E3000
#define I2S_BASE 0x702D1000

#define _REG(base, off) *(vu32 *)((base) + (off))

#define HOST1X(off) _REG(HOST1X_BASE, off)
#define DISPLAY_A(off) _REG(DISPLAY_A_BASE, off)
#define DSI(off) _REG(DSI_BASE, off)
#define VIC(off) _REG(VIC_BASE, off)
#define TMR(off) _REG(TMR_BASE, off)
#define CLOCK(off) _REG(CLOCK_BASE, off)
#define SYSREG(off) _REG(SYSREG_BASE, off)
#define GPIO_1(off) _REG(GPIO_1_BASE, off)
#define GPIO_2(off) _REG(GPIO_2_BASE, off)
#define GPIO_3(off) _REG(GPIO_3_BASE, off)
#define GPIO_6(off) _REG(GPIO_6_BASE, off)
#define PINMUX_AUX(off) _REG(PINMUX_AUX_BASE, off)
#define PMC(off) _REG(PMC_BASE, off)
#define SYSCTR0(off) _REG(SYSCTR0_BASE, off)
#define FUSE(off) _REG(FUSE_BASE, off)
#define MC(off) _REG(MC_BASE, off)
#define EMC(off) _REG(EMC_BASE, off)
#define MIPI_CAL(off) _REG(MIPI_CAL_BASE, off)
#define I2S(off) _REG(I2S_BASE, off)

/*! System registers. */
#define AHB_ARBITRATION_XBAR_CTRL 0xE0

/*! SYSCTR0 registers. */
#define SYSCTR0_CNTFID0 0x20

#endif
