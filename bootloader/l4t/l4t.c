/*
 * L4T Loader for Tegra X1
 *
 * Copyright (c) 2020-2025 CTCaer
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

#include <bdk.h>
#include <soc/pmc_lp0_t210.h>

#include "../hos/hos.h"
#include "../hos/pkg1.h"
#include "l4t.h"
#include "l4t_config.inl"

/*
 * API Revision info
 *
 * 0: Base.
 * 1: SDMMC1 LA programming for SDMMC1 UHS DDR200.
 * 2: Arachne Register Cell v1.
 * 3: Arachne Register Cell v2. PTSA Rework support.
 * 4: Arachne Register Cell v3. DRAM OPT and DDR200 changes.
 * 5: Arachne Register Cell v4. DRAM FREQ and DDR200 changes.
 * 6: Arachne Register Cell v5. Signal quality and performance changes. TZ param changes.
 * 7: Arachne Register Cell v6. Decouple of rd/wr latencies.
 */

#define L4T_LOADER_API_REV 7
#define L4T_FIRMWARE_REV   0x37524556 // REV7.

#ifdef DEBUG_UART_PORT
 #include <soc/uart.h>
 #define UPRINTF(...) uart_printf(__VA_ARGS__)
#else
 #define UPRINTF(...)
#endif

#if CARVEOUT_NVDEC_TSEC_ENABLE && CARVEOUT_SECFW_ENABLE
#error "NVDEC and SECFW carveouts can't be used together!"
#endif

#if CARVEOUT_NVDEC_TSEC_ENABLE
 #define TZDRAM_SIZE_CFG  SZ_8M
#else
 #define TZDRAM_SIZE_CFG  SZ_1M
#endif

// TZDRAM addresses and sizes.
#define TZDRAM_SIZE       TZDRAM_SIZE_CFG                  // Secure Element.
#define TZDRAM_BASE       (0xFFFFFFFF - TZDRAM_SIZE + 1)   // 0xFFF00000 or 0xFF800000.
#define TZDRAM_COLD_ENTRY (TZDRAM_BASE)
#define TZDRAM_WARM_ENTRY (TZDRAM_BASE + 0x200)
#define TZ_PARAM_SIZE     SZ_4K
#define TZ_PARAM_BASE     (0xFFFFFFFF - TZ_PARAM_SIZE + 1) // 0xFFFFF000.

// Carveout sizes.
#define CARVEOUT_NVDEC_SIZE  SZ_1M
#define CARVEOUT_TSEC_SIZE   SZ_1M
#define CARVEOUT_SECFW_SIZE  SZ_1M
#define CARVEOUT_GPUFW_SIZE  SZ_256K
#if CARVEOUT_NVDEC_TSEC_ENABLE
 #define CARVEOUT_GPUWPR_SIZE CARVEOUT_GPUWPR_SIZE_CFG
#else
 #define CARVEOUT_GPUWPR_SIZE (SZ_512K + SZ_256K)
#endif

#define SC7ENTRY_HDR_SIZE 0x400

// Always start 1MB below TZDRAM for Secure Firmware or NVDEC.
#define GEN_CARVEOUT_TOP       (TZDRAM_BASE - SZ_1M)

// NVDEC and SECFW bases.
#define NVDFW_BASE             GEN_CARVEOUT_TOP // 0xFF700000.
#define SECFW_BASE             GEN_CARVEOUT_TOP // 0xFFE00000.

// Secure Elements addresses for T210.
#define SC7ENTRY_HDR_BASE      (SECFW_BASE + 0)
#define SC7ENTRY_BASE          (SECFW_BASE + SC7ENTRY_HDR_SIZE) // After header.
#define SC7EXIT_BASE           (SECFW_BASE + SZ_64K)            //  64KB after SECFW_BASE.
#define R2P_PAYLOAD_BASE       (SECFW_BASE + SZ_256K)           // 256KB after SECFW_BASE.
#define MTCTABLE_BASE          (SECFW_BASE + SZ_512K)           // 512KB after SECFW_BASE.
#define BPMPFW_BASE            (SECFW_BASE + SZ_512K + SZ_256K) // 768KB after SECFW_BASE.
#define BPMPFW_ENTRYPOINT      (BPMPFW_BASE + 0x100)            // Used internally also.

// Secure Elements addresses for T210B01.
#define BPMPFW_B01_BASE       (SECFW_BASE)                         // !! DTS carveout-start must match !!
#define BPMPFW_B01_ENTRYPOINT (BPMPFW_B01_BASE + 0x40)             // Used internally also.
#define BPMPFW_B01_HEAP_BASE  (BPMPFW_B01_BASE + SZ_256K - SZ_1K)  // 255KB after BPMPFW_B01_BASE.
#define BPMPFW_B01_EDTB_BASE  (BPMPFW_B01_BASE + SZ_1M - 0)        // Top BPMPFW carveout minus EMC DTB size.
#define BPMPFW_B01_ADTB_BASE  (BPMPFW_B01_BASE + 0x26008)          // Attached BPMP-FW DTB address.
#define SC7EXIT_B01_BASE      (BPMPFW_B01_HEAP_BASE - SZ_4K)       // 4KB before BPMP heap.

// BPMP-FW defines. Offsets are 0xD8 below real main binary.
#define BPMPFW_B01_DTB_ADDR   (BPMPFW_B01_BASE + 0x14)    // u32. DTB address if not attached.
#define BPMPFW_B01_CC_INIT_OP (BPMPFW_B01_BASE + 0x17324) // u8.  Initial table training OP. 0: OP_SWITCH, 1: OP_TRAIN, 2: OP_TRAIN_SWITCH. Default: OP_TRAIN.
#define BPMPFW_B01_LOGLEVEL   (BPMPFW_B01_BASE + 0x2547C) // u32. Log level. Default 3.
#define BPMPFW_B01_LOGLEVEL   (BPMPFW_B01_BASE + 0x2547C) // u32. Log level. Default 3.
#define BPMPFW_B01_CC_PT_TIME (BPMPFW_B01_BASE + 0x25644) // u32. Periodic training period (in ms). Default 100 ms.
#define BPMPFW_B01_CC_DEBUG   (BPMPFW_B01_BASE + 0x257F8) // u32. EMC Clock Change debug mask. Default: 0x50000101.

// BPMP-FW attached DTB defines. Can be generalized.
#define BPMPFW_B01_DTB_EMC_ENTRIES           4
#define BPMPFW_B01_DTB_SERIAL_PORT_VAL       (BPMPFW_B01_ADTB_BASE + 0x5B) // u8. DTB UART port offset. 0: Disabled.
#define BPMPFW_B01_DTB_SET_SERIAL_PORT(port) (*(u8 *)BPMPFW_B01_DTB_SERIAL_PORT_VAL = port)
#define BPMPFW_B01_DTB_EMC_TBL_OFF           (BPMPFW_B01_ADTB_BASE + 0xA0)
#define BPMPFW_B01_DTB_EMC_TBL_SZ            0x1120
#define BPMPFW_B01_DTB_EMC_NAME_VAL          0xA
#define BPMPFW_B01_DTB_EMC_ENABLE_OFF        0x20
#define BPMPFW_B01_DTB_EMC_VALUES_OFF        0x4C
#define BPMPFW_B01_DTB_EMC_FREQ_VAL          0x8C
#define BPMPFW_B01_DTB_EMC_OPT_VAL           0xEDC
#define BPMPFW_B01_DTB_EMC_TBL_START(idx)             (BPMPFW_B01_DTB_EMC_TBL_OFF + BPMPFW_B01_DTB_EMC_TBL_SZ * (idx))
#define BPMPFW_B01_DTB_EMC_TBL_SET_VAL(idx, off, val) (*(u32 *)(BPMPFW_B01_DTB_EMC_TBL_START(idx) + (off)) = (val))
#define BPMPFW_B01_DTB_EMC_TBL_SET_FREQ(idx, freq)    (*(u32 *)(BPMPFW_B01_DTB_EMC_TBL_START(idx) + BPMPFW_B01_DTB_EMC_FREQ_VAL) = (freq))
#define BPMPFW_B01_DTB_EMC_TBL_SET_OPTC(idx, opt)     (*(u32 *)(BPMPFW_B01_DTB_EMC_TBL_START(idx) + BPMPFW_B01_DTB_EMC_OPT_VAL) = (opt))
#define BPMPFW_B01_DTB_EMC_TBL_SET_NAME(idx, name)    (strcpy((char *)(BPMPFW_B01_DTB_EMC_TBL_START(idx) + BPMPFW_B01_DTB_EMC_NAME_VAL), (name)))
#define BPMPFW_B01_DTB_EMC_TBL_ENABLE(idx)            (*(char *)(BPMPFW_B01_DTB_EMC_TBL_START(idx) + BPMPFW_B01_DTB_EMC_ENABLE_OFF) = 'n')
#define BPMPFW_B01_DTB_EMC_TBL_OFFSET(idx)            ((void *)(BPMPFW_B01_DTB_EMC_TBL_START(idx) + BPMPFW_B01_DTB_EMC_VALUES_OFF))

