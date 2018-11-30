/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
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

#include "../sec/tsec.h"
#include "../sec/tsec_t210.h"
#include "../sec/se_t210.h"
#include "../soc/clock.h"
#include "../soc/smmu.h"
#include "../soc/t210.h"
#include "../mem/heap.h"
#include "../mem/mc.h"
#include "../utils/util.h"
#include "../hos/hos.h"

/* #include "../gfx/gfx.h"
extern gfx_con_t gfx_con; */

static int _tsec_dma_wait_idle()
{
	u32 timeout = get_tmr_ms() + 10000;

	while (!(TSEC(0x1118) & 2))
		if (get_tmr_ms() > timeout)
			return 0;

	return 1;
}

static int _tsec_dma_pa_to_internal_100(int not_imem, int i_offset, int pa_offset)
{
	u32 cmd;

	if (not_imem)
		cmd = 0x600; // DMA 0x100 bytes
	else
		cmd = 0x10; // dma imem

	TSEC(0x1114) = i_offset; // tsec_dmatrfmoffs_r
	TSEC(0x111C) = pa_offset; // tsec_dmatrffboffs_r
	TSEC(0x1118) = cmd; // tsec_dmatrfcmd_r

	return _tsec_dma_wait_idle();
}

int tsec_query(u8 *tsec_keys, u8 kb, tsec_ctxt_t *tsec_ctxt)
{
	int res = 0;
	u8 *fwbuf = NULL;
	u32 *pdir, *car, *fuse, *pmc, *flowctrl, *se, *mc, *iram, *evec;

	//Enable clocks.
	clock_enable_host1x();
	clock_enable_tsec();
	clock_enable_sor_safe();
	clock_enable_sor0();
	clock_enable_sor1();
	clock_enable_kfuse();

	//Configure Falcon.
	TSEC(0x110C) = 0; // tsec_dmactl_r
	TSEC(0x1010) = 0xFFF2; // tsec_irqmset_r
	TSEC(0x101C) = 0xFFF0; // tsec_irqdest_r
	TSEC(0x1048) = 3; // tsec_itfen_r
	if (!_tsec_dma_wait_idle())
	{
		res = -1;
		goto out;
	}

	//Load firmware or emulate memio environment for newer TSEC fw.
	if (kb <= KB_FIRMWARE_VERSION_600)
	{
		fwbuf = (u8 *)malloc(0x2000);
		u8 *fwbuf_aligned = (u8 *)ALIGN((u32)fwbuf + 0x1000, 0x100);
		memcpy(fwbuf_aligned, tsec_ctxt->fw, tsec_ctxt->size);
		TSEC(TSEC_DMATRFBASE) = (u32)fwbuf_aligned >> 8;
	}
	else
		TSEC(TSEC_DMATRFBASE) = (u32)tsec_ctxt->fw >> 8;

	for (u32 addr = 0; addr < tsec_ctxt->size; addr += 0x100)
	{
		if (!_tsec_dma_pa_to_internal_100(false, addr, addr))
		{
			res = -2;
			goto out_free;
		}
	}

	if (kb >= KB_FIRMWARE_VERSION_620)
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
		smmu_map(pdir, 0x40010000, (u32)iram, 0x30, _READABLE | _WRITABLE | _NONSECURE);

		// Exception vectors
		evec = page_alloc(1);
		smmu_map(pdir, EXCP_VEC_BASE, (u32)evec, 1, _READABLE | _WRITABLE | _NONSECURE);
	}

	//Execute firmware.
	HOST1X(0x3300) = 0x34C2E1DA;
	TSEC(0x1044) = 0;
	TSEC(0x1040) = rev;
	TSEC(0x1104) = 0; // tsec_bootvec_r
	TSEC(0x1100) = 2; // tsec_cpuctl_r

	if (kb <= KB_FIRMWARE_VERSION_600)
	{
		if (!_tsec_dma_wait_idle())
		{
			res = -3;
			goto out_free;
		}
		u32 timeout = get_tmr_ms() + 2000;
		while (!TSEC(0x1044))
			if (get_tmr_ms() > timeout)
			{
				res = -4;
				goto out_free;
			}
		if (TSEC(0x1044) != 0xB0B0B0B0)
		{
			res = -5;
			goto out_free;
		}

		//Fetch result.
		HOST1X(0x3300) = 0;
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
	else
	{
		u32 start = get_tmr_us();
		u32 k = se[SE_KEYTABLE_DATA0_REG_OFFSET / 4];
		u32 key[16] = {0};
		u32 kidx = 0;

		while (memcmp((u8 *)(iram + ((tsec_ctxt->pkg11_off + 0x20) / 4)), "PK11", 4))
		{
			smmu_flush_all();
			if (k == se[SE_KEYTABLE_DATA0_REG_OFFSET / 4])
				continue;
			k = se[SE_KEYTABLE_DATA0_REG_OFFSET / 4];
			key[kidx++] = k;

		}

		if (kidx != 8)
		{
			res = -6;
			smmu_deinit_for_tsec();

			goto out;
		}

		memcpy(tsec_keys, &key, 0x20);
		memcpy(tsec_ctxt->pkg1, iram, 0x30000);
		
		smmu_deinit_for_tsec();

		// for (int i = 0; i < kidx; i++)
		// 	gfx_printf(&gfx_con, "key %08X\n", key[i]);

		// gfx_printf(&gfx_con, "cpuctl (%08X) mbox (%08X)\n", TSEC(TSEC_CPUCTL), TSEC(TSEC_STATUS));

		// u32 errst = MC(MC_ERR_STATUS);
		// gfx_printf(&gfx_con, " MC %08X %08X %08X\n", MC(MC_INTSTATUS), errst, MC(MC_ERR_ADR));
		// gfx_printf(&gfx_con, " type: %02X\n", errst >> 28);
		// gfx_printf(&gfx_con, " smmu: %02X\n", (errst >> 25) & 3);
		// gfx_printf(&gfx_con, " dir:  %s\n", (errst >> 16) & 1 ? "W" : "R");
		// gfx_printf(&gfx_con, " cid:  %02x\n", errst & 0xFF);
	}

out_free:;
	free(fwbuf);

out:;

	//Disable clocks.
	clock_disable_kfuse();
	clock_disable_sor1();
	clock_disable_sor0();
	clock_disable_sor_safe();
	clock_disable_tsec();
	clock_disable_host1x();

	return res;
}
