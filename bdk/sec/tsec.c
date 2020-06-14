/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 CTCaer
 * Copyright (c) 2018 balika011
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

#include "tsec.h"
#include "tsec_t210.h"
#include <sec/se_t210.h>
#include <soc/bpmp.h>
#include <soc/clock.h>
#include <soc/kfuse.h>
#include <soc/t210.h>
#include <mem/heap.h>
#include <mem/mc.h>
#include <mem/smmu.h>
#include <utils/util.h>

// #include <gfx_utils.h>

#define PKG11_MAGIC 0x31314B50
#define KB_TSEC_FW_EMU_COMPAT 6 // KB ID for HOS 6.2.0.

static int _tsec_dma_wait_idle()
{
	u32 timeout = get_tmr_ms() + 10000;

	while (!(TSEC(TSEC_DMATRFCMD) & TSEC_DMATRFCMD_IDLE))
		if (get_tmr_ms() > timeout)
			return 0;

	return 1;
}

static int _tsec_dma_pa_to_internal_100(int not_imem, int i_offset, int pa_offset)
{
	u32 cmd;

	if (not_imem)
		cmd = TSEC_DMATRFCMD_SIZE_256B; // DMA 256 bytes
	else
		cmd = TSEC_DMATRFCMD_IMEM;      // DMA IMEM (Instruction memmory)

	TSEC(TSEC_DMATRFMOFFS) = i_offset;
	TSEC(TSEC_DMATRFFBOFFS) = pa_offset;
	TSEC(TSEC_DMATRFCMD) = cmd;

	return _tsec_dma_wait_idle();
}