// MTC table defines for T210B01.
#define BPMPFW_B01_MTC_TABLE_BASE            0xA0000000
#define BPMPFW_B01_MTC_FREQ_TABLE_SIZE       4300
#define BPMPFW_B01_MTC_TABLE_SIZE            (BPMPFW_B01_MTC_FREQ_TABLE_SIZE * 3)
#define BPMPFW_B01_MTC_TABLE(idx)                     (BPMPFW_B01_MTC_TABLE_BASE + BPMPFW_B01_MTC_TABLE_SIZE * (idx))
#define BPMPFW_B01_MTC_TABLE_OFFSET(idx, fidx)        ((void *)(BPMPFW_B01_MTC_TABLE(idx) + BPMPFW_B01_MTC_FREQ_TABLE_SIZE * (fidx)))

// BL31 Enable IRAM based config.
#define BL31_IRAM_PARAMS           0x4D415249 // "IRAM".
#define BL31_EXTRA_FEATURES_ENABLE 0x52545845 // "EXTR".

// BL31 Flags.
#define FLAGS_PMC_NON_SECURE          BIT(0)
#define FLAGS_SC7_NO_BASE_RESTRICTION BIT(1)

// BL31 config.
#define PARAM_EP            1
#define PARAM_BL31          3
#define PARAM_EP_SECURE	    0
#define PARAM_EP_NON_SECURE	1
#define VERSION_1           1
#define SPSR_EL2T           BIT(3)

// BL33 config.
#define BL33_LOAD_BASE        0xAA000000                 // DTB is loaded at 0xA8000000, so 32MB above.
#define BL33_ENV_BASE         (BL33_LOAD_BASE - SZ_256K) // Before BL33_LOAD_BASE.
#define BL33_ENV_MAGIC_OFFSET (BL33_ENV_BASE - 4)
#define BL33_ENV_MAGIC        0x33334C42
#define BL33_DTB_OFFSET       (BL33_LOAD_BASE + 0x10)    // After code end.
#define BL33_DTB_BASE         (BL33_LOAD_BASE + *(u32 *)BL33_DTB_OFFSET)
#define BL33_DTB_UART_STATUS  0x1C94
#define BL33_DTB_UART_STS_OF  0x12C
#define BL33_DTB_STDOUT_PATH  0x3F34
#define BL33_DTB_STDERR_PATH  0x3F54
#define BL33_DTB_SET_UART_STATUS(port) (strcpy((char *)(BL33_DTB_BASE + BL33_DTB_UART_STATUS + (port - 1) * BL33_DTB_UART_STS_OF), "okay"))
#define BL33_DTB_SET_STDOUT_PATH(path) (strcpy((char *)(BL33_DTB_BASE + BL33_DTB_STDOUT_PATH), (path)))
#define BL33_DTB_SET_STDERR_PATH(path) (strcpy((char *)(BL33_DTB_BASE + BL33_DTB_STDERR_PATH), (path)))

// Misc.
#define DTB_MAGIC             0xEDFE0DD0 // D00DFEED.
#define FALCON_DMA_PAGE_SIZE  0x100
#define SOC_ID_T210           0x210
#define SOC_ID_T210B01        0x214
#define SKU_NX                0x83
#define HDCP22                2

typedef struct _tsec_init_t {
	u32 sku;
	u32 hdcp;
	u32 soc;
} tsec_init_t;

typedef struct _param_header {
	u8  type;    // type of the structure.
	u8  version; // version of this structure.
	u16 size;    // size of this structure in bytes.
	u32 attr;    // attributes: unused bits SBZ.
} param_header_t;

typedef struct _aapcs64_params {
	u64 x0;
	u64 x1;
	u64 x2;
	u64 x3;
	u64 x4;
	u64 x5;
	u64 x6;
	u64 x7;
} aapcs64_params_t;

typedef struct _entry_point_info {
	param_header_t hdr;
	u64 pc;
	u32 spsr;
	u32 align0; // Alignment to u64 for the above member.
	aapcs64_params_t args;
} entry_point_info_t;

typedef struct _image_info {
	param_header_t hdr;
	u64 image_base; // physical address of base of image.
	u32 image_size; // bytes read from image file.
	u32 image_max_size;
} image_info_t;

typedef struct _bl_v1_params {
	param_header_t hdr;
	u64 bl31_image_info;
	u64 bl32_ep_info;
	u64 bl32_image_info;
	u64 bl33_ep_info;
	u64 bl33_image_info;
} bl31_v1_params_t;

typedef struct _plat_params_from_bl2 {
	// TZDRAM.
	u64 tzdram_size;
	u64 tzdram_base;

	s32 uart_id;                  // UART port ID.
	s32 l2_ecc_parity_prot_dis;   // L2 ECC parity protection disable flag.
	u64 boot_profiler_shmem_base; // SHMEM base address for storing the boot logs.

	// SC7-Entry firmware.
	u64 sc7entry_fw_size;
	u64 sc7entry_fw_base;

	s32 enable_ccplex_lock_step;  // Enable dual execution.

	// Enable below features.
	s32 enable_extra_features;

	// MTC table.
	u64 emc_table_size;
	u64 emc_table_base;

	// Initial R2P payload.
	u64 r2p_payload_size;
	u64 r2p_payload_base;

	u64 flags;                    // Platform flags.
} bl31_plat_params_from_bl2_t;

typedef struct _l4t_fw_t
{
	u32   addr;
	char *name;
} l4t_fw_t;

typedef struct _l4t_ctxt_t
{
	char *path;

	char *ram_oc_txt;
	int   ram_oc_freq;
	int   ram_oc_vdd2;
	int   ram_oc_vddq;
	int   ram_oc_opt;

	u32   serial_port;
	u32   sld_type;

	u32   sc7entry_size;

	emc_table_t *mtc_table;

	bl31_v1_params_t            bl31_v1_params;
	bl31_plat_params_from_bl2_t bl31_plat_params;
	entry_point_info_t          bl33_ep_info;
} l4t_ctxt_t;

#define DRAM_VDD2_OC_MIN_VOLTAGE   1050
#define DRAM_VDD2_OC_MAX_VOLTAGE   1175
#define DRAM_VDD2Q_OC_MAX_VOLTAGE  1237
#define DRAM_VDDQ_OC_MIN_VOLTAGE   550
#define DRAM_VDDQ_OC_MAX_VOLTAGE   650
#define DRAM_T210B01_TBL_MAX_FREQ 1600000

#define NA 0 // Default to 0 for incorrect dram ids.

//!TODO: Update on dram config changes.
static const u8 mtc_table_idx_t210b01[] = {
/*	00  01  02  03  04  05  06  07  08  09  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34 */
	NA, NA, NA,  7, NA,  7,  7, NA,  0,  1,  2,  3,  0,  1,  2,  3, NA,  4,  5,  4,  8,  8,  8,  5,  4,  6,  6,  6,  5,  9,  9,  9, 10, 10, 10
};

#undef NA

static const l4t_fw_t l4t_fw[] = {
	{ TZDRAM_BASE,               "bl31.bin"        },
	{ BL33_LOAD_BASE,            "bl33.bin"        },
	{ SC7ENTRY_BASE,             "sc7entry.bin"    },
	{ SC7EXIT_BASE,              "sc7exit.bin"     },
	{ SC7EXIT_B01_BASE,          "sc7exit_b01.bin" }, //!TODO: Update on fuse burns.
	{ BPMPFW_BASE,               "bpmpfw.bin"      },
	{ BPMPFW_B01_BASE,           "bpmpfw_b01.bin"  },
	{ BPMPFW_B01_MTC_TABLE_BASE, "mtc_tbl_b01.bin" },
};

