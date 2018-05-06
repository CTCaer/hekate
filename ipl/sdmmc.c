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

#include <string.h>
#include "sdmmc.h"
#include "mmc.h"
#include "sd.h"
#include "util.h"
#include "heap.h"

/*#include "gfx.h"
extern gfx_ctxt_t gfx_ctxt;
extern gfx_con_t gfx_con;
#define DPRINTF(...) gfx_printf(&gfx_con, __VA_ARGS__)*/
#define DPRINTF(...)

static inline u32 unstuff_bits(u32 *resp, u32 start, u32 size)
{
	const u32 mask = (size < 32 ? 1 << size : 0) - 1;
	const u32 off = 3 - ((start) / 32);
	const u32 shft = (start) & 31;
	u32 res = resp[off] >> shft;
	if (size + shft > 32)
		res |= resp[off - 1] << ((32 - shft) % 32);
	return res & mask;
}

/*
* Common functions for SD and MMC.
*/

static int _sdmmc_storage_check_result(u32 res)
{
	//Error mask:
	//R1_OUT_OF_RANGE, R1_ADDRESS_ERROR, R1_BLOCK_LEN_ERROR,
	//R1_ERASE_SEQ_ERROR, R1_ERASE_PARAM, R1_WP_VIOLATION,
	//R1_LOCK_UNLOCK_FAILED, R1_COM_CRC_ERROR, R1_ILLEGAL_COMMAND,
	//R1_CARD_ECC_FAILED, R1_CC_ERROR, R1_ERROR, R1_CID_CSD_OVERWRITE,
	//R1_WP_ERASE_SKIP, R1_ERASE_RESET, R1_SWITCH_ERROR
	if (!(res & 0xFDF9A080))
		return 1;
	//TODO: R1_SWITCH_ERROR we can skip for certain card types.
	return 0;
}

static int _sdmmc_storage_execute_cmd_type1_ex(sdmmc_storage_t *storage, u32 *resp, u32 cmd, u32 arg, u32 check_busy, u32 expected_state, u32 mask)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, cmd, arg, SDMMC_RSP_TYPE_1, check_busy);
	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, 0, 0))
		return 0;

	sdmmc_get_rsp(storage->sdmmc, resp, 4, SDMMC_RSP_TYPE_1);
	if (mask)
		*resp &= ~mask;

	if (_sdmmc_storage_check_result(*resp))
		if (expected_state == 0x10 || R1_CURRENT_STATE(*resp) == expected_state)
			return 1;
	return 0;
}

static int _sdmmc_storage_execute_cmd_type1(sdmmc_storage_t *storage, u32 cmd, u32 arg, u32 check_busy, u32 expected_state)
{
	u32 tmp;
	return _sdmmc_storage_execute_cmd_type1_ex(storage, &tmp, cmd, arg, check_busy, expected_state, 0);
}

static int _sdmmc_storage_go_idle_state(sdmmc_storage_t *storage)
{
	sdmmc_cmd_t cmd;
	sdmmc_init_cmd(&cmd, MMC_GO_IDLE_STATE, 0, SDMMC_RSP_TYPE_0, 0);
	return sdmmc_execute_cmd(storage->sdmmc, &cmd, 0, 0);
}

static int _sdmmc_storage_get_cid(sdmmc_storage_t *storage, void *buf)
{
	sdmmc_cmd_t cmd;
	sdmmc_init_cmd(&cmd, MMC_ALL_SEND_CID, 0, SDMMC_RSP_TYPE_2, 0);
	if (!sdmmc_execute_cmd(storage->sdmmc, &cmd, 0, 0))
		return 0;
	sdmmc_get_rsp(storage->sdmmc, buf, 0x10, SDMMC_RSP_TYPE_2);
	return 1;
}

static int _sdmmc_storage_select_card(sdmmc_storage_t *storage)
{
	return _sdmmc_storage_execute_cmd_type1(storage, MMC_SELECT_CARD, storage->rca << 16, 1, 0x10);
}

