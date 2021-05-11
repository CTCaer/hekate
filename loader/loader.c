/*
 * Copyright (c) 2019 CTCaer
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
#include <stdlib.h>

#include "payload_00.h"
#include "payload_01.h"

#include <memory_map.h>
#include <libs/compr/lz.h>
#include <soc/clock.h>
#include <soc/t210.h>

// 0x4003D000: Safe for panic preserving, 0x40038000: Safe for debugging needs.
#define IPL_RELOC_TOP  0x40038000
#define IPL_PATCHED_RELOC_SZ 0x94

boot_cfg_t __attribute__((section ("._boot_cfg"))) b_cfg;
const volatile ipl_ver_meta_t __attribute__((section ("._ipl_version"))) ipl_ver = {
	.magic = BL_MAGIC,
	.version = (BL_VER_MJ + '0') | ((BL_VER_MN + '0') << 8) | ((BL_VER_HF + '0') << 16),
	.rsvd0 = 0,
	.rsvd1 = 0
};

const volatile char __attribute__((section ("._octopus"))) octopus[] =
	"\n"
	"                         ___\n"
	"                      .-'   `'.\n"
	"                     /         \\\n"
	"                     |         ;\n"
	"                     |         |           ___.--,\n"
	"            _.._     |0) = (0) |    _.---'`__.-( (_.\n"
	"     __.--'`_.. '.__.\\    '--. \\_.-' ,.--'`     `\"\"`\n"
	"    ( ,.--'`   ',__ /./;   ;, '.__.'`    __\n"
	"    _`) )  .---.__.' / |   |\\   \\__..--\"\"  \"\"\"--.,_\n"
	"   `---' .'.''-._.-'`_./  /\\ '.  \\ _.--''````'''--._`-.__.'\n"
	"         | |  .' _.-' |  |  \\  \\  '.               `----`\n"
	"          \\ \\/ .'     \\  \\   '. '-._)\n"
	"           \\/ /        \\  \\    `=.__`'-.\n"
	"           / /\\         `) )    / / `\"\".`\\\n"
	"     , _.-'.'\\ \\        / /    ( (     / /\n"
	"      `--'`   ) )    .-'.'      '.'.  | (\n"
	"             (/`    ( (`          ) )  '-;   [switchbrew]\n";

void loader_main()
{
	// Preliminary BPMP clocks init.
	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 0x10;          // Set HCLK div to 2 and PCLK div to 1.
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SYS) = 0;              // Set SCLK div to 1.
	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20004444;  // Set clk source to Run and PLLP_OUT2 (204MHz).
	CLOCK(CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER) = 0x80000000; // Enable SUPER_SDIV to 1.
	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 2;             // Set HCLK div to 1 and PCLK div to 3.
	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20003333;  // Set SCLK to PLLP_OUT (408MHz).

	// Get Loader and Payload size.
	u32 payload_size = sizeof(payload_00) + sizeof(payload_01);             // Actual payload size.
	payload_size += (u32)payload_01 - (u32)payload_00 - sizeof(payload_00); // Add array alignment.
	u32 *payload_addr = (u32 *)payload_00;

	// Relocate payload to a safer place.
	u32 bytes = ALIGN(payload_size, 4) >> 2;
	u32 *addr = payload_addr + bytes - 1;
	u32 *dst = (u32 *)(IPL_RELOC_TOP - 4);
	while (bytes)
	{
		*dst = *addr;
		dst--;
		addr--;
		bytes--;
	}

	// Set source address of the first part.
	u8 *src_addr = (void *)(IPL_RELOC_TOP - ALIGN(payload_size, 4));
	// Uncompress first part.
	u32 dst_pos = LZ_Uncompress((const u8 *)src_addr, (u8*)IPL_LOAD_ADDR, sizeof(payload_00));

	// Set source address of the second part. Includes array alignment.
	src_addr += (u32)payload_01 - (u32)payload_00;
	// Uncompress second part.
	LZ_Uncompress((const u8 *)src_addr, (u8*)IPL_LOAD_ADDR + dst_pos, sizeof(payload_01));

	// Copy over boot configuration storage.
	memcpy((u8 *)(IPL_LOAD_ADDR + IPL_PATCHED_RELOC_SZ), &b_cfg, sizeof(boot_cfg_t));

	// Chainload into uncompressed payload.
	void (*ipl_ptr)() = (void *)IPL_LOAD_ADDR;
	(*ipl_ptr)();

	// Halt if we managed to get out of execution.
	while (true)
		;
}