enum {
	BL31_FW            = 0,
	BL33_FW            = 1,
	SC7ENTRY_FW        = 2,
	SC7EXIT_FW         = 3,
	SC7EXIT_B01_FW     = 4,
	BPMPFW_FW          = 5,
	BPMPFW_B01_FW      = 6,
	BPMPFW_B01_MTC_TBL = 7
};

static void _l4t_crit_error(const char *text, bool needs_update)
{
	gfx_con.mute = false;
	gfx_printf("%kL4T Error: %s!%sFailed to launch L4T!\n%k",
		TXT_CLR_ERROR, text, needs_update ? "\nUpdate bootloader folder!\n\n" : "\n\n", TXT_CLR_DEFAULT);
}

char *sd_path;
u32 sd_path_len;
static int _l4t_sd_load(u32 idx)
{
	FIL fp;
	void *load_address = (void *)l4t_fw[idx].addr;

	if (idx == SC7EXIT_B01_FW)
		load_address -= sizeof(u32);

	strcpy(sd_path + sd_path_len, l4t_fw[idx].name);

	if (f_open(&fp, sd_path, FA_READ) != FR_OK)
		return 0;

	u32 size = f_size(&fp);
	if (f_read(&fp, load_address, size, NULL) != FR_OK)
		size = 0;

	f_close(&fp);

	u32 rev = *(u32 *)(load_address + size - sizeof(u32));
	if (idx >= SC7ENTRY_FW && rev != L4T_FIRMWARE_REV)
		return 0;

	return size;
}

static void _l4t_sdram_lp0_save_params(bool t210b01)
{
	struct tegra_pmc_regs *pmc = (struct tegra_pmc_regs *)PMC_BASE;

#define _REG_S(base, off) *(u32 *)((base) + (off))
#define MC_S(off) _REG_S(MC_BASE, off)

#define pack(src, src_bits, dst, dst_bits) { \
		u32 mask = 0xffffffff >> (31 - ((1 ? src_bits) - (0 ? src_bits))); \
		dst &= ~(mask << (0 ? dst_bits)); \
		dst |= ((src >> (0 ? src_bits)) & mask) << (0 ? dst_bits); }

#define s(param, src_bits, pmcreg, dst_bits) \
	pack(MC_S(param), src_bits, pmc->pmcreg, dst_bits)

// 32 bits version of s macro.
#define s32(param, pmcreg) pmc->pmcreg = MC_S(param)

	// Only save changed carveout registers into PMC for SC7 Exit.

	// VPR.
#if CARVEOUT_NVDEC_TSEC_ENABLE
	s32(MC_VIDEO_PROTECT_GPU_OVERRIDE_0, secure_scratch12);
	s(MC_VIDEO_PROTECT_GPU_OVERRIDE_1, 15:0, secure_scratch49, 15:0);
#endif
	s(MC_VIDEO_PROTECT_BOM,     31:20, secure_scratch52, 26:15);
	s(MC_VIDEO_PROTECT_SIZE_MB, 11:0,  secure_scratch53, 11:0);
	if (!t210b01) {
		s(MC_VIDEO_PROTECT_REG_CTRL, 1:0, secure_scratch13, 31:30);
	} else {
		s(MC_VIDEO_PROTECT_REG_CTRL, 1:0, secure_scratch14, 31:30);
	}

	// TZDRAM.
	s(MC_SEC_CARVEOUT_BOM,     31:20, secure_scratch53, 23:12);
	s(MC_SEC_CARVEOUT_SIZE_MB, 11:0,  secure_scratch54, 11:0);
	if (!t210b01) {
		s(MC_SEC_CARVEOUT_REG_CTRL, 0:0, secure_scratch18, 30:30);
	} else {
		s(MC_SEC_CARVEOUT_REG_CTRL, 0:0, secure_scratch19, 29:29);
	}

	// NVDEC or SECFW.
	s(MC_SECURITY_CARVEOUT1_BOM,        31:17, secure_scratch50, 29:15);
	s(MC_SECURITY_CARVEOUT1_SIZE_128KB, 11:0,  secure_scratch57, 11:0);
	s32(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS0,  secure_scratch59);
	s32(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS1,  secure_scratch60);
	s32(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS2,  secure_scratch61);
	s32(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS3,  secure_scratch62);
	if (!t210b01)
	{
		s(MC_SECURITY_CARVEOUT1_CFG0,  2:0,  secure_scratch56, 27:25);
		s(MC_SECURITY_CARVEOUT1_CFG0,  6:3,  secure_scratch41, 28:25);
		s(MC_SECURITY_CARVEOUT1_CFG0, 13:7,  secure_scratch42, 31:25);
		s(MC_SECURITY_CARVEOUT1_CFG0, 17:14, secure_scratch43, 28:25);
		s(MC_SECURITY_CARVEOUT1_CFG0, 21:18, secure_scratch44, 28:25);
		s(MC_SECURITY_CARVEOUT1_CFG0, 25:22, secure_scratch56, 31:28);
		s(MC_SECURITY_CARVEOUT1_CFG0, 26:26, secure_scratch57, 24:24);
	}
	else
	{
		s(MC_SECURITY_CARVEOUT1_CFG0,  1:0,  secure_scratch56, 31:30);
		s(MC_SECURITY_CARVEOUT1_CFG0,  2:2,  secure_scratch57, 24:24);
		s(MC_SECURITY_CARVEOUT1_CFG0,  6:3,  secure_scratch42, 28:25);
		s(MC_SECURITY_CARVEOUT1_CFG0, 10:7,  secure_scratch43, 28:25);
		s(MC_SECURITY_CARVEOUT1_CFG0, 13:11, secure_scratch42, 31:29);
		s(MC_SECURITY_CARVEOUT1_CFG0, 17:14, secure_scratch44, 28:25);
		s(MC_SECURITY_CARVEOUT1_CFG0, 21:18, secure_scratch47, 25:22);
		s(MC_SECURITY_CARVEOUT1_CFG0, 26:22, secure_scratch57, 29:25);
	}

	// GPU FW.
	s(MC_SECURITY_CARVEOUT2_BOM,        31:17, secure_scratch51, 29:15);
	s(MC_SECURITY_CARVEOUT2_SIZE_128KB, 11:0,  secure_scratch56, 23:12);
	if (!t210b01)
	{
		s(MC_SECURITY_CARVEOUT2_CFG0,  2:0,  secure_scratch55, 27:25);
		s(MC_SECURITY_CARVEOUT2_CFG0,  6:3,  secure_scratch19, 31:28);
		s(MC_SECURITY_CARVEOUT2_CFG0, 10:7,  secure_scratch20, 31:28);
		s(MC_SECURITY_CARVEOUT2_CFG0, 13:11, secure_scratch41, 31:29);
		s(MC_SECURITY_CARVEOUT2_CFG0, 17:14, secure_scratch39, 30:27);
		s(MC_SECURITY_CARVEOUT2_CFG0, 21:18, secure_scratch40, 30:27);
		s(MC_SECURITY_CARVEOUT2_CFG0, 25:22, secure_scratch55, 31:28);
		s(MC_SECURITY_CARVEOUT2_CFG0, 26:26, secure_scratch56, 24:24);
	}
	else
	{
		s(MC_SECURITY_CARVEOUT2_CFG0,  1:0,  secure_scratch55, 31:30);
		s(MC_SECURITY_CARVEOUT2_CFG0,  2:2,  secure_scratch56, 24:24);
		s(MC_SECURITY_CARVEOUT2_CFG0,  6:3,  secure_scratch20, 31:28);
		s(MC_SECURITY_CARVEOUT2_CFG0, 10:7,  secure_scratch39, 30:27);
		s(MC_SECURITY_CARVEOUT2_CFG0, 13:11, secure_scratch41, 31:29);
		s(MC_SECURITY_CARVEOUT2_CFG0, 17:14, secure_scratch40, 30:27);
		s(MC_SECURITY_CARVEOUT2_CFG0, 21:18, secure_scratch41, 28:25);
		s(MC_SECURITY_CARVEOUT2_CFG0, 26:22, secure_scratch56, 29:25);
	}

	// GPU WPR.
	s(MC_SECURITY_CARVEOUT3_BOM,        31:17, secure_scratch50, 14:0);
	s(MC_SECURITY_CARVEOUT3_SIZE_128KB, 11:0,  secure_scratch56, 11:0);
	s32(MC_SECURITY_CARVEOUT3_CLIENT_ACCESS2,  secure_scratch71);
	s32(MC_SECURITY_CARVEOUT3_CLIENT_ACCESS4,  secure_scratch73);
	if (!t210b01)
	{
		s(MC_SECURITY_CARVEOUT3_CFG0,  2:0,  secure_scratch57, 27:25);
		s(MC_SECURITY_CARVEOUT3_CFG0, 10:3,  secure_scratch47, 29:22);
		s(MC_SECURITY_CARVEOUT3_CFG0, 13:11, secure_scratch43, 31:29);
		s(MC_SECURITY_CARVEOUT3_CFG0, 21:14, secure_scratch48, 29:22);
		s(MC_SECURITY_CARVEOUT3_CFG0, 25:22, secure_scratch57, 31:28);
		s(MC_SECURITY_CARVEOUT3_CFG0, 26:26, secure_scratch58, 0:0);
	}
	else
	{
		s(MC_SECURITY_CARVEOUT3_CFG0,  1:0,  secure_scratch57, 31:30);
		s(MC_SECURITY_CARVEOUT3_CFG0,  2:2,  secure_scratch58, 0:0);
		s(MC_SECURITY_CARVEOUT3_CFG0,  6:3,  secure_scratch47, 29:26);
		s(MC_SECURITY_CARVEOUT3_CFG0, 10:7, secure_scratch48, 25:22);
		s(MC_SECURITY_CARVEOUT3_CFG0, 13:11, secure_scratch43, 31:29);
		s(MC_SECURITY_CARVEOUT3_CFG0, 17:14, secure_scratch48, 29:26);
		s(MC_SECURITY_CARVEOUT3_CFG0, 21:18, secure_scratch52, 30:27);
		s(MC_SECURITY_CARVEOUT3_CFG0, 26:22, secure_scratch58, 5:1);
	}

	// TSECA.
	s(MC_SECURITY_CARVEOUT4_BOM,        31:17, secure_scratch51, 14:0);
	s(MC_SECURITY_CARVEOUT4_SIZE_128KB, 11:0,  secure_scratch55, 23:12);
	s(MC_SECURITY_CARVEOUT4_CFG0,       26:0,  secure_scratch39, 26:0);

	// TSECB.
	s(MC_SECURITY_CARVEOUT5_BOM,        31:17, secure_scratch52, 14:0);
	s(MC_SECURITY_CARVEOUT5_SIZE_128KB, 11:0,  secure_scratch57, 23:12);
	s(MC_SECURITY_CARVEOUT5_CFG0,       26:0,  secure_scratch40, 26:0);
}