static int _sdmmc_storage_get_csd(sdmmc_storage_t *storage, void *buf)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, MMC_SEND_CSD, storage->rca << 16, SDMMC_RSP_TYPE_2, 0);
	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, 0, 0))
		return 0;
	sdmmc_get_rsp(storage->sdmmc, buf, 0x10, SDMMC_RSP_TYPE_2);
	return 1;
}

static int _sdmmc_storage_set_blocklen(sdmmc_storage_t *storage, u32 blocklen)
{
	return _sdmmc_storage_execute_cmd_type1(storage, MMC_SET_BLOCKLEN, blocklen, 0, R1_STATE_TRAN);
}

static int _sdmmc_storage_get_status(sdmmc_storage_t *storage, u32 *resp, u32 mask)
{
	return _sdmmc_storage_execute_cmd_type1_ex(storage, resp, MMC_SEND_STATUS, storage->rca << 16, 0, R1_STATE_TRAN, mask);
}

static int _sdmmc_storage_check_status(sdmmc_storage_t *storage)
{
	u32 tmp;
	return _sdmmc_storage_get_status(storage, &tmp, 0);
}

static int _sdmmc_storage_readwrite_ex(sdmmc_storage_t *storage, u32 *blkcnt_out, u32 sector, u32 num_sectors, void *buf, u32 is_write)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, is_write ? MMC_WRITE_MULTIPLE_BLOCK : MMC_READ_MULTIPLE_BLOCK, sector, SDMMC_RSP_TYPE_1, 0);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.num_sectors = num_sectors;
	reqbuf.blksize = 512;
	reqbuf.is_write = is_write;
	reqbuf.is_multi_block = 1;
	reqbuf.is_auto_cmd12 = 1;

	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, &reqbuf, blkcnt_out))
	{
		u32 tmp = 0;
		sdmmc_stop_transmission(storage->sdmmc, &tmp);
		_sdmmc_storage_get_status(storage, &tmp, 0);
		return 0;
	}
	return 1;
}

int sdmmc_storage_end(sdmmc_storage_t *storage)
{
	if (!_sdmmc_storage_go_idle_state(storage))
		return 0;
	sdmmc_end(storage->sdmmc);
	return 1;
}

static int _sdmmc_storage_readwrite(sdmmc_storage_t *storage, u32 sector, u32 num_sectors, void *buf, u32 is_write)
{
	u8 *bbuf = (u8 *)buf;

	while (num_sectors)
	{
		u32 blkcnt = 0;
		//Retry once on error.
		if (!_sdmmc_storage_readwrite_ex(storage, &blkcnt, sector, MIN(num_sectors, 0xFFFF), bbuf, is_write))
			if (!_sdmmc_storage_readwrite_ex(storage, &blkcnt, sector, MIN(num_sectors, 0xFFFF), bbuf, is_write))
				return 0;
		DPRINTF("readwrite: %08X\n", blkcnt);
		sector += blkcnt;
		num_sectors -= blkcnt;
		bbuf += 512 * blkcnt;
	}
}

int sdmmc_storage_read(sdmmc_storage_t *storage, u32 sector, u32 num_sectors, void *buf)
{
	return _sdmmc_storage_readwrite(storage, sector, num_sectors, buf, 0);
}

int sdmmc_storage_write(sdmmc_storage_t *storage, u32 sector, u32 num_sectors, void *buf)
{
	return _sdmmc_storage_readwrite(storage, sector, num_sectors, buf, 1);
}

/*
* MMC specific functions. 
*/

static int _mmc_storage_get_op_cond_inner(sdmmc_storage_t *storage, u32 *pout, u32 power)
{
	sdmmc_cmd_t cmd;

	u32 arg = 0;
	switch (power)
	{
	case SDMMC_POWER_1_8:
		arg = 0x40000080; //Sector access, voltage.
		break;
	case SDMMC_POWER_3_3:
		arg = 0x403F8000; //Sector access, voltage.
		break;
	default:
		return 0;
	}

	sdmmc_init_cmd(&cmd, MMC_SEND_OP_COND, arg, SDMMC_RSP_TYPE_3, 0);
	if (!sdmmc_execute_cmd(storage->sdmmc, &cmd, 0, 0))
		return 0;

	return sdmmc_get_rsp(storage->sdmmc, pout, 4, SDMMC_RSP_TYPE_3);
}