int tsec_query(u8 *tsec_keys, u8 kb, tsec_ctxt_t *tsec_ctxt)
{
	int res = 0;
	u8 *fwbuf = NULL;
	u32 *pdir, *car, *fuse, *pmc, *flowctrl, *se, *mc, *iram, *evec;
	u32 *pkg11_magic_off;

	bpmp_mmu_disable();
	bpmp_clk_rate_set(BPMP_CLK_NORMAL);

	// Enable clocks.
	clock_enable_host1x();
	usleep(2);
	clock_enable_tsec();
	clock_enable_sor_safe();
	clock_enable_sor0();
	clock_enable_sor1();
	clock_enable_kfuse();

	kfuse_wait_ready();

	// Configure Falcon.
	TSEC(TSEC_DMACTL) = 0;
	TSEC(TSEC_IRQMSET) =
		TSEC_IRQMSET_EXT(0xFF) |
		TSEC_IRQMSET_WDTMR |
		TSEC_IRQMSET_HALT |
		TSEC_IRQMSET_EXTERR |
		TSEC_IRQMSET_SWGEN0 |
		TSEC_IRQMSET_SWGEN1;
	TSEC(TSEC_IRQDEST) =
		TSEC_IRQDEST_EXT(0xFF) |
		TSEC_IRQDEST_HALT |
		TSEC_IRQDEST_EXTERR |
		TSEC_IRQDEST_SWGEN0 |
		TSEC_IRQDEST_SWGEN1;
	TSEC(TSEC_ITFEN) = TSEC_ITFEN_CTXEN | TSEC_ITFEN_MTHDEN;
	if (!_tsec_dma_wait_idle())
	{
		res = -1;
		goto out;
	}

	// Load firmware or emulate memio environment for newer TSEC fw.
	if (kb == KB_TSEC_FW_EMU_COMPAT)
		TSEC(TSEC_DMATRFBASE) = (u32)tsec_ctxt->fw >> 8;
	else
	{
		fwbuf = (u8 *)malloc(0x4000);
		u8 *fwbuf_aligned = (u8 *)ALIGN((u32)fwbuf, 0x100);
		memcpy(fwbuf_aligned, tsec_ctxt->fw, tsec_ctxt->size);
		TSEC(TSEC_DMATRFBASE) = (u32)fwbuf_aligned >> 8;
	}

	for (u32 addr = 0; addr < tsec_ctxt->size; addr += 0x100)
	{
		if (!_tsec_dma_pa_to_internal_100(false, addr, addr))
		{
			res = -2;
			goto out_free;
		}
	}

	if (kb == KB_TSEC_FW_EMU_COMPAT)
	{
		// Init SMMU translation for TSEC.
		pdir = smmu_init_for_tsec();
		smmu_init(tsec_ctxt->secmon_base);
		// Enable SMMU
		if (!smmu_is_used())
			smmu_enable();

		// Clock reset controller.
		car = page_alloc(1);
		memcpy(car, (void *)CLOCK_BASE, 0x1000);
		car[CLK_RST_CONTROLLER_CLK_SOURCE_TSEC / 4] = 2;
		smmu_map(pdir, CLOCK_BASE, (u32)car, 1, _WRITABLE | _READABLE | _NONSECURE);

		// Fuse driver.
		fuse = page_alloc(1);
		memcpy((void *)&fuse[0x800/4], (void *)FUSE_BASE, 0x400);
		fuse[0x82C / 4] = 0;
		fuse[0x9E0 / 4] = (1 << (kb + 2)) - 1;
		fuse[0x9E4 / 4] = (1 << (kb + 2)) - 1;
		smmu_map(pdir, (FUSE_BASE - 0x800), (u32)fuse, 1, _READABLE | _NONSECURE);

		// Power management controller.
		pmc = page_alloc(1);
		smmu_map(pdir, RTC_BASE, (u32)pmc, 1, _READABLE | _NONSECURE);

		// Flow control.
		flowctrl = page_alloc(1);
		smmu_map(pdir, FLOW_CTLR_BASE, (u32)flowctrl, 1, _WRITABLE | _NONSECURE);

		// Security engine.
		se = page_alloc(1);
		memcpy(se, (void *)SE_BASE, 0x1000);
		smmu_map(pdir, SE_BASE, (u32)se, 1, _READABLE | _WRITABLE | _NONSECURE);

		// Memory controller.
		mc = page_alloc(1);
		memcpy(mc, (void *)MC_BASE, 0x1000);
		mc[MC_IRAM_BOM / 4] = 0;
		mc[MC_IRAM_TOM / 4] = 0x80000000;
		smmu_map(pdir, MC_BASE, (u32)mc, 1, _READABLE | _NONSECURE);

		// IRAM
		iram = page_alloc(0x30);
		memcpy(iram, tsec_ctxt->pkg1, 0x30000);
		// PKG1.1 magic offset.
		pkg11_magic_off = (u32 *)(iram + ((tsec_ctxt->pkg11_off + 0x20) / 4));
		smmu_map(pdir, 0x40010000, (u32)iram, 0x30, _READABLE | _WRITABLE | _NONSECURE);

		// Exception vectors
		evec = page_alloc(1);
		smmu_map(pdir, EXCP_VEC_BASE, (u32)evec, 1, _READABLE | _WRITABLE | _NONSECURE);
	}

	// Execute firmware.
	HOST1X(HOST1X_CH0_SYNC_SYNCPT_160) = 0x34C2E1DA;
	TSEC(TSEC_STATUS) = 0;
	TSEC(TSEC_BOOTKEYVER) = 1; // HOS uses key version 1.
	TSEC(TSEC_BOOTVEC) = 0;
	TSEC(TSEC_CPUCTL) = TSEC_CPUCTL_STARTCPU;

	if (kb == KB_TSEC_FW_EMU_COMPAT)
	{
		u32 start = get_tmr_us();
		u32 k = se[SE_KEYTABLE_DATA0_REG_OFFSET / 4];
		u32 key[16] = {0};
		u32 kidx = 0;

		while (*pkg11_magic_off != PKG11_MAGIC)
		{
			smmu_flush_all();

			if (k != se[SE_KEYTABLE_DATA0_REG_OFFSET / 4])
			{
				k = se[SE_KEYTABLE_DATA0_REG_OFFSET / 4];
				key[kidx++] = k;
			}

			// Failsafe.
			if ((u32)get_tmr_us() - start > 125000)
				break;
		}

		if (kidx != 8)
		{
			res = -6;
			smmu_deinit_for_tsec();

			goto out_free;
		}

		// Give some extra time to make sure PKG1.1 is decrypted.
		msleep(50);

		memcpy(tsec_keys, &key, 0x20);
		memcpy(tsec_ctxt->pkg1, iram, 0x30000);

		smmu_deinit_for_tsec();

		// for (int i = 0; i < kidx; i++)
		// 	gfx_printf("key %08X\n", key[i]);

		// gfx_printf("cpuctl (%08X) mbox (%08X)\n", TSEC(TSEC_CPUCTL), TSEC(TSEC_STATUS));

		// u32 errst = MC(MC_ERR_STATUS);
		// gfx_printf(" MC %08X %08X %08X\n", MC(MC_INTSTATUS), errst, MC(MC_ERR_ADR));
		// gfx_printf(" type: %02X\n", errst >> 28);
		// gfx_printf(" smmu: %02X\n", (errst >> 25) & 3);
		// gfx_printf(" dir:  %s\n", (errst >> 16) & 1 ? "W" : "R");
		// gfx_printf(" cid:  %02x\n", errst & 0xFF);
	}
	else
	{
		if (!_tsec_dma_wait_idle())
		{
			res = -3;
			goto out_free;
		}
		u32 timeout = get_tmr_ms() + 2000;
		while (!TSEC(TSEC_STATUS))
			if (get_tmr_ms() > timeout)
			{
				res = -4;
				goto out_free;
			}
		if (TSEC(TSEC_STATUS) != 0xB0B0B0B0)
		{
			res = -5;
			goto out_free;
		}

		// Fetch result.
		HOST1X(HOST1X_CH0_SYNC_SYNCPT_160) = 0;
		u32 buf[4];
		buf[0] = SOR1(SOR_NV_PDISP_SOR_DP_HDCP_BKSV_LSB);
		buf[1] = SOR1(SOR_NV_PDISP_SOR_TMDS_HDCP_BKSV_LSB);
		buf[2] = SOR1(SOR_NV_PDISP_SOR_TMDS_HDCP_CN_MSB);
		buf[3] = SOR1(SOR_NV_PDISP_SOR_TMDS_HDCP_CN_LSB);
		SOR1(SOR_NV_PDISP_SOR_DP_HDCP_BKSV_LSB) = 0;
		SOR1(SOR_NV_PDISP_SOR_TMDS_HDCP_BKSV_LSB) = 0;
		SOR1(SOR_NV_PDISP_SOR_TMDS_HDCP_CN_MSB) = 0;
		SOR1(SOR_NV_PDISP_SOR_TMDS_HDCP_CN_LSB) = 0;

		memcpy(tsec_keys, &buf, 0x10);
	}

out_free:;
	free(fwbuf);

out:;

	// Disable clocks.
	clock_disable_kfuse();
	clock_disable_sor1();
	clock_disable_sor0();
	clock_disable_sor_safe();
	clock_disable_tsec();
	bpmp_mmu_enable();
	bpmp_clk_rate_set(BPMP_CLK_DEFAULT_BOOST);

	return res;
}