static void _l4t_mc_config_carveout(bool t210b01)
{
#if CARVEOUT_NVDEC_TSEC_ENABLE
	// Re-enable access for TSEC clients.
	MC(MC_VIDEO_PROTECT_GPU_OVERRIDE_0) &= ~BIT(22);
	MC(MC_VIDEO_PROTECT_GPU_OVERRIDE_1) &= ~(BIT(15) | BIT(14) | BIT(13));
#endif

	// Disabled VPR carveout. DT decides if enabled or not.
	MC(MC_VIDEO_PROTECT_BOM)      = 0;
	MC(MC_VIDEO_PROTECT_SIZE_MB)  = 0;
	MC(MC_VIDEO_PROTECT_REG_CTRL) = VPR_CTRL_TZ_SECURE | VPR_CTRL_LOCKED;

	// Temporarily disable TZDRAM carveout. For launching coldboot TZ.
	MC(MC_SEC_CARVEOUT_BOM)      = 0;
	MC(MC_SEC_CARVEOUT_SIZE_MB)  = 0;
	MC(MC_SEC_CARVEOUT_REG_CTRL) = 0;

	// Print real one, not temp disabled.
	UPRINTF("TZD: TZDRAM Carveout: %08X - %08X\n", TZDRAM_BASE, TZDRAM_BASE - 1 + TZDRAM_SIZE);

	// Configure generalized security carveouts.
	u32 carveout_base = GEN_CARVEOUT_TOP;

#if CARVEOUT_NVDEC_TSEC_ENABLE

	// Set NVDEC keyslot sticky bits.
	clock_enable_nvdec();
	clock_enable_nvjpg();
	NVDEC(NVDEC_SA_KEYSLOT_GLOBAL_RW) = 0xFFFF;     // Read disable.
	NVDEC(NVDEC_SA_KEYSLOT_TZ)        = 0xFFFFFFFF; // TZ enable.
	NVDEC(NVDEC_SA_KEYSLOT_FALCON)    = 0;          // Falcon disable.
	NVDEC(NVDEC_SA_KEYSLOT_OTF)       = 0;          // OTF disable.
	NVDEC(NVDEC_VPR_ALL_OTF_GOTO_VPR) = 1;          // Enable.
	clock_disable_nvjpg();
	clock_disable_nvdec();

	// Set NVDEC carveout. Only for NVDEC bl/prod.
	carveout_base -= ALIGN(CARVEOUT_NVDEC_SIZE, SZ_1M);
	MC(MC_SECURITY_CARVEOUT1_BOM)        = carveout_base;
	MC(MC_SECURITY_CARVEOUT1_BOM_HI)     = 0;
	MC(MC_SECURITY_CARVEOUT1_SIZE_128KB) = CARVEOUT_NVDEC_SIZE / SZ_128K;
	MC(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS0) = 0;
	MC(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS1) = 0;
	MC(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS2) = SEC_CARVEOUT_CA2_R_TSEC  | SEC_CARVEOUT_CA2_W_TSEC;
	MC(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS3) = SEC_CARVEOUT_CA3_R_NVDEC | SEC_CARVEOUT_CA3_W_NVDEC;
	MC(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS4) = 0;
	MC(MC_SECURITY_CARVEOUT1_CFG0) = SEC_CARVEOUT_CFG_LOCKED            |
									 SEC_CARVEOUT_CFG_UNTRANSLATED_ONLY |
									 SEC_CARVEOUT_CFG_RD_SEC            |
									 SEC_CARVEOUT_CFG_RD_FALCON_LS      |
									 SEC_CARVEOUT_CFG_RD_FALCON_HS      |
									 SEC_CARVEOUT_CFG_WR_FALCON_HS      |
									 SEC_CARVEOUT_CFG_APERTURE_ID(1)    |
									 SEC_CARVEOUT_CFG_FORCE_APERTURE_ID_MATCH;
	UPRINTF("GSC1: NVDEC Carveout: %08X - %08X\n",
		MC(MC_SECURITY_CARVEOUT1_BOM), MC(MC_SECURITY_CARVEOUT1_BOM) + MC(MC_SECURITY_CARVEOUT1_SIZE_128KB) * SZ_128K);

#elif CARVEOUT_SECFW_ENABLE

	// Flush data to ram.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_INVALID_WAY, false);

	// Set SC7-Entry/SC7-Exit/R2P/MTC Table or SC7-Exit/BPMP-FW carveout. Only BPMP, CCPLEX and AHB have R/W access.
	MC(MC_SECURITY_CARVEOUT1_BOM)        = carveout_base;
	MC(MC_SECURITY_CARVEOUT1_BOM_HI)     = 0;
	MC(MC_SECURITY_CARVEOUT1_SIZE_128KB) = CARVEOUT_SECFW_SIZE / SZ_128K;
	MC(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS0) = SEC_CARVEOUT_CA0_R_BPMP_C     |
											   SEC_CARVEOUT_CA0_R_PPCSAHBSLV;
	MC(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS1) = SEC_CARVEOUT_CA1_W_BPMP_C     |
											   SEC_CARVEOUT_CA1_R_CCPLEX_C   |
											   SEC_CARVEOUT_CA1_R_CCPLEXLP_C |
											   SEC_CARVEOUT_CA1_W_CCPLEX_C   |
											   SEC_CARVEOUT_CA1_W_CCPLEXLP_C |
											   SEC_CARVEOUT_CA1_W_PPCSAHBSLV;
	MC(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS2) = 0;
	MC(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS3) = 0;
	MC(MC_SECURITY_CARVEOUT1_CLIENT_ACCESS4) = 0;
	MC(MC_SECURITY_CARVEOUT1_CFG0) = SEC_CARVEOUT_CFG_RD_NS  |
									 SEC_CARVEOUT_CFG_RD_SEC |
									 SEC_CARVEOUT_CFG_WR_NS  |
									 SEC_CARVEOUT_CFG_WR_SEC |
									 SEC_CARVEOUT_CFG_LOCKED;
	UPRINTF("GSC1: SECFW Carveout: %08X - %08X\n",
		MC(MC_SECURITY_CARVEOUT1_BOM), MC(MC_SECURITY_CARVEOUT1_BOM) + MC(MC_SECURITY_CARVEOUT1_SIZE_128KB) * SZ_128K);

#endif

	// Set GPU FW WPR carveout. Same value is used for GPU WPR carveout calculation if not full TOS.
	carveout_base -= ALIGN(CARVEOUT_GPUFW_SIZE, SZ_1M);

	// Sanitize it and enable GSC3 for ACR.
	memset((void *)carveout_base, 0, CARVEOUT_GPUFW_SIZE);
	*(u32 *)(carveout_base + CARVEOUT_GPUFW_SIZE - sizeof(u32)) = ACR_GSC3_ENABLE_MAGIC;

	tsec_init_t *tsec_init = (tsec_init_t *)(carveout_base + CARVEOUT_GPUFW_SIZE - FALCON_DMA_PAGE_SIZE);

	// Set TSEC init info.
	tsec_init->sku  = SKU_NX;
	tsec_init->hdcp = HDCP22;
	tsec_init->soc  = t210b01 ? SOC_ID_T210B01 : SOC_ID_T210;

	// Flush data to ram.
	bpmp_mmu_maintenance(BPMP_MMU_MAINT_INVALID_WAY, false);

	// Set carveout config.
	MC(MC_SECURITY_CARVEOUT2_BOM)        = carveout_base;
	MC(MC_SECURITY_CARVEOUT2_BOM_HI)     = 0;
	MC(MC_SECURITY_CARVEOUT2_SIZE_128KB) = CARVEOUT_GPUFW_SIZE / SZ_128K;
	MC(MC_SECURITY_CARVEOUT2_CLIENT_ACCESS0) = 0;
	MC(MC_SECURITY_CARVEOUT2_CLIENT_ACCESS1) = 0;
	MC(MC_SECURITY_CARVEOUT2_CLIENT_ACCESS2) = SEC_CARVEOUT_CA2_R_GPU  | SEC_CARVEOUT_CA2_W_GPU | SEC_CARVEOUT_CA2_R_TSEC;
	MC(MC_SECURITY_CARVEOUT2_CLIENT_ACCESS3) = 0;
	MC(MC_SECURITY_CARVEOUT2_CLIENT_ACCESS4) = SEC_CARVEOUT_CA4_R_GPU2 | SEC_CARVEOUT_CA4_W_GPU2;
	MC(MC_SECURITY_CARVEOUT2_CFG0) = SEC_CARVEOUT_CFG_LOCKED            |
									 SEC_CARVEOUT_CFG_UNTRANSLATED_ONLY |
									 SEC_CARVEOUT_CFG_RD_NS             |
									 SEC_CARVEOUT_CFG_RD_SEC            |
									 SEC_CARVEOUT_CFG_RD_FALCON_LS      |
									 SEC_CARVEOUT_CFG_RD_FALCON_HS      |
									 SEC_CARVEOUT_CFG_WR_FALCON_LS      |
									 SEC_CARVEOUT_CFG_WR_FALCON_HS      |
									 SEC_CARVEOUT_CFG_APERTURE_ID(2)    |
									 SEC_CARVEOUT_CFG_SEND_CFG_TO_GPU   |
									 SEC_CARVEOUT_CFG_FORCE_APERTURE_ID_MATCH; // SEC_CARVEOUT_CFG_IS_WPR is set from GPU.
	UPRINTF("GSC2: GPUFW Carveout: %08X - %08X\n",
		MC(MC_SECURITY_CARVEOUT2_BOM), MC(MC_SECURITY_CARVEOUT2_BOM) + MC(MC_SECURITY_CARVEOUT2_SIZE_128KB) * SZ_128K);

	// Set GPU WPR carveout.
#if CARVEOUT_NVDEC_TSEC_ENABLE
	carveout_base -= ALIGN(CARVEOUT_GPUWPR_SIZE, SZ_1M);
	MC(MC_SECURITY_CARVEOUT3_BOM) = carveout_base;
#else
	MC(MC_SECURITY_CARVEOUT3_BOM) = carveout_base + CARVEOUT_GPUFW_SIZE;
#endif
	MC(MC_SECURITY_CARVEOUT3_BOM_HI)     = 0x0;
	MC(MC_SECURITY_CARVEOUT3_SIZE_128KB) = CARVEOUT_GPUWPR_SIZE / SZ_128K;
	MC(MC_SECURITY_CARVEOUT3_CLIENT_ACCESS0) = 0;
	MC(MC_SECURITY_CARVEOUT3_CLIENT_ACCESS1) = 0;
	MC(MC_SECURITY_CARVEOUT3_CLIENT_ACCESS2) = 0; // HOS: SEC_CARVEOUT_CA2_R_GPU  | SEC_CARVEOUT_CA2_W_GPU
	MC(MC_SECURITY_CARVEOUT3_CLIENT_ACCESS3) = 0;
	MC(MC_SECURITY_CARVEOUT3_CLIENT_ACCESS4) = 0; // HOS: SEC_CARVEOUT_CA4_R_GPU2 | SEC_CARVEOUT_CA4_W_GPU2
	MC(MC_SECURITY_CARVEOUT3_CFG0) = SEC_CARVEOUT_CFG_LOCKED            |
									 SEC_CARVEOUT_CFG_UNTRANSLATED_ONLY |
									 SEC_CARVEOUT_CFG_RD_NS             |
									 SEC_CARVEOUT_CFG_RD_SEC            |
									 SEC_CARVEOUT_CFG_RD_FALCON_LS      |
									 SEC_CARVEOUT_CFG_RD_FALCON_HS      |
									 SEC_CARVEOUT_CFG_WR_FALCON_LS      |
									 SEC_CARVEOUT_CFG_WR_FALCON_HS      |
									 SEC_CARVEOUT_CFG_APERTURE_ID(3)    |
									 SEC_CARVEOUT_CFG_SEND_CFG_TO_GPU   |
									 SEC_CARVEOUT_CFG_FORCE_APERTURE_ID_MATCH; // SEC_CARVEOUT_CFG_IS_WPR is set from GPU.
	UPRINTF("GSC3: GPUW2 Carveout: %08X - %08X\n",
		MC(MC_SECURITY_CARVEOUT3_BOM), MC(MC_SECURITY_CARVEOUT3_BOM) + MC(MC_SECURITY_CARVEOUT3_SIZE_128KB) * SZ_128K);

	/*
	 * Set TSECA/B carveout. Only for NVDEC bl/prod and TSEC.
	 *
	 * Otherwise disabled.
	 *
	 * With HOS SC7-Exit fw, it gets set to disallow RAM access for BPMP. But TZ can change it.
	 * We lock the carveout and save it in restore scratch registers so SC7-Exit can't touch it.
	 */
	carveout_base -= CARVEOUT_NVDEC_TSEC_ENABLE ? ALIGN(CARVEOUT_TSEC_SIZE, SZ_1M) : 0;
	MC(MC_SECURITY_CARVEOUT4_BOM)        = CARVEOUT_NVDEC_TSEC_ENABLE ? carveout_base : 0;
	MC(MC_SECURITY_CARVEOUT4_BOM_HI)     = 0x0;
	MC(MC_SECURITY_CARVEOUT4_SIZE_128KB) = CARVEOUT_NVDEC_TSEC_ENABLE ? CARVEOUT_TSEC_SIZE / SZ_128K : 0;
	MC(MC_SECURITY_CARVEOUT4_CLIENT_ACCESS2) = SEC_CARVEOUT_CA2_R_TSEC  | SEC_CARVEOUT_CA2_W_TSEC;
	MC(MC_SECURITY_CARVEOUT4_CLIENT_ACCESS4) = SEC_CARVEOUT_CA4_R_TSECB | SEC_CARVEOUT_CA4_W_TSECB;
	MC(MC_SECURITY_CARVEOUT4_CFG0) = SEC_CARVEOUT_CFG_LOCKED         |
									 SEC_CARVEOUT_CFG_RD_FALCON_HS   |
									 SEC_CARVEOUT_CFG_WR_FALCON_HS   |
									 SEC_CARVEOUT_CFG_APERTURE_ID(4) |
									 SEC_CARVEOUT_CFG_FORCE_APERTURE_ID_MATCH;
	UPRINTF("GSC4: TSEC1 Carveout: %08X - %08X\n",
		MC(MC_SECURITY_CARVEOUT4_BOM), MC(MC_SECURITY_CARVEOUT4_BOM) + MC(MC_SECURITY_CARVEOUT4_SIZE_128KB) * SZ_128K);

	// Set TSECA carveout. Only for NVDEC bl/prod and TSEC. Otherwise disabled.
	carveout_base -= CARVEOUT_NVDEC_TSEC_ENABLE ? ALIGN(CARVEOUT_TSEC_SIZE, SZ_1M) : 0;
	MC(MC_SECURITY_CARVEOUT5_BOM)        = CARVEOUT_NVDEC_TSEC_ENABLE ? carveout_base : 0;
	MC(MC_SECURITY_CARVEOUT5_BOM_HI)     = 0x0;
	MC(MC_SECURITY_CARVEOUT5_SIZE_128KB) = CARVEOUT_NVDEC_TSEC_ENABLE ? CARVEOUT_TSEC_SIZE / SZ_128K : 0;
	MC(MC_SECURITY_CARVEOUT5_CLIENT_ACCESS2) = SEC_CARVEOUT_CA2_R_TSEC | SEC_CARVEOUT_CA2_W_TSEC;
	MC(MC_SECURITY_CARVEOUT5_CFG0) = SEC_CARVEOUT_CFG_LOCKED         |
									 SEC_CARVEOUT_CFG_RD_FALCON_HS   |
									 SEC_CARVEOUT_CFG_WR_FALCON_HS   |
									 SEC_CARVEOUT_CFG_APERTURE_ID(5) |
									 SEC_CARVEOUT_CFG_FORCE_APERTURE_ID_MATCH;
	UPRINTF("GSC5: TSEC2 Carveout: %08X - %08X\n",
		MC(MC_SECURITY_CARVEOUT5_BOM), MC(MC_SECURITY_CARVEOUT5_BOM) + MC(MC_SECURITY_CARVEOUT5_SIZE_128KB) * SZ_128K);

	UPRINTF("DRAM  Bank 0 TOP:     %08X\n", carveout_base);

	// Save carveouts to lp0 pmc registers.
	_l4t_sdram_lp0_save_params(t210b01);
}