static int _mmc_storage_get_op_cond(sdmmc_storage_t *storage, u32 power)
{
	u32 timeout = get_tmr() + 1500000;

	while (1)
	{
		u32 cond = 0;
		if (!_mmc_storage_get_op_cond_inner(storage, &cond, power))
			break;
		if (cond & 0x80000000)
		{
			if (cond & 0x40000000)
				storage->has_sector_access = 1;
			return 1;
		}
		if (get_tmr() > timeout)
			break;
		sleep(1000);
	}

	return 0;
}

static int _mmc_storage_set_relative_addr(sdmmc_storage_t *storage)
{
	return _sdmmc_storage_execute_cmd_type1(storage, MMC_SET_RELATIVE_ADDR, storage->rca << 16, 0, 0x10);
}

static int _mmc_storage_get_ext_csd(sdmmc_storage_t *storage, void *buf)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, MMC_SEND_EXT_CSD, 0, SDMMC_RSP_TYPE_1, 0);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.blksize = 512;
	reqbuf.num_sectors = 1;
	reqbuf.is_write = 0;
	reqbuf.is_multi_block = 0;
	reqbuf.is_auto_cmd12 = 0;

	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, &reqbuf, 0))
		return 0;

	u32 tmp = 0;
	sdmmc_get_rsp(storage->sdmmc, &tmp, 4, SDMMC_RSP_TYPE_1);
	return _sdmmc_storage_check_result(tmp);
}

static int _mmc_storage_switch(sdmmc_storage_t *storage, u32 arg)
{
	return _sdmmc_storage_execute_cmd_type1(storage, MMC_SWITCH, arg, 1, 0x10);
}

static int _mmc_storage_switch_buswidth(sdmmc_storage_t *storage, u32 bus_width)
{
	if (bus_width == SDMMC_BUS_WIDTH_1)
		return 1;

	u32 arg = 0;
	switch (bus_width)
	{
	case SDMMC_BUS_WIDTH_4:
		arg = SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_WIDTH_4);
		break;
	case SDMMC_BUS_WIDTH_8:
		arg = SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_WIDTH_8);
		break;
	}

	if (_mmc_storage_switch(storage, arg))
		if (_sdmmc_storage_check_status(storage))
		{
			sdmmc_set_bus_width(storage->sdmmc, bus_width);
			return 1;
		}

	return 0;
}

static int _mmc_storage_enable_HS(sdmmc_storage_t *storage, int check)
{
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS)))
		return 0;
	if (check && !_sdmmc_storage_check_status(storage))
		return 0;
	if (!sdmmc_setup_clock(storage->sdmmc, 2))
		return 0;
	DPRINTF("[mmc] switched to HS\n");
	if (check || _sdmmc_storage_check_status(storage))
		return 1;
	return 0;
}

static int _mmc_storage_enable_HS200(sdmmc_storage_t *storage)
{
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS200)))
		return 0;
	if (!sdmmc_setup_clock(storage->sdmmc, 3))
		return 0;
	if (!sdmmc_config_tuning(storage->sdmmc, 3, MMC_SEND_TUNING_BLOCK_HS200))
		return 0;
	DPRINTF("[mmc] switched to HS200\n");
	return _sdmmc_storage_check_status(storage);
}

static int _mmc_storage_enable_HS400(sdmmc_storage_t *storage)
{
	if (!_mmc_storage_enable_HS200(storage))
		return 0;
	sdmmc_get_venclkctl(storage->sdmmc);
	if (!_mmc_storage_enable_HS(storage, 0))
		return 0;
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_BUS_WIDTH, EXT_CSD_DDR_BUS_WIDTH_8)))
		return 0;
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS400)))
		return 0;
	if (!sdmmc_setup_clock(storage->sdmmc, 4))
		return 0;
	DPRINTF("[mmc] switched to HS400\n");
	return _sdmmc_storage_check_status(storage);
}

