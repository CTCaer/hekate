#include "clock.h"
#include "t210.h"

static const clock_t _clock_uart[] =  {
	/* UART A */ { 4, 0x10, 0x178, 6, 0, 0 },
	/* UART B */ { 4, 0x10, 0x17C, 7, 0, 0 },
	/* UART C */ { 8, 0x14, 0x1A0, 0x17, 0, 0 },
	/* UART D */ { 0 },
	/* UART E */ { 0 }
};

static const clock_t _clock_i2c[] = {
	/* I2C1 */ { 4, 0x10, 0x124, 0xC, 6, 0 },
	/* I2C2 */ { 0 },
	/* I2C3 */ { 0 },
	/* I2C4 */ { 0 },
	/* I2C5 */ { 8, 0x14, 0x128, 0xF, 6, 0 },
	/* I2C6 */ { 0 }
};

void clock_enable(const clock_t *clk)
{
	//Put clock into reset.
	CLOCK(clk->reset) = CLOCK(clk->reset) & ~(1 << clk->index) | (1 << clk->index);
	//Disable.
	CLOCK(clk->enable) &= ~(1 << clk->index);
	//Configure clock source if required.
	if (clk->source)
		CLOCK(clk->source) = clk->clk_div | (clk->clk_src << 29);
	//Enable.
	CLOCK(clk->enable) = CLOCK(clk->enable) & ~(1 << clk->index) | (1 << clk->index);
	//Take clock off reset.
	CLOCK(clk->reset) &= ~(1 << clk->index);
}

void clock_enable_fuse(u32 enable)
{
	CLOCK(CLK_RST_CONTROLLER_MISC_CLK_ENB) = CLOCK(CLK_RST_CONTROLLER_MISC_CLK_ENB) & 0xEFFFFFFF | ((enable & 1) << 28) & 0x10000000;
}

void clock_enable_uart(u32 idx)
{
	clock_enable(&_clock_uart[idx]);
}

void clock_enable_i2c(u32 idx)
{
	clock_enable(&_clock_i2c[idx]);
}