static void _l4t_late_hw_config(bool t210b01)
{
	// Reset System Counters.
	for (u32 i = 0; i < SYSCTR0_COUNTERS; i += sizeof(u32))
		SYSCTR0(SYSCTR0_COUNTERS_BASE + i) = 0;

	/*
	 * PMIC config scratch register
	 *
	 * bit00:    active cluster slow
	 * bit01:    Set: max77621/max77812. Unset: OVR/OVR2.
	 * bit02-07: unused
	 * bit08-15: pmic cpu i2c address
	 * bit16:23: pmic cpu i2c en reg
	 * bit24:31: pmic cpu i2c en val
	 */
	PMC(APBDEV_PMC_SCRATCH201) = BIT(1);

	// Clear PLLM override for SC7.
	PMC(APBDEV_PMC_PLLP_WB0_OVERRIDE) &= ~PMC_PLLP_WB0_OVERRIDE_PLLM_OVERRIDE_ENABLE;

	// Set spare reg to 0xE0000 and clear everything else.
	if (t210b01 && (SYSREG(AHB_AHB_SPARE_REG) & 0xE0000000) != 0xE0000000)
		SYSREG(AHB_AHB_SPARE_REG) = 0xE0000 << 12;

	// HDA loopback disable on prod.
	PMC(APBDEV_PMC_STICKY_BITS) = PMC_STICKY_BITS_HDA_LPBK_DIS;

	// Clear any MC error.
	MC(MC_INTSTATUS) = MC(MC_INTSTATUS);


#if LOCK_PMC_REGISTERS
	// Lock LP0 parameters and misc secure registers. Always happens on warmboot.
 #if CARVEOUT_NVDEC_TSEC_ENABLE
	pmc_scratch_lock(PMC_SEC_LOCK_CARVEOUTS_L4T | PMC_SEC_LOCK_SE_SRK);
 #else
	pmc_scratch_lock(PMC_SEC_LOCK_LP0_PARAMS | PMC_SEC_LOCK_MISC | PMC_SEC_LOCK_SE_SRK);
 #endif

	// Lock SE2 SRK and misc secure regs. Also lock writes on normal LP0 scratch regs.
	if (t210b01)
		pmc_scratch_lock(PMC_SEC_LOCK_MISC_B01 | PMC_SEC_LOCK_SE2_SRK_B01 | PMC_SEC_LOCK_LP0_PARAMS_B01);
#endif
}