static int _mmc_storage_enable_highspeed(sdmmc_storage_t *storage, u32 card_type, u32 type)
{
	//TODO: this should be a config item.
	//---v
	if (!1 || sdmmc_get_voltage(storage->sdmmc) != SDMMC_POWER_1_8)
		goto out;

	if (sdmmc_get_bus_width(storage->sdmmc) == SDMMC_BUS_WIDTH_8 &&
		card_type & EXT_CSD_CARD_TYPE_HS400_1_8V &&
		type == 4)
		return _mmc_storage_enable_HS400(storage);

	if (sdmmc_get_bus_width(storage->sdmmc) == SDMMC_BUS_WIDTH_8 ||
		sdmmc_get_bus_width(storage->sdmmc) == SDMMC_BUS_WIDTH_4
		&& card_type & EXT_CSD_CARD_TYPE_HS200_1_8V
		&& (type == 4 || type == 3))
		return _mmc_storage_enable_HS200(storage);

out:;
	if (card_type & EXT_CSD_CARD_TYPE_HS_52)
		return _mmc_storage_enable_HS(storage, 1);
	return 1;
}

static int _mmc_storage_enable_bkops(sdmmc_storage_t *storage)
{
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_SET_BITS, EXT_CSD_BKOPS_EN, EXT_CSD_BKOPS_LEVEL_2)))
		return 0;
	return _sdmmc_storage_check_status(storage);
}

int sdmmc_storage_init_mmc(sdmmc_storage_t *storage, sdmmc_t *sdmmc, u32 id, u32 bus_width, u32 type)
{
	memset(storage, 0, sizeof(sdmmc_storage_t));
	storage->sdmmc = sdmmc;
	storage->rca = 2; //TODO: this could be a config item.

	if (!sdmmc_init(sdmmc, id, SDMMC_POWER_1_8, SDMMC_BUS_WIDTH_1, 0, 0))
		return 0;
	DPRINTF("[mmc] after init\n");

	sleep(1000 + (74000 + sdmmc->divisor - 1) / sdmmc->divisor);

	if (!_sdmmc_storage_go_idle_state(storage))
		return 0;
	DPRINTF("[mmc] went to idle state\n");

	if (!_mmc_storage_get_op_cond(storage, SDMMC_POWER_1_8))
		return 0;
	DPRINTF("[mmc] got op cond\n");

	if (!_sdmmc_storage_get_cid(storage, storage->cid))
		return 0;
	DPRINTF("[mmc] got cid\n");

	if (!_mmc_storage_set_relative_addr(storage))
		return 0;
	DPRINTF("[mmc] set relative addr\n");

	if (!_sdmmc_storage_get_csd(storage, storage->csd))
		return 0;
	DPRINTF("[mmc] got csd\n");

	if (!sdmmc_setup_clock(storage->sdmmc, 1))
		return 0;
	DPRINTF("[mmc] after setup clock\n");

	if (!_sdmmc_storage_select_card(storage))
		return 0;
	DPRINTF("[mmc] card selected\n");

	if (!_sdmmc_storage_set_blocklen(storage, 512))
		return 0;
	DPRINTF("[mmc] set blocklen to 512\n");

	u32 *csd = (u32 *)storage->csd;
	//Check system specification version, only version 4.0 and later support below features.
	if (unstuff_bits(csd, 122, 4) < CSD_SPEC_VER_4)
	{
		storage->sec_cnt = (1 + unstuff_bits(csd, 62, 12)) << (unstuff_bits(csd, 47, 3) + 2);
		return 1;
	}

	if (!_mmc_storage_switch_buswidth(storage, bus_width))
		return 0;
	DPRINTF("[mmc] switched buswidth\n");

	u8 *ext_csd = (u8 *)malloc(512);
	if (!_mmc_storage_get_ext_csd(storage, ext_csd))
	{
		free(ext_csd);
		return 0;
	}
	//gfx_hexdump(&gfx_con, 0, ext_csd, 512);

	storage->sec_cnt = *(u32 *)&ext_csd[EXT_CSD_SEC_CNT];

	if (storage->cid[0xE] == 0x11 && ext_csd[EXT_CSD_BKOPS_EN] & EXT_CSD_BKOPS_LEVEL_2)
		_mmc_storage_enable_bkops(storage);

	if (!_mmc_storage_enable_highspeed(storage, ext_csd[EXT_CSD_CARD_TYPE], type))
	{
		free(ext_csd);
		return 0;
	}
	DPRINTF("[mmc] switched to possible highspeed mode\n");

	sdmmc_sd_clock_ctrl(storage->sdmmc, 1);

	free(ext_csd);
	return 1;
}