static void _l4t_bpmpfw_b01_config(l4t_ctxt_t *ctxt)
{
	char *ram_oc_txt  = ctxt->ram_oc_txt;
	u32   ram_oc_freq = ctxt->ram_oc_freq;
	u32   ram_oc_opt  = ctxt->ram_oc_opt;
	u32   ram_id      = fuse_read_dramid(true);

	// Set default parameters.
	*(u32 *)BPMPFW_B01_DTB_ADDR = 0;
	*(u8  *)BPMPFW_B01_CC_INIT_OP = OP_TRAIN;
	*(u32 *)BPMPFW_B01_CC_PT_TIME = 100;

#if DEBUG_LOG_BPMPFW
	// Set default debug parameters.
	*(u32 *)BPMPFW_B01_LOGLEVEL = 3;
	*(u32 *)BPMPFW_B01_CC_DEBUG = 0x50000101;

	// Set serial debug port.
	if (*(u32 *)BPMPFW_B01_ADTB_BASE == DTB_MAGIC)
		BPMPFW_B01_DTB_SET_SERIAL_PORT(ctxt->serial_port);
#endif

	// Set and copy MTC tables.
	u32 mtc_idx = mtc_table_idx_t210b01[ram_id];
	for (u32 i = 0; i < 3; i++)
	{
		if (true)
			minerva_sdmmc_la_program(BPMPFW_B01_MTC_TABLE_OFFSET(mtc_idx, i), true);
		memcpy(BPMPFW_B01_DTB_EMC_TBL_OFFSET(i), BPMPFW_B01_MTC_TABLE_OFFSET(mtc_idx, i), BPMPFW_B01_MTC_FREQ_TABLE_SIZE);
	}

	// Always use Arachne Register Cell (ARC) for setting rated frequencies if no ram_oc is defined.
	if (!ram_oc_freq)
	{
		if (ram_id >= LPDDR4X_IOWA_4GB_SAMSUNG_K4U6E3S4AM_MGCJ &&
			ram_id <= LPDDR4X_HOAG_4GB_MICRON_MT53E512M32D2NP_046_WTE)
		{
			ram_oc_txt  = "1866000";
			ram_oc_freq = 1866000;
		}
		else
		{
			ram_oc_txt  = "2133000";
			ram_oc_freq = 2133000;
		}
	}

	// Set DRAM voltage.
	if (ctxt->ram_oc_vdd2)
		max7762x_regulator_set_voltage(REGULATOR_SD1,  ctxt->ram_oc_vdd2 * 1000);
	if (ctxt->ram_oc_vddq)
		max7762x_regulator_set_voltage(REGULATOR_RAM0, ctxt->ram_oc_vddq * 1000);

	// A frequency of lower or equal with stock max will skip ARC.
	if (ram_oc_freq > DRAM_T210B01_TBL_MAX_FREQ)
	{
		// Final table.
		const u32 tbl_idx = BPMPFW_B01_DTB_EMC_ENTRIES - 1;

		// Copy table and prep it for Arachne.
		memcpy(BPMPFW_B01_DTB_EMC_TBL_OFFSET(tbl_idx), BPMPFW_B01_MTC_TABLE_OFFSET(mtc_idx, 2), BPMPFW_B01_MTC_FREQ_TABLE_SIZE);

		BPMPFW_B01_DTB_EMC_TBL_SET_NAME(tbl_idx, ram_oc_txt);
		BPMPFW_B01_DTB_EMC_TBL_SET_FREQ(tbl_idx, ram_oc_freq);
		BPMPFW_B01_DTB_EMC_TBL_SET_OPTC(tbl_idx, ram_oc_opt);

		// Enable table.
		BPMPFW_B01_DTB_EMC_TBL_ENABLE(tbl_idx);

		UPRINTF("RAM Frequency set to: %d KHz. Voltage: %d mV\n", ram_oc_freq, ctxt->ram_oc_vdd2);
	}

	// Save BPMP-FW entrypoint for TZ.
	PMC(APBDEV_PMC_SCRATCH39) = BPMPFW_B01_ENTRYPOINT;
	PMC(APBDEV_PMC_SCRATCH_WRITE_DISABLE1) |= BIT(15);
}

static int _l4t_sc7_exit_config(bool t210b01)
{
	if (!t210b01)
	{
		// Apply Nintendo Switch (2017) RSA modulus to SC7-Exit firmware.
		emmc_initialize(false);
		pkg1_warmboot_rsa_mod(SC7EXIT_BASE);
		emmc_end();

		// Set SC7-Exit firmware address for bootrom to find.
		PMC(APBDEV_PMC_SCRATCH1) = SC7EXIT_BASE;
	}
	else
	{
		launch_ctxt_t hos_ctxt = {0};
		u32 fw_fuses = *(u32 *)(SC7EXIT_B01_BASE - sizeof(u32)); // Fuses count in front of actual firmware.

		// Get latest SC7-Exit if needed and setup PA id.
		if (!pkg1_warmboot_config(&hos_ctxt, 0, fw_fuses, 0))
		{
			gfx_con.mute = false;
			gfx_wputs("\nFailed to match warmboot with fuses!\nIf you continue, sleep wont work!");

			gfx_puts("\nPress POWER to continue.\nPress VOL to go to the menu.\n");

			if (!(btn_wait() & BTN_POWER))
				return 0;
		}

		// Copy loaded warmboot fw to address if from storage.
		if (hos_ctxt.warmboot)
			memcpy((void *)SC7EXIT_B01_BASE, hos_ctxt.warmboot, hos_ctxt.warmboot_size);

		// Set SC7-Exit firmware address for bootrom to find.
		PMC(APBDEV_PMC_SECURE_SCRATCH119) = SC7EXIT_B01_BASE;
		PMC(APBDEV_PMC_SEC_DISABLE8) |= BIT(30);
	}

	return 1;
}

static void _l4t_bl33_cfg_set_key(char *env, const char *key, const char *val)
{
	strcat(env, key);
	strcat(env, "=");
	strcat(env, val);
	strcat(env, "\n");
}

static void _l4t_set_config(l4t_ctxt_t *ctxt, const ini_sec_t *ini_sec, int entry_idx, int is_list, bool t210b01)
{
	char *bl33_env = (char *)BL33_ENV_BASE;
	bl33_env[0] = '\0';
	char val[4] = {0};

	// Set default SLD type.
	ctxt->sld_type = BL_MAGIC_L4TLDR_SLD;

	// Parse ini section and prepare BL33 env.
	LIST_FOREACH_ENTRY(ini_kv_t, kv, &ini_sec->kvs, link)
	{
		if (!strcmp("boot_prefixes",    kv->key))
			ctxt->path        = kv->val;
		else if (!strcmp("ram_oc",      kv->key))
		{
			ctxt->ram_oc_txt  = kv->val;
			ctxt->ram_oc_freq = atoi(kv->val);
		}
		else if (!strcmp("ram_oc_vdd2", kv->key))
		{
			ctxt->ram_oc_vdd2 = atoi(kv->val);

			if (t210b01 && ctxt->ram_oc_vdd2 > DRAM_VDD2_OC_MAX_VOLTAGE)
				ctxt->ram_oc_vdd2 = DRAM_VDD2_OC_MAX_VOLTAGE;
			else if (!t210b01 && ctxt->ram_oc_vdd2 > DRAM_VDD2Q_OC_MAX_VOLTAGE)
				ctxt->ram_oc_vdd2 = DRAM_VDD2Q_OC_MAX_VOLTAGE;
			else if (ctxt->ram_oc_vdd2 < DRAM_VDD2_OC_MIN_VOLTAGE)
				ctxt->ram_oc_vdd2 = DRAM_VDD2_OC_MIN_VOLTAGE;
		}
		else if (!strcmp("ram_oc_vddq", kv->key))
		{
			ctxt->ram_oc_vddq = atoi(kv->val);
			if (ctxt->ram_oc_vddq > DRAM_VDDQ_OC_MAX_VOLTAGE)
				ctxt->ram_oc_vddq = DRAM_VDDQ_OC_MAX_VOLTAGE;
			else if (ctxt->ram_oc_vddq < DRAM_VDDQ_OC_MIN_VOLTAGE)
				ctxt->ram_oc_vddq = 0;
		}
		else if (!strcmp("ram_oc_opt",  kv->key))
			ctxt->ram_oc_opt  = atoi(kv->val);
		else if (!strcmp("uart_port",   kv->key))
			ctxt->serial_port = atoi(kv->val);
		else if (!strcmp("sld_type",    kv->key))
			ctxt->sld_type    = strtol(kv->val, NULL, 16);

		// Set key/val to BL33 env.
		_l4t_bl33_cfg_set_key(bl33_env, kv->key, kv->val);
	}

#ifdef DEBUG_UART_PORT
	// Override port if bootloader UART debug is enabled.
	ctxt->serial_port = DEBUG_UART_PORT + 1;
#endif

	// Set r2p parameters.
	if (entry_idx >= 10)
	{
		val[0] = '1';
		val[1] = '0' + (entry_idx % 10);
	}
	else
		val[0] = '0' + entry_idx;
	_l4t_bl33_cfg_set_key(bl33_env, "autoboot",      val);

	// NULL terminate at single char for the next env sets.
	val[1] = 0;

	val[0] = '0' + is_list;
	_l4t_bl33_cfg_set_key(bl33_env, "autoboot_list", val);

	val[0] = '0' + L4T_LOADER_API_REV;
	_l4t_bl33_cfg_set_key(bl33_env, "loader_rev",    val);

	// Enable BL33 memory env import.
	*(u32 *)(BL33_ENV_MAGIC_OFFSET) = BL33_ENV_MAGIC;

	// Set boot path.
	sd_path = (char *)malloc(512);
	sd_path_len = strlen(ctxt->path);
	strcpy(sd_path, ctxt->path);
}