int sdmmc_storage_set_mmc_partition(sdmmc_storage_t *storage, u32 partition)
{
	if (!_mmc_storage_switch(storage, SDMMC_SWITCH(MMC_SWITCH_MODE_WRITE_BYTE, EXT_CSD_PART_CONFIG, partition)))
		return 0;
	if (!_sdmmc_storage_check_status(storage))
		return 0;
	storage->partition = partition;
	return 1;
}

/*
* SD specific functions.
*/

static int _sd_storage_execute_app_cmd(sdmmc_storage_t *storage, u32 expected_state, u32 mask, sdmmc_cmd_t *cmd, sdmmc_req_t *req, u32 *blkcnt_out)
{
	u32 tmp;
	if (!_sdmmc_storage_execute_cmd_type1_ex(storage, &tmp, MMC_APP_CMD, storage->rca << 16, 0, expected_state, mask))
		return 0;
	return sdmmc_execute_cmd(storage->sdmmc, cmd, req, blkcnt_out);
}

static int _sd_storage_execute_app_cmd_type1(sdmmc_storage_t *storage, u32 *resp, u32 cmd, u32 arg, u32 check_busy, u32 expected_state)
{
	if (!_sdmmc_storage_execute_cmd_type1(storage, MMC_APP_CMD, storage->rca << 16, 0, R1_STATE_TRAN))
		return 0;
	return _sdmmc_storage_execute_cmd_type1_ex(storage, resp, cmd, arg, check_busy, expected_state, 0);
}

static int _sd_storage_send_if_cond(sdmmc_storage_t *storage)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, SD_SEND_IF_COND, 0x1AA, SDMMC_RSP_TYPE_5, 0);
	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, 0, 0))
		return 0;

	//TODO: we may have received a timeout error in the above request, which indicates a version 1 card.

	u32 resp = 0;
	if (!sdmmc_get_rsp(storage->sdmmc, &resp, 4, SDMMC_RSP_TYPE_5))
		return 0;

	return (resp & 0xFF) == 0xAA ? 1 : 0;
}

static int _sd_storage_get_op_cond_once(sdmmc_storage_t *storage, u32 *cond, int is_version_1, int supports_low_voltage)
{
	sdmmc_cmd_t cmdbuf;
	u32 arg = (((~is_version_1 & 1) << 28) & 0xBFFFFFFF | ((~is_version_1 & 1) << 30)) & 0xFEFFFFFF | ((supports_low_voltage & ~is_version_1 & 1) << 24) | 0x100000;
	sdmmc_init_cmd(&cmdbuf, SD_APP_OP_COND, arg, SDMMC_RSP_TYPE_3, 0);
	if (!_sd_storage_execute_app_cmd(storage, 0x10, is_version_1 ? 0x400000 : 0, &cmdbuf, 0, 0))
		return 0;
	return sdmmc_get_rsp(storage->sdmmc, cond, 4, SDMMC_RSP_TYPE_3);
}