void launch_l4t(const ini_sec_t *ini_sec, int entry_idx, int is_list, bool t210b01)
{
	l4t_ctxt_t *ctxt = (l4t_ctxt_t *)TZ_PARAM_BASE;
	memset(ctxt, 0, TZ_PARAM_SIZE);

	gfx_con_setpos(0, 0);

	// Parse config.
	_l4t_set_config(ctxt, ini_sec, entry_idx, is_list, t210b01);

	if (!ctxt->path)
	{
		_l4t_crit_error("Path missing", false);
		return;
	}

	// Get MTC table.
	ctxt->mtc_table = minerva_get_mtc_table();
	if (!t210b01 && !ctxt->mtc_table)
	{
		_l4t_crit_error("Minerva missing", true);
		return;
	}

	// Load BL31 (ATF/TrustZone fw).
	if (!_l4t_sd_load(BL31_FW))
	{
		_l4t_crit_error("BL31 missing", false);
		return;
	}

	// Load BL33 (U-BOOT/CBOOT).
	if (!_l4t_sd_load(BL33_FW))
	{
		_l4t_crit_error("BL33 missing", false);
		return;
	}

	// Set firmware path.
	strcpy(sd_path, "bootloader/sys/l4t/");
	sd_path_len = strlen(sd_path);

	if (!t210b01)
	{
		// Load SC7-Entry firmware.
		ctxt->sc7entry_size = _l4t_sd_load(SC7ENTRY_FW);
		if (!ctxt->sc7entry_size)
		{
			_l4t_crit_error("loading SC7-Entry", true);
			return;
		}

		// Load BPMP-FW. Does power management.
		if (!_l4t_sd_load(BPMPFW_FW))
		{
			_l4t_crit_error("loading BPMP-FW", true);
			return;
		}
	}
	else
	{
		// Load BPMP-FW. Manages SC7-Entry also.
		if (!_l4t_sd_load(BPMPFW_B01_FW))
		{
			_l4t_crit_error("loading BPMP-FW", true);
			return;
		}

		// Load BPMP-FW MTC table.
		if (!_l4t_sd_load(BPMPFW_B01_MTC_TBL))
		{
			_l4t_crit_error("loading BPMP-FW MTC", true);
			return;
		}
	}

	// Load SC7-Exit firmware.
	if (!_l4t_sd_load(!t210b01 ? SC7EXIT_FW : SC7EXIT_B01_FW))
	{
		_l4t_crit_error("loading SC7-Exit", true);
		return;
	}

	// Set SC7-Exit firmware address to PMC for bootrom and do further setup.
	if (!_l4t_sc7_exit_config(t210b01))
		return;

	// Done loading bootloaders/firmware.
	sd_end();

	// We don't need AHB aperture open.
	mc_disable_ahb_redirect();

	// Enable debug port.
	if (ctxt->serial_port)
	{
		pinmux_config_uart(ctxt->serial_port - 1);
		clock_enable_uart(ctxt->serial_port - 1);
	}

	// Restore UARTB/C TX pins to SPIO.
	gpio_config(GPIO_PORT_G, GPIO_PIN_0, GPIO_MODE_SPIO);
	gpio_config(GPIO_PORT_D, GPIO_PIN_1, GPIO_MODE_SPIO);

	// Configure BL33 parameters.
	if (*(u32 *)BL33_DTB_BASE == DTB_MAGIC)
	{
		// Defaults are for UARTA.
		char *bl33_serial_port = NULL;
		switch (ctxt->serial_port)
		{
		case 0: // Disable.
			break;
		case 1: // UARTA.
			bl33_serial_port = "70006000";
			break;
		case 2: // UARTB.
			bl33_serial_port = "70006040";
			break;
		case 3: // UARTC.
			bl33_serial_port = "70006200";
			break;
		}

		if (bl33_serial_port)
		{
			BL33_DTB_SET_STDOUT_PATH(bl33_serial_port);
			BL33_DTB_SET_STDERR_PATH(bl33_serial_port);
			BL33_DTB_SET_UART_STATUS(ctxt->serial_port);
		}
	}

	// Set BL31 params.
	ctxt->bl31_v1_params.hdr.type     = PARAM_BL31;
	ctxt->bl31_v1_params.hdr.version  = VERSION_1;
	ctxt->bl31_v1_params.hdr.size     = sizeof(bl31_v1_params_t);
	ctxt->bl31_v1_params.hdr.attr     = PARAM_EP_SECURE;
	ctxt->bl31_v1_params.bl33_ep_info = (u64)(u32)&ctxt->bl33_ep_info;

	// Set BL33 params.
	ctxt->bl33_ep_info.hdr.type    = PARAM_EP;
	ctxt->bl33_ep_info.hdr.version = VERSION_1;
	ctxt->bl33_ep_info.hdr.size    = sizeof(entry_point_info_t);
	ctxt->bl33_ep_info.hdr.attr    = PARAM_EP_NON_SECURE;
	ctxt->bl33_ep_info.pc          = BL33_LOAD_BASE;
	ctxt->bl33_ep_info.spsr        = SPSR_EL2T;

	// Set Platform parameters.
	ctxt->bl31_plat_params.tzdram_base = TZDRAM_BASE;
	ctxt->bl31_plat_params.tzdram_size = TZDRAM_SIZE;
#if DEBUG_LOG_ATF
	ctxt->bl31_plat_params.uart_id = ctxt->serial_port;
#endif

	if (!t210b01)
	{
		// Set SC7-Entry fw parameters. For now BPMP-FW is not used on T210.
		ctxt->bl31_plat_params.sc7entry_fw_base = SC7ENTRY_HDR_BASE;
		ctxt->bl31_plat_params.sc7entry_fw_size = ALIGN(ctxt->sc7entry_size + SC7ENTRY_HDR_SIZE, SZ_PAGE);
	}

	// Enable below features.
	ctxt->bl31_plat_params.enable_extra_features = BL31_EXTRA_FEATURES_ENABLE;

	if (!t210b01)
	{
		// Set R2P payload.
		reloc_meta_t *reloc = (reloc_meta_t *)(IPL_LOAD_ADDR + 0x7C);
		memcpy((u8 *)R2P_PAYLOAD_BASE, (u8 *)reloc->start, reloc->end - reloc->start);
		memset((u8 *)R2P_PAYLOAD_BASE + 0x94, 0, sizeof(boot_cfg_t)); // Clear Boot Config Storage.

		// Set R2P payload fw parameters.
		ctxt->bl31_plat_params.r2p_payload_base = R2P_PAYLOAD_BASE;
		ctxt->bl31_plat_params.r2p_payload_size = ALIGN(reloc->end - reloc->start, SZ_PAGE);
	}

	// Set PMC access security. NS is mandatory for T210B01.
	ctxt->bl31_plat_params.flags  = FLAGS_PMC_NON_SECURE; // Unsecure it unconditionally to reduce SMC calls to a minimum.
	// Lift SC7 placement restrictions. Disables TZDRAM increased carveout too.
	ctxt->bl31_plat_params.flags |= FLAGS_SC7_NO_BASE_RESTRICTION;

	// Prepare EMC table.
	if (ctxt->mtc_table)
	{
		// Set DRAM voltage.
		if (ctxt->ram_oc_vdd2)
			max7762x_regulator_set_voltage(REGULATOR_SD1, ctxt->ram_oc_vdd2 * 1000);

		// Train the rest of the table, apply FSP WAR, set RAM to 800 MHz.
		minerva_prep_boot_l4t(ctxt->ram_oc_freq, ctxt->ram_oc_opt, true);

		// Set emc table parameters and copy it.
		int table_entries = minerva_get_mtc_table_entries();
		ctxt->bl31_plat_params.emc_table_base = MTCTABLE_BASE;
		ctxt->bl31_plat_params.emc_table_size = sizeof(emc_table_t) * table_entries;
		memcpy((u32 *)MTCTABLE_BASE, ctxt->mtc_table, sizeof(emc_table_t) * table_entries);
	}

	// Set and enable IRAM based BL31 config.
	PMC(APBDEV_PMC_SECURE_SCRATCH112) = PMC(APBDEV_PMC_SECURE_SCRATCH108);
	PMC(APBDEV_PMC_SECURE_SCRATCH114) = PMC(APBDEV_PMC_SECURE_SCRATCH109);
	PMC(APBDEV_PMC_SECURE_SCRATCH108) = (u32)&ctxt->bl31_v1_params;
	PMC(APBDEV_PMC_SECURE_SCRATCH109) = (u32)&ctxt->bl31_plat_params;
	PMC(APBDEV_PMC_SECURE_SCRATCH110) = BL31_IRAM_PARAMS;

	// Set panel model.
	PMC(APBDEV_PMC_SECURE_SCRATCH113) = display_get_decoded_panel_id();

	// Set charging limit parameters.
	if (fuse_read_hw_type() == FUSE_NX_HW_TYPE_HOAG)
	{
		int in_volt_lim = 0;
		bq24193_get_property(BQ24193_ChargeVoltageLimit, &in_volt_lim);

		PMC(APBDEV_PMC_SECURE_SCRATCH113) |= in_volt_lim << 16;
	}

	// Disable writes to above registers.
	PMC(APBDEV_PMC_SEC_DISABLE8) |= BIT(18) | BIT(16) | BIT(12) | BIT(10) | BIT(8);

	// Set BPMP-FW parameters.
	if (t210b01)
		_l4t_bpmpfw_b01_config(ctxt);

	// Set carveouts and save them to PMC for SC7 Exit.
	_l4t_mc_config_carveout(t210b01);

	// Deinit any unneeded HW.
	hw_deinit(false, ctxt->sld_type);

	// Do late hardware config.
	_l4t_late_hw_config(t210b01);

	if (t210b01)
	{
		// Launch BL31.
		ccplex_boot_cpu0(TZDRAM_COLD_ENTRY, true);

		// Enable Wrap burst for BPMP, GPU and PCIE.
		MSELECT(MSELECT_CONFIG) = (MSELECT(MSELECT_CONFIG) & (~(MSELECT_CFG_ERR_RESP_EN_GPU | MSELECT_CFG_ERR_RESP_EN_PCIE))) |
								  (MSELECT_CFG_WRAP_TO_INCR_GPU | MSELECT_CFG_WRAP_TO_INCR_PCIE | MSELECT_CFG_WRAP_TO_INCR_BPMP);

		// For T210B01, prep reset vector for SC7 save state and start BPMP-FW.
		EXCP_VEC(EVP_COP_RESET_VECTOR) = BPMPFW_B01_ENTRYPOINT;
		void (*bpmp_fw_ptr)() = (void *)BPMPFW_B01_ENTRYPOINT;
		(*bpmp_fw_ptr)();
	}
	else
	{
		// If T210, BPMP-FW runs BL31.
		void (*bpmp_fw_ptr)() = (void *)BPMPFW_ENTRYPOINT;
		(*bpmp_fw_ptr)();
	}

	// Halt BPMP.
	while (true)
		bpmp_halt();
}