static int _sd_storage_get_op_cond(sdmmc_storage_t *storage, int is_version_1, int supports_low_voltage)
{
	u32 timeout = get_tmr() + 1500000;

	while (1)
	{
		u32 cond = 0;
		if (!_sd_storage_get_op_cond_once(storage, &cond, is_version_1, supports_low_voltage))
			break;
		if (cond & 0x80000000)
		{
			if (cond & 0x40000000)
				storage->has_sector_access = 1;
			// TODO: Some SD Card incorrectly report low voltage support
			// Disable it for now
			if (cond & 0x1000000 && supports_low_voltage)
			{
				//The low voltage regulator configuration is valid for SDMMC1 only.
				if (storage->sdmmc->id == SDMMC_1 && 
					_sdmmc_storage_execute_cmd_type1(storage, SD_SWITCH_VOLTAGE, 0, 0, R1_STATE_READY))
				{
					if (!sdmmc_enable_low_voltage(storage->sdmmc))
						return 0;
					storage->is_low_voltage = 1;

					DPRINTF("-> switched to low voltage\n");
				}
			}

			return 1;
		}
		if (get_tmr() > timeout)
			break;
		sleep(10000); // Needs to be at least 10ms for some SD Cards
	}

	return 0;
}

static int _sd_storage_get_rca(sdmmc_storage_t *storage)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, SD_SEND_RELATIVE_ADDR, 0, SDMMC_RSP_TYPE_4, 0);

	u32 timeout = get_tmr() + 1500000;

	while (1)
	{
		if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, 0, 0))
			break;

		u32 resp = 0;
		if (!sdmmc_get_rsp(storage->sdmmc, &resp, 4, SDMMC_RSP_TYPE_4))
			break;

		if (resp >> 16)
		{
			storage->rca = resp >> 16;
			return 1;
		}

		if (get_tmr() > timeout)
			break;
		sleep(1000);
	}

	return 0;
}

int _sd_storage_get_scr(sdmmc_storage_t *storage, void *buf)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, SD_APP_SEND_SCR, 0, SDMMC_RSP_TYPE_1, 0);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.blksize = 8;
	reqbuf.num_sectors = 1;
	reqbuf.is_write = 0;
	reqbuf.is_multi_block = 0;
	reqbuf.is_auto_cmd12 = 0;

	if (!_sd_storage_execute_app_cmd(storage, R1_STATE_TRAN, 0, &cmdbuf, &reqbuf, 0))
		return 0;

	u32 tmp = 0;
	sdmmc_get_rsp(storage->sdmmc, &tmp, 4, SDMMC_RSP_TYPE_1);
	return _sdmmc_storage_check_result(tmp);
}

int _sd_storage_switch_get(sdmmc_storage_t *storage, void *buf)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, SD_SWITCH, 0xFFFFFF, SDMMC_RSP_TYPE_1, 0);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.blksize = 64;
	reqbuf.num_sectors = 1;
	reqbuf.is_write = 0;
	reqbuf.is_multi_block = 0;
	reqbuf.is_auto_cmd12 = 0;

	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, &reqbuf, 0))
		return 0;

	u32 tmp = 0;
	sdmmc_get_rsp(storage->sdmmc, &tmp, 4, SDMMC_RSP_TYPE_1);
	return _sdmmc_storage_check_result(tmp);
}

int _sd_storage_switch(sdmmc_storage_t *storage, void *buf, int flag, u32 arg)
{
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, SD_SWITCH, arg | (flag << 31) | 0xFFFFF0, SDMMC_RSP_TYPE_1, 0);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.blksize = 64;
	reqbuf.num_sectors = 1;
	reqbuf.is_write = 0;
	reqbuf.is_multi_block = 0;
	reqbuf.is_auto_cmd12 = 0;

	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, &reqbuf, 0))
		return 0;

	u32 tmp = 0;
	sdmmc_get_rsp(storage->sdmmc, &tmp, 4, SDMMC_RSP_TYPE_1);
	return _sdmmc_storage_check_result(tmp);
}

int _sd_storage_enable_highspeed(sdmmc_storage_t *storage, u32 hs_type, u8 *buf)
{
	if (!_sd_storage_switch(storage, buf, 0, hs_type))
		return 0;

	u32 type_out = buf[16] & 0xF;
	if (type_out != hs_type)
		return 0;

	if (((u16)buf[0] << 8) | buf[1] < 0x320)
	{
		if (!_sd_storage_switch(storage, buf, 1, hs_type))
			return 0;

		if (type_out != buf[16] & 0xF)
			return 0;
	}

	return 1;
}

int _sd_storage_enable_highspeed_low_volt(sdmmc_storage_t *storage, u32 type, u8 *buf)
{
	if (sdmmc_get_bus_width(storage->sdmmc) != SDMMC_BUS_WIDTH_4)
		return 0;

	if (!_sd_storage_switch_get(storage, buf))
		return 0;

	u32 hs_type = 0;
	switch (type)
	{
	case 11:
		if (buf[13] & 8)
		{
			type = 11;
			hs_type = 3;
			break;
		}
		//Fall through.
	case 10:
		if (!(buf[13] & 4))
			return 0;
		type = 10;
		hs_type = 2;
		break;
	default:
		return 0;
		break;
	}

	if (!_sd_storage_enable_highspeed(storage, hs_type, buf))
		return 0;
	if (!sdmmc_setup_clock(storage->sdmmc, type))
		return 0;
	if (!sdmmc_config_tuning(storage->sdmmc, type, MMC_SEND_TUNING_BLOCK))
		return 0;
	return _sdmmc_storage_check_status(storage);
}

int _sd_storage_enable_highspeed_high_volt(sdmmc_storage_t *storage, u8 *buf)
{
	if (!_sd_storage_switch_get(storage, buf))
		return 0;

	if (!(buf[13] & 2))
		return 1;

	if (!_sd_storage_enable_highspeed(storage, 1, buf))
		return 0;
	if (!_sdmmc_storage_check_status(storage))
		return 0;
	return sdmmc_setup_clock(storage->sdmmc, 7);
}

int sdmmc_storage_init_sd(sdmmc_storage_t *storage, sdmmc_t *sdmmc, u32 id, u32 bus_width, u32 type)
{
	memset(storage, 0, sizeof(sdmmc_storage_t));
	storage->sdmmc = sdmmc;

	if (!sdmmc_init(sdmmc, id, SDMMC_POWER_3_3, SDMMC_BUS_WIDTH_1, 5, 0))
		return 0;
	DPRINTF("[sd] after init\n");

	sleep(1000 + (74000 + sdmmc->divisor - 1) / sdmmc->divisor);

	if (!_sdmmc_storage_go_idle_state(storage))
		return 0;
	DPRINTF("[sd] went to idle state\n");

	if (!_sd_storage_send_if_cond(storage))
		return 0;
	DPRINTF("[sd] after send if cond\n");

	//TODO: use correct version here -----v
	if (!_sd_storage_get_op_cond(storage, 0, bus_width == SDMMC_BUS_WIDTH_4 && (type | 1) == 11))
		return 0;
	DPRINTF("[sd] got op cond\n");

	if (!_sdmmc_storage_get_cid(storage, storage->cid))
		return 0;
	DPRINTF("[sd] got cid\n");

	if (!_sd_storage_get_rca(storage))
		return 0;
	DPRINTF("[sd] got rca (= %04X)\n", storage->rca);

	if (!_sdmmc_storage_get_csd(storage, storage->csd))
		return 0;
	DPRINTF("[sd] got csd\n");

	//Parse CSD.
	u32 *csd = (u32 *)storage->csd;
	u32 csd_struct = unstuff_bits(csd, 126, 2);
	switch (csd_struct)
	{
	case 0:
		storage->sec_cnt = (1 + unstuff_bits(csd, 62, 12)) << (unstuff_bits(csd, 47, 3) + 2);
		break;
	case 1:
		storage->sec_cnt = (1 + unstuff_bits(csd, 48, 22)) << 10;
		break;
	default:
		DPRINTF("[sd] Unknown CSD structure %d\n", csd_struct);
		//TODO: I've encountered this with one of my SD cards, but
		//      according to the spec only version 0 and 1 are
		//      supposed to be in use (mine was version 2).
		//return 0;
		break;
	}

	if (!storage->is_low_voltage)
	{
		if (!sdmmc_setup_clock(storage->sdmmc, 6))
			return 0;
		DPRINTF("[sd] after setup clock\n");
	}

	if (!_sdmmc_storage_select_card(storage))
		return 0;
	DPRINTF("[sd] card selected\n");

	if (!_sdmmc_storage_set_blocklen(storage, 512))
		return 0;
	DPRINTF("[sd] set blocklen to 512\n");

	u32 tmp = 0;
	if (!_sd_storage_execute_app_cmd_type1(storage, &tmp, SD_APP_SET_CLR_CARD_DETECT, 0, 0, R1_STATE_TRAN))
		return 0;
	DPRINTF("[sd] cleared card detect\n");

	u8 *buf = (u8 *)malloc(512);
	if (!_sd_storage_get_scr(storage, buf))
		return 0;
	memcpy(storage->scr, buf, 8);
	DPRINTF("[sd] got scr\n");

	// Check if card supports a wider bus and if it's not SD Version 1.0
	if (bus_width == SDMMC_BUS_WIDTH_4 && storage->scr[1] & 4 && (storage->scr[0] & 0xF))
	{
		if (!_sd_storage_execute_app_cmd_type1(storage, &tmp, SD_APP_SET_BUS_WIDTH, SD_BUS_WIDTH_4, 0, R1_STATE_TRAN))
		{
			free(buf);
			return 0;
		}
		sdmmc_set_bus_width(storage->sdmmc, SDMMC_BUS_WIDTH_4);
		DPRINTF("[sd] switched to wide bus width\n");
	}
	else
		DPRINTF("[sd] SD does not support wide bus width\n");

	if (storage->is_low_voltage)
	{
		if (!_sd_storage_enable_highspeed_low_volt(storage, type, buf))
		{
			free(buf);
			return 0;
		}
		DPRINTF("[sd] enabled highspeed (low voltage)\n");
	}
	else if (type != 6 && (storage->scr[0] & 0xF) != 0)
	{
		if (!_sd_storage_enable_highspeed_high_volt(storage, buf))
		{
			free(buf);
			return 0;
		}
		DPRINTF("[sd] enabled highspeed (high voltage)\n");
	}

	sdmmc_sd_clock_ctrl(sdmmc, 1);

	free(buf);
	return 1;
}

/*
* Gamecard specific functions.
*/

int _gc_storage_custom_cmd(sdmmc_storage_t *storage, void *buf)
{
	u32 resp;
	sdmmc_cmd_t cmdbuf;
	sdmmc_init_cmd(&cmdbuf, 60, 0, SDMMC_RSP_TYPE_1, 1);

	sdmmc_req_t reqbuf;
	reqbuf.buf = buf;
	reqbuf.blksize = 0x40;
	reqbuf.num_sectors = 1;
	reqbuf.is_write = 1;
	reqbuf.is_multi_block = 0;
	reqbuf.is_auto_cmd12 = 0;

	if (!sdmmc_execute_cmd(storage->sdmmc, &cmdbuf, &reqbuf, 0))
	{
		sdmmc_stop_transmission(storage->sdmmc, &resp);
		return 0;
	}

	if (!sdmmc_get_rsp(storage->sdmmc, &resp, 4, SDMMC_RSP_TYPE_1))
		return 0;
	if (!_sdmmc_storage_check_result(resp))
		return 0;
	return _sdmmc_storage_check_status(storage);
}

int sdmmc_storage_init_gc(sdmmc_storage_t *storage, sdmmc_t *sdmmc)
{
	memset(storage, 0, sizeof(sdmmc_storage_t));
	storage->sdmmc = sdmmc;

	if (!sdmmc_init(sdmmc, SDMMC_2, SDMMC_POWER_1_8, SDMMC_BUS_WIDTH_8, 14, 0))
		return 0;
	DPRINTF("[gc] after init\n");

	sleep(1000 + (10000 + sdmmc->divisor - 1) / sdmmc->divisor);

	if (!sdmmc_config_tuning(storage->sdmmc, 14, MMC_SEND_TUNING_BLOCK_HS200))
		return 0;
	DPRINTF("[gc] after tuning\n");

	sdmmc_sd_clock_ctrl(sdmmc, 1);

	return 1;
}
