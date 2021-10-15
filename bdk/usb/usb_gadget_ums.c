/*
 * USB Gadget UMS driver for Tegra X1
 *
 * Copyright (c) 2003-2008 Alan Stern
 * Copyright (c) 2009 Samsung Electronics
 *                    Author: Michal Nazarewicz <m.nazarewicz@samsung.com>
 * Copyright (c) 2019-2021 CTCaer
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

#include <usb/usbd.h>
#include <gfx_utils.h>
#include <soc/hw_init.h>
#include <soc/t210.h>
#include <storage/nx_sd.h>
#include <storage/sdmmc.h>
#include <storage/sdmmc_driver.h>
#include <utils/btn.h>
#include <utils/sprintf.h>
#include <utils/util.h>

#include <memory_map.h>

//#define DPRINTF(...) gfx_printf(__VA_ARGS__)
#define DPRINTF(...)

#define UMS_MAX_LUN 1 // Only 1 disk/partition for now.

#define USB_BULK_CB_WRAP_LEN 31
#define USB_BULK_CB_SIG      0x43425355 // USBC.
#define USB_BULK_IN_FLAG     0x80

#define USB_BULK_CS_WRAP_LEN 13
#define USB_BULK_CS_SIG      0x53425355 // USBS.

#define USB_STATUS_PASS        0
#define USB_STATUS_FAIL        1
#define USB_STATUS_PHASE_ERROR 2

#define UMS_DISK_LBA_SHIFT 9
#define UMS_DISK_LBA_SIZE  (1 << UMS_DISK_LBA_SHIFT)

#define UMS_DISK_MAX_IO_TRANSFER_64K (USB_EP_BUFFER_MAX_SIZE >> UMS_DISK_LBA_SHIFT)
#define UMS_DISK_MAX_IO_TRANSFER_32K (UMS_DISK_MAX_IO_TRANSFER_64K / 2)

#define UMS_SCSI_TRANSFER_512K (0x80000 >> UMS_DISK_LBA_SHIFT)

#define UMS_EP_OUT_MAX_XFER (USB_EP_BULK_OUT_MAX_XFER)

// Length of a SCSI Command Data Block.
#define SCSI_MAX_CMD_SZ 16

// SCSI device types
#define SCSI_TYPE_DISK  0x00

// SCSI commands.
#define SC_FORMAT_UNIT        0x04
#define SC_INQUIRY            0x12
#define SC_LOG_SENSE          0x4D
#define SC_MODE_SELECT_6      0x15
#define SC_MODE_SELECT_10     0x55
#define SC_MODE_SENSE_6       0x1A
#define SC_MODE_SENSE_10      0x5A
#define SC_PREVENT_ALLOW_MEDIUM_REMOVAL 0x1E
#define SC_READ_6             0x08
#define SC_READ_10            0x28
#define SC_READ_12            0xA8
#define SC_READ_CAPACITY      0x25
#define SC_READ_FORMAT_CAPACITIES 0x23
#define SC_READ_HEADER        0x44
#define SC_READ_TOC           0x43
#define SC_RELEASE            0x17
#define SC_REQUEST_SENSE      0x03
#define SC_RESERVE            0x16
#define SC_SEND_DIAGNOSTIC    0x1D
#define SC_START_STOP_UNIT    0x1B
#define SC_SYNCHRONIZE_CACHE  0x35
#define SC_TEST_UNIT_READY    0x00
#define SC_VERIFY             0x2F
#define SC_WRITE_6            0x0A
#define SC_WRITE_10           0x2A
#define SC_WRITE_12           0xAA

// SCSI Sense Key/Additional Sense Code/ASC Qualifier values.
#define SS_NO_SENSE                           0x0
#define SS_COMMUNICATION_FAILURE              0x40800
#define SS_INVALID_COMMAND                    0x52000
#define SS_INVALID_FIELD_IN_CDB               0x52400
#define SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE 0x52100
#define SS_MEDIUM_NOT_PRESENT                 0x23A00
#define SS_MEDIUM_REMOVAL_PREVENTED           0x55302
#define SS_NOT_READY_TO_READY_TRANSITION      0x62800
#define SS_RESET_OCCURRED                     0x62900
#define SS_SAVING_PARAMETERS_NOT_SUPPORTED    0x53900
#define SS_UNRECOVERED_READ_ERROR             0x31100
#define SS_WRITE_ERROR                        0x30C02
#define SS_WRITE_PROTECTED                    0x72700

#define SK(x)   ((u8) ((x) >> 16)) // Sense Key byte, etc.
#define ASC(x)  ((u8) ((x) >> 8))
#define ASCQ(x) ((u8) (x))

enum ums_state {
	UMS_STATE_NORMAL = 0,
	UMS_STATE_ABORT_BULK_OUT,
	UMS_STATE_PROTOCOL_RESET,
	UMS_STATE_EXIT,
	UMS_STATE_TERMINATED
};

enum ums_result {
	UMS_RES_OK          = 0,
	UMS_RES_IO_ERROR    = -5,
	UMS_RES_TIMEOUT     = -3,
	UMS_RES_PROT_FATAL  = -4,
	UMS_RES_INVALID_ARG = -22
};


enum data_direction {
	DATA_DIR_UNKNOWN = 0,
	DATA_DIR_FROM_HOST,
	DATA_DIR_TO_HOST,
	DATA_DIR_NONE
};

enum buffer_state {
	BUF_STATE_EMPTY = 0,
	BUF_STATE_FULL,
	BUF_STATE_BUSY
};

typedef struct _bulk_recv_pkt_t {
	u32 Signature;          // 'USBC'.
	u32 Tag;                // Unique per command id.
	u32 DataTransferLength; // Size of the data.
	u8  Flags;              // Direction in bit 7.
	u8  Lun;                // LUN (normally 0).
	u8  Length;             // Of the CDB, <= SCSI_MAX_CMD_SZ.
	u8  CDB[16];            // Command Data Block.
} bulk_recv_pkt_t;

typedef struct _bulk_send_pkt_t {
	u32 Signature; // 'USBS'.
	u32 Tag;       // Same as original command.
	u32 Residue;   // Amount not transferred.
	u8  Status;
} bulk_send_pkt_t;

typedef struct _logical_unit_t
{
	sdmmc_t *sdmmc;
	sdmmc_storage_t *storage;

	u32 num_sectors;
	u32 offset;

	int unmounted;

	u32 ro;
	u32 type;
	u32 partition;
	u32 removable;
	u32 prevent_medium_removal;

	u32 info_valid;

	u32 sense_data;
	u32 sense_data_info;
	u32 unit_attention_data;
} logical_unit_t;

typedef struct _bulk_ctxt_t {
	u32  bulk_in;
	int  bulk_in_status;
	u32  bulk_in_length;
	u32  bulk_in_length_actual;
	u8  *bulk_in_buf;
	enum buffer_state bulk_in_buf_state;

	u32  bulk_out;
	int  bulk_out_status;
	u32  bulk_out_length;
	u32  bulk_out_length_actual;
	int  bulk_out_ignore;
	u8  *bulk_out_buf;
	enum buffer_state bulk_out_buf_state;
} bulk_ctxt_t;

typedef struct _usbd_gadget_ums_t {
	bulk_ctxt_t bulk_ctxt;

	u32  cmnd_size;
	u8   cmnd[SCSI_MAX_CMD_SZ];

	u32  lun_idx; // lun index
	logical_unit_t lun;

	enum ums_state state; // For exception handling.

	enum data_direction data_dir;
	u32  data_size;
	u32  data_size_from_cmnd;
	u32  tag;
	u32  residue;
	u32  usb_amount_left;
	bool cbw_req_queued;

	u32  phase_error;
	u32  short_packet_received;

	int  thread_wakeup_needed;
	int  can_stall;

	u32 timeouts;
	bool xusb;

	void (*system_maintenance)(bool);
	void *label;
	void (*set_text)(void *, const char *);
} usbd_gadget_ums_t;

static usb_ops_t usb_ops;

static inline void put_array_le_to_be16(u16 val, void *p)
{
	u8 *_p = p;
	_p[0] = val >> 8;
	_p[1] = val;
}

static inline void put_array_le_to_be32(u32 val, void *p)
{
	u8 *_p = p;
	_p[0] = val >> 24;
	_p[1] = val >> 16;
	_p[2] = val >> 8;
	_p[3] = val;
}

static inline u16 get_array_be_to_le16(const void *p)
{
	const u8 *_p = p;
	u16 val = _p[0] << 8 | _p[1];
	return val;
}

static inline u32 get_array_be_to_le24(const void *p)
{
	const u8 *_p = p;
	u32 val = (_p[0] << 16) | (_p[1] << 8) | _p[2];
	return val;
}

static inline u32 get_array_be_to_le32(const void *p)
{
	const u8 *_p = p;
	u32 val = (_p[0] << 24) | (_p[1] << 16) | (_p[2] << 8) | _p[3];
	return val;
}

static void raise_exception(usbd_gadget_ums_t *ums, enum ums_state new_state)
{
	/* Do nothing if a higher-priority exception is already in progress.
	 * If a lower-or-equal priority exception is in progress, preempt it
	 * and notify the main thread by sending it a signal. */
	if (ums->state <= new_state) {
		ums->state = new_state;
		ums->thread_wakeup_needed = 1;
	}
}

static void ums_handle_ep0_ctrl(usbd_gadget_ums_t *ums)
{
	if (usb_ops.usbd_handle_ep0_ctrl_setup())
		raise_exception(ums, UMS_STATE_PROTOCOL_RESET);
}

static int ums_wedge_bulk_in_endpoint(usbd_gadget_ums_t *ums)
{
	/* usbd_set_ep_wedge(bulk_ctxt->bulk_in); */

	return UMS_RES_OK;
}

static int ums_set_stall(u32 ep)
{
	usb_ops.usbd_set_ep_stall(ep, USB_EP_CFG_STALL);

	return UMS_RES_OK;
}

static int ums_clear_stall(u32 ep)
{
	usb_ops.usbd_set_ep_stall(ep, USB_EP_CFG_CLEAR);

	return UMS_RES_OK;
}

static void ums_flush_endpoint(u32 ep)
{
	if (usb_ops.usbd_flush_endpoint)
		usb_ops.usbd_flush_endpoint(ep);
}

static void _ums_transfer_start(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt, u32 ep, u32 sync_timeout)
{
	if (ep == bulk_ctxt->bulk_in)
	{
		bulk_ctxt->bulk_in_status = usb_ops.usb_device_ep1_in_write(
			bulk_ctxt->bulk_in_buf, bulk_ctxt->bulk_in_length,
			&bulk_ctxt->bulk_in_length_actual, sync_timeout);

		if (bulk_ctxt->bulk_in_status == USB_ERROR_XFER_ERROR)
		{
			ums->set_text(ums->label, "#FFDD00 Error:# EP IN transfer!");
			ums_flush_endpoint(bulk_ctxt->bulk_in);
		}
		else if (bulk_ctxt->bulk_in_status == USB2_ERROR_XFER_NOT_ALIGNED)
			ums->set_text(ums->label, "#FFDD00 Error:# EP IN Buffer not aligned!");

		if (sync_timeout)
			bulk_ctxt->bulk_in_buf_state = BUF_STATE_EMPTY;
	}
	else
	{
		bulk_ctxt->bulk_out_status = usb_ops.usb_device_ep1_out_read(
			bulk_ctxt->bulk_out_buf, bulk_ctxt->bulk_out_length,
			&bulk_ctxt->bulk_out_length_actual, sync_timeout);

		if (bulk_ctxt->bulk_out_status == USB_ERROR_XFER_ERROR)
		{
			ums->set_text(ums->label, "#FFDD00 Error:# EP OUT transfer!");
			ums_flush_endpoint(bulk_ctxt->bulk_out);
		}
		else if (bulk_ctxt->bulk_out_status == USB2_ERROR_XFER_NOT_ALIGNED)
			ums->set_text(ums->label, "#FFDD00 Error:# EP OUT Buffer not aligned!");

		if (sync_timeout)
			bulk_ctxt->bulk_out_buf_state = BUF_STATE_FULL;
	}
}

static void _ums_transfer_out_big_read(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
		bulk_ctxt->bulk_out_status = usb_ops.usb_device_ep1_out_read_big(
			bulk_ctxt->bulk_out_buf, bulk_ctxt->bulk_out_length,
			&bulk_ctxt->bulk_out_length_actual);

		if (bulk_ctxt->bulk_out_status == USB_ERROR_XFER_ERROR)
		{
			ums->set_text(ums->label, "#FFDD00 Error:# EP OUT transfer!");
			ums_flush_endpoint(bulk_ctxt->bulk_out);
		}

		bulk_ctxt->bulk_out_buf_state = BUF_STATE_FULL;
}

static void _ums_transfer_finish(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt, u32 ep, u32 sync_timeout)
{
	if (ep == bulk_ctxt->bulk_in)
	{
		bulk_ctxt->bulk_in_status = usb_ops.usb_device_ep1_in_writing_finish(
			&bulk_ctxt->bulk_in_length_actual, sync_timeout);

		if (bulk_ctxt->bulk_in_status == USB_ERROR_XFER_ERROR)
		{
			ums->set_text(ums->label, "#FFDD00 Error:# EP IN transfer!");
			ums_flush_endpoint(bulk_ctxt->bulk_in);
		}

		bulk_ctxt->bulk_in_buf_state = BUF_STATE_EMPTY;
	}
	else
	{
		bulk_ctxt->bulk_out_status = usb_ops.usb_device_ep1_out_reading_finish(
			&bulk_ctxt->bulk_out_length_actual, sync_timeout);

		if (bulk_ctxt->bulk_out_status == USB_ERROR_XFER_ERROR)
		{
			ums->set_text(ums->label, "#FFDD00 Error:# EP OUT transfer!");
			ums_flush_endpoint(bulk_ctxt->bulk_out);
		}

		bulk_ctxt->bulk_out_buf_state = BUF_STATE_FULL;
	}
}

static void _ums_reset_buffer(bulk_ctxt_t *bulk_ctxt, u32 ep)
{
	if (ep == bulk_ctxt->bulk_in)
		bulk_ctxt->bulk_in_buf  = (u8 *)USB_EP_BULK_IN_BUF_ADDR;
	else
		bulk_ctxt->bulk_out_buf = (u8 *)USB_EP_BULK_OUT_BUF_ADDR;
}

/*
 * The following are old data based on max 64KB SCSI transfers.
 * The endpoint xfer is actually 41.2 MB/s and SD card max 39.2 MB/s, with higher SCSI
 * transfers, but the concurrency still helps and increases speeds by 20%.
 *
 * Concurrency of the SDMMC and USB xfers is very important with no cache.
 * The worst offender being the SD card. We are already limited by bus, so
 * concurrency helps minimize the SDMMC overhead.
 * Max achieved bulk endpoint rate on a Tegra X1 and USB2.0 is 39.4 MB/s.
 *
 * USB bulk endpoint raw max transfer rate:
 * 39.4MB/S - SCSI 128KB.
 * 38.2MB/s - SCSI  64KB.
 *
 *     128 KB,      64 KB,      32 KB,     16 KB,      8 KB - Internal SDMMC I\O Sizes
 * -------------------------------------------------------------------------------------
 * eMMC - Toshiba - 4MB reads: 314.8 MB/s:
 * 225.9 MB/s, 168.6 MB/s, 114.7 MB/s, 86.4 MB/s, 50.3 MB/s - RAW SDMMC.
 *  33.5 MB/s,  31.9 MB/s,  29.3 MB/s, 27.1 MB/s, 22.1 MB/s - SCSI 128KB, No concurrency.
 *  33.5 MB/s,  35.3 MB/s,  36.3 MB/s, 37.3 MB/s, 37.8 MB/s - SCSI 128KB, Concurrency.
 *  --.- --/-,  31.1 MB/s,  28.7 MB/s, 26.5 MB/s, 21.7 MB/s - SCSI  64KB, No concurrency.
 *  --.- --/-,  31.1 MB/s,  32.7 MB/s, 34.4 MB/s, 35.0 MB/s - SCSI  64KB, Concurrency.
 *
 * SD Card - Samsung Evo+ 128GB - 4MB reads: 91.6 MB/s:
 *  72.6 MB/s,  62.8 MB/s,  47.4 MB/s, 31.1 MB/s, 18.5 MB/s - RAW SDMMC.
 *  25.5 MB/s,  24.2 MB/s,  21.5 MB/s, 17.4 MB/s, 12.6 MB/s - SCSI 128KB, No concurrency.
 *  25.5 MB/s,  30.0 MB/s,  32.6 MB/s, 28.3 MB/s, 18.0 MB/s - SCSI 128KB, Concurrency.
 *  --.- --/-,  23.8 MB/s,  21.2 MB/s, 17.1 MB/s, 12.5 MB/s - SCSI  64KB, No concurrency.
 *  --.- --/-,  23.8 MB/s,  27.2 MB/s, 25.8 MB/s, 17.5 MB/s - SCSI  64KB, Concurrency.
 */

static int _scsi_read(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	u32 lba_offset;
	bool first_read = true;
	u8 *sdmmc_buf = (u8 *)SDXC_BUF_ALIGNED;

	// Get the starting LBA and check that it's not too big.
	if (ums->cmnd[0] == SC_READ_6)
		lba_offset = get_array_be_to_le24(&ums->cmnd[1]);
	else
	{
		lba_offset = get_array_be_to_le32(&ums->cmnd[2]);

		// We allow DPO and FUA bypass cache bits, but we don't use them.
		if ((ums->cmnd[1] & ~0x18) != 0)
		{
			ums->lun.sense_data = SS_INVALID_FIELD_IN_CDB;

			return UMS_RES_INVALID_ARG;
		}
	}
	if (lba_offset >= ums->lun.num_sectors)
	{
		ums->set_text(ums->label, "#FF8000 Warn:# Read - Out of range! Host notified.");
		ums->lun.sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;

		return UMS_RES_INVALID_ARG;
	}

	// Check that request data size is not 0.
	u32 amount_left = ums->data_size_from_cmnd >> UMS_DISK_LBA_SHIFT;
	if (!amount_left)
		return UMS_RES_IO_ERROR; // No default reply.

	// Limit IO transfers based on request for faster concurrent reads.
	u32 max_io_transfer = (amount_left >= UMS_SCSI_TRANSFER_512K) ?
		UMS_DISK_MAX_IO_TRANSFER_64K : UMS_DISK_MAX_IO_TRANSFER_32K;

	while (true)
	{
		// Max io size and end sector limits.
		u32 amount = MIN(amount_left, max_io_transfer);
		amount = MIN(amount, ums->lun.num_sectors - lba_offset);

		// Check if it is a read past the end sector.
		if (!amount)
		{
			ums->lun.sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
			ums->lun.sense_data_info = lba_offset;
			ums->lun.info_valid = 1;
			bulk_ctxt->bulk_in_length = 0;
			bulk_ctxt->bulk_in_buf_state = BUF_STATE_FULL;
			break;
		}

		// Do the SDMMC read.
		if (!sdmmc_storage_read(ums->lun.storage, ums->lun.offset + lba_offset, amount, sdmmc_buf))
			amount = 0;

		// Wait for the async USB transfer to finish.
		if (!first_read)
			_ums_transfer_finish(ums, bulk_ctxt, bulk_ctxt->bulk_in, USB_XFER_SYNCED);

		lba_offset   += amount;
		amount_left  -= amount;
		ums->residue -= amount << UMS_DISK_LBA_SHIFT;

		bulk_ctxt->bulk_in_length    = amount << UMS_DISK_LBA_SHIFT;
		bulk_ctxt->bulk_in_buf_state = BUF_STATE_FULL;
		bulk_ctxt->bulk_in_buf       = sdmmc_buf;

		// If an error occurred, report it and its position.
		if (!amount)
		{
			ums->set_text(ums->label, "#FFDD00 Error:# SDMMC Read!");
			ums->lun.sense_data = SS_UNRECOVERED_READ_ERROR;
			ums->lun.sense_data_info = lba_offset;
			ums->lun.info_valid = 1;
			break;
		}

		// Last SDMMC read. Last part will be sent by the finish reply function.
		if (!amount_left)
			break;

		// Start the USB transfer.
		_ums_transfer_start(ums, bulk_ctxt, bulk_ctxt->bulk_in, USB_XFER_START);
		first_read = false;

		// Increment our buffer to read new data.
		sdmmc_buf += amount << UMS_DISK_LBA_SHIFT;
	}

	return UMS_RES_IO_ERROR; // No default reply.
}

/*
 * Writes are another story.
 * Tests showed that big writes are faster than concurrent 32K usb reads + writes.
 * The only thing that can help here is caching the writes. But for the simplicity
 * of this implementation it will not be implemented yet.
 */

static int _scsi_write(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	static char txt_buf[256];
	u32 amount_left_to_req, amount_left_to_write;
	u32 usb_lba_offset, lba_offset;
	u32 amount;

	if (ums->lun.ro)
	{
		ums->set_text(ums->label, "#FF8000 Warn:# Write - Read only! Host notified.");
		ums->lun.sense_data = SS_WRITE_PROTECTED;

		return UMS_RES_INVALID_ARG;
	}

	if (ums->cmnd[0] == SC_WRITE_6)
		lba_offset = get_array_be_to_le24(&ums->cmnd[1]);
	else
	{
		lba_offset = get_array_be_to_le32(&ums->cmnd[2]);

		// We allow DPO and FUA bypass cache bits. We only implement FUA by performing synchronous output.
		if (ums->cmnd[1] & ~0x18)
		{
			ums->lun.sense_data = SS_INVALID_FIELD_IN_CDB;

			return UMS_RES_INVALID_ARG;
		}
	}

	// Check that starting LBA is not past the end sector offset.
	if (lba_offset >= ums->lun.num_sectors)
	{
		ums->set_text(ums->label, "#FF8000 Warn:# Write - Out of range! Host notified.");
		ums->lun.sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;

		return UMS_RES_INVALID_ARG;
	}

	// Carry out the file writes.
	usb_lba_offset = lba_offset;
	amount_left_to_req = ums->data_size_from_cmnd;
	amount_left_to_write = ums->data_size_from_cmnd;

	while (amount_left_to_write > 0)
	{
		// Queue a request for more data from the host.
		if (amount_left_to_req > 0)
		{

			// Limit write to max supported read from EP OUT.
			amount = MIN(amount_left_to_req, UMS_EP_OUT_MAX_XFER);

			if (usb_lba_offset >= ums->lun.num_sectors)
			{
				ums->set_text(ums->label, "#FFDD00 Error:# Write - Past last sector!");
				ums->lun.sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
				ums->lun.sense_data_info = usb_lba_offset;
				ums->lun.info_valid = 1;
				break;
			}

			// Get the next buffer.
			usb_lba_offset += amount >> UMS_DISK_LBA_SHIFT;
			ums->usb_amount_left -= amount;
			amount_left_to_req -= amount;

			bulk_ctxt->bulk_out_length = amount;

			_ums_transfer_out_big_read(ums, bulk_ctxt);
		}

		if (bulk_ctxt->bulk_out_buf_state == BUF_STATE_FULL)
		{
			bulk_ctxt->bulk_out_buf_state = BUF_STATE_EMPTY;

			// Did something go wrong with the transfer?.
			if (bulk_ctxt->bulk_out_status != 0)
			{
				ums->lun.sense_data = SS_COMMUNICATION_FAILURE;
				ums->lun.sense_data_info = lba_offset;
				ums->lun.info_valid = 1;
				s_printf(txt_buf, "#FFDD00 Error:# Write - Comm failure %d!", bulk_ctxt->bulk_out_status);
				ums->set_text(ums->label, txt_buf);
				break;
			}

			amount = bulk_ctxt->bulk_out_length_actual;

			if ((ums->lun.num_sectors - lba_offset) < (amount >> UMS_DISK_LBA_SHIFT))
			{
				DPRINTF("write %X @ %X beyond end %X\n", amount, lba_offset, ums->lun.num_sectors);
				amount = (ums->lun.num_sectors - lba_offset) << UMS_DISK_LBA_SHIFT;
			}

			/*
			 * Don't accept excess data.  The spec doesn't say
			 * what to do in this case.  We'll ignore the error.
			 */
			amount = MIN(amount, bulk_ctxt->bulk_out_length);

			// Don't write a partial block.
			amount -= (amount & 511);
			if (amount == 0)
				goto empty_write;

			// Perform the write.
			if (!sdmmc_storage_write(ums->lun.storage, ums->lun.offset + lba_offset,
				amount >> UMS_DISK_LBA_SHIFT, (u8 *)bulk_ctxt->bulk_out_buf))
				amount = 0;

DPRINTF("file write %X @ %X\n", amount, lba_offset);

			lba_offset           += amount >> UMS_DISK_LBA_SHIFT;
			amount_left_to_write -= amount;
			ums->residue         -= amount;

			// If an error occurred, report it and its position.
			if (!amount)
			{
				ums->set_text(ums->label, "#FFDD00 Error:# SDMMC Write!");
				ums->lun.sense_data = SS_WRITE_ERROR;
				ums->lun.sense_data_info = lba_offset;
				ums->lun.info_valid = 1;
				break;
			}

 empty_write:
			// Did the host decide to stop early?
			if (bulk_ctxt->bulk_out_length_actual < bulk_ctxt->bulk_out_length)
			{
				ums->set_text(ums->label, "#FFDD00 Error:# Empty Write!");
				ums->short_packet_received = 1;
				break;
			}
		}
	}

	return UMS_RES_IO_ERROR; // No default reply.
}

static int _scsi_verify(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	// Check that start LBA is past the end sector offset.
	u32 lba_offset = get_array_be_to_le32(&ums->cmnd[2]);
	if (lba_offset >= ums->lun.num_sectors)
	{
		ums->set_text(ums->label, "#FF8000 Warn:# Verif - Out of range! Host notified.");
		ums->lun.sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;

		return UMS_RES_INVALID_ARG;
	}

	// We allow DPO but we don't implement it. Check that nothing else is enabled.
	if (ums->cmnd[1] & ~0x10)
	{
		ums->lun.sense_data = SS_INVALID_FIELD_IN_CDB;

		return UMS_RES_INVALID_ARG;
	}

	u32  verification_length = get_array_be_to_le16(&ums->cmnd[7]);
	if (verification_length == 0)
		return UMS_RES_IO_ERROR; // No default reply.

	u32 amount;
	while (verification_length > 0)
	{

		// Limit to EP buffer size and end sector offset.
		amount = MIN(verification_length, USB_EP_BUFFER_MAX_SIZE >> UMS_DISK_LBA_SHIFT);
		amount = MIN(amount, ums->lun.num_sectors - lba_offset);
		if (amount == 0) {
			ums->lun.sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
			ums->lun.sense_data_info = lba_offset;
			ums->lun.info_valid = 1;
			break;
		}

		if (!sdmmc_storage_read(ums->lun.storage, ums->lun.offset + lba_offset, amount, bulk_ctxt->bulk_in_buf))
			amount = 0;

DPRINTF("File read %X @ %X\n", amount, lba_offset);

		if (!amount)
		{
			ums->set_text(ums->label, "#FFDD00 Error:# File verify!");
			ums->lun.sense_data = SS_UNRECOVERED_READ_ERROR;
			ums->lun.sense_data_info = lba_offset;
			ums->lun.info_valid = 1;
			break;
		}
		lba_offset += amount;
		verification_length -= amount;
	}
	return UMS_RES_OK;
}

static int _scsi_inquiry(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	u8 *buf = (u8 *)bulk_ctxt->bulk_in_buf;

	memset(buf, 0, 36);

	// Enable Vital Product Data (EVPD) and Unit Serial Number.
	if (ums->cmnd[1] == 1 && ums->cmnd[2] == 0x80)
	{
		buf[0] = 0;
		buf[1] = ums->cmnd[2];
		buf[2] = 0;
		buf[3] = 20;  // Additional length.

		buf += 4;
		s_printf((char *)buf, "%04X%s",
			ums->lun.storage->cid.serial, ums->lun.type == MMC_SD ? " SD " : " eMMC ");

		switch (ums->lun.partition)
		{
		case 0:
			strcpy((char *)buf + strlen((char *)buf), "RAW");
			break;
		case EMMC_GPP + 1:
			s_printf((char *)buf + strlen((char *)buf), "GPP");
			break;
		case EMMC_BOOT0 + 1:
			s_printf((char *)buf + strlen((char *)buf), "BOOT0");
			break;
		case EMMC_BOOT1 + 1:
			s_printf((char *)buf + strlen((char *)buf), "BOOT1");
			break;
		}

		for (u32 i = strlen((char *)buf); i < 20; i++)
			buf[i] = ' ';

		return 24;
	}
	else /* if (ums->cmnd[1] == 0 && ums->cmnd[2] == 0) */ // Standard inquiry.
	{
		buf[0] = SCSI_TYPE_DISK;
		buf[1] = ums->lun.removable ? 0x80 : 0;
		buf[2] = 6;  // ANSI INCITS 351-2001 (SPC-2).////////SPC2: 4, SPC4: 6
		buf[3] = 2;  // SCSI-2 INQUIRY data format.
		buf[4] = 31; // Additional length.
		// buf5-7: No special options.

		// Vendor ID. Max 8 chars.
		buf += 8;
		strcpy((char *)buf, "hekate");

		// Product ID. Max 16 chars.
		buf += 8;
		switch (ums->lun.partition)
		{
		case 0:
			s_printf((char *)buf, "%s", "SD RAW");
			break;
		case EMMC_GPP + 1:
			s_printf((char *)buf, "%s%s",
				ums->lun.type == MMC_SD ? "SD " : "eMMC ", "GPP");
			break;
		case EMMC_BOOT0 + 1:
			s_printf((char *)buf, "%s%s",
				ums->lun.type == MMC_SD ? "SD " : "eMMC ", "BOOT0");
			break;
		case EMMC_BOOT1 + 1:
			s_printf((char *)buf, "%s%s",
				ums->lun.type == MMC_SD ? "SD " : "eMMC ", "BOOT1");
			break;
		}

		// Rev ID. Max 4 chars.
		buf += 16;
		strcpy((char *)buf, "1.00");

		return 36;
	}
}

static int _scsi_request_sense(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	u8 *buf = (u8 *)bulk_ctxt->bulk_in_buf;
	u32 sd, sdinfo;
	int valid;

	sd = ums->lun.sense_data;
	sdinfo = ums->lun.sense_data_info;
	valid = ums->lun.info_valid << 7;
	ums->lun.sense_data = SS_NO_SENSE;
	ums->lun.sense_data_info = 0;
	ums->lun.info_valid = 0;

	memset(buf, 0, 18);
	buf[0]  = valid | 0x70; // Valid, current error.
	buf[2]  = SK(sd);
	put_array_le_to_be32(sdinfo, &buf[3]); // Sense information.
	buf[7]  = 18 - 8; // Additional sense length.
	buf[12] = ASC(sd);
	buf[13] = ASCQ(sd);

	return 18;
}

static int _scsi_read_capacity(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	u8 *buf = (u8 *)bulk_ctxt->bulk_in_buf;
	u32 lba = get_array_be_to_le32(&ums->cmnd[2]);
	int pmi = ums->cmnd[8];

	// Check the PMI and LBA fields.
	if (pmi > 1 || (pmi == 0 && lba != 0))
	{
		ums->lun.sense_data = SS_INVALID_FIELD_IN_CDB;

		return UMS_RES_INVALID_ARG;
	}

	put_array_le_to_be32(ums->lun.num_sectors - 1, &buf[0]); // Max logical block.
	put_array_le_to_be32(UMS_DISK_LBA_SIZE, &buf[4]);        // Block length.

	return 8;
}

static int _scsi_log_sense(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	u8  *buf = (u8 *)bulk_ctxt->bulk_in_buf;
	u8  *buf0 = buf;
	bool valid_page = false;

	u8 pc = ums->cmnd[2] >> 6;
	u8 page_code = ums->cmnd[2] & 0x3F;
	u8 sub_page_code = ums->cmnd[3];

	if (ums->cmnd[1] & 1)
	{
		ums->lun.sense_data = SS_SAVING_PARAMETERS_NOT_SUPPORTED;

		return UMS_RES_INVALID_ARG;
	}

	if (pc != 1) // Current cumulative values.
	{
		ums->lun.sense_data = SS_INVALID_FIELD_IN_CDB;

		return UMS_RES_INVALID_ARG;
	}

	memset(buf, 0, 8);
	if (page_code == 0x00 && !sub_page_code) // Supported pages.
	{
		valid_page = true;
		buf[0] = 0x00; // Page code.
		buf += 4;

		buf[0] = 0x00; // Page 0.
		buf[1] = 0x0D; // Page 1.

		buf += 2;
	}
	else if (page_code == 0x0d && !sub_page_code) // Temperature.
	{
		valid_page = true;
		buf[0] = 0x0D;
		buf += 4;

		put_array_le_to_be16(0, &buf[0]); // Param code.
		buf[2] = 1;  // Param control byte.
		buf[3] = 2;  // Param length.
		buf[4] = 0;  // Reserved.
		buf[5] = 35; // Temperature (C) current (PCB here).

		put_array_le_to_be16(0, &buf[6]); // PARAMETER CODE
		buf[8] = 1;   // Param control byte.
		buf[9] = 2;   // Param length.
		buf[10] = 0;  // Reserved.
		buf[11] = 60; // Temperature (C) reference.

		buf += 12;
	}

	// Check that a valid page mode data length was requested.
	u32 len = buf - buf0;
	if (!valid_page)
	{
		ums->lun.sense_data = SS_INVALID_FIELD_IN_CDB;

		return UMS_RES_INVALID_ARG;
	}

	put_array_le_to_be16(len - 4, &buf0[2]);

	return len;
}

static int _scsi_mode_sense(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	u8  *buf = (u8 *)bulk_ctxt->bulk_in_buf;
	u8  *buf0 = buf;
	bool valid_page = false;

	u8 pc = ums->cmnd[2] >> 6;
	u8 page_code = ums->cmnd[2] & 0x3F;
	bool changeable_values = pc == 1;
	bool all_pages = page_code == 0x3F;

	if ((ums->cmnd[1] & ~0x08) != 0) // Mask away DBD.
	{
		ums->lun.sense_data = SS_INVALID_FIELD_IN_CDB;

		return UMS_RES_INVALID_ARG;
	}

	if (pc == 3)
	{
		ums->lun.sense_data = SS_SAVING_PARAMETERS_NOT_SUPPORTED;

		return UMS_RES_INVALID_ARG;
	}

	/* Write the mode parameter header.  Fixed values are: default
	 * medium type, no cache control (DPOFUA), and no block descriptors.
	 * The only variable value is the WriteProtect bit.  We will fill in
	 * the mode data length later. */
	memset(buf, 0, 8);
	if (ums->cmnd[0] == SC_MODE_SENSE_6)
	{
		buf[2] = (ums->lun.ro ? 0x80 : 0x00); // WP, DPOFUA.
		buf += 4;
	}
	else // SC_MODE_SENSE_10.
	{
		buf[3] = (ums->lun.ro ? 0x80 : 0x00); // WP, DPOFUA.
		buf += 8;
	}

	// The only page we support is the Caching page.
	// What about x1C
	if (page_code == 0x08 || all_pages)
	{
		valid_page = true;
		buf[0] = 0x08; // Page code.
		buf[1] = 18;   // Page length.
		memset(buf + 2, 0, 18); // Set all parameters to 0.

		// None of the fields are changeable.
		if (!changeable_values)
		{
			// Write Cache enable, Read Cache not disabled, Multiplication Factor off.
			buf[2] = 0x04;

			// Multiplication Factor is disabled, so all values below are 1x LBA.
			put_array_le_to_be16(0xFFFF, &buf[4]);  // Disable Prefetch if >32MB.
			put_array_le_to_be16(0x0000, &buf[6]);  // Minimum Prefetch 0MB.
			put_array_le_to_be16(0xFFFF, &buf[8]);  // Maximum Prefetch 32MB.
			put_array_le_to_be16(0xFFFF, &buf[10]); // Maximum Prefetch ceiling 32MB.
		}

		buf += 20;
	}

	// Check that a valid page mode data length was requested.
	u32 len = buf - buf0;
	if (!valid_page)
	{
		ums->lun.sense_data = SS_INVALID_FIELD_IN_CDB;

		return UMS_RES_INVALID_ARG;
	}

	//  Store the mode data length.
	if (ums->cmnd[0] == SC_MODE_SENSE_6)
		buf0[0] = len - 1;
	else
		put_array_le_to_be16(len - 2, buf0);

	return len;
}

static int _scsi_start_stop(usbd_gadget_ums_t *ums)
{
	int loej, start;

	if (!ums->lun.removable)
	{
		ums->lun.sense_data = SS_INVALID_COMMAND;

		return UMS_RES_INVALID_ARG;
	}
	else if ((ums->cmnd[1] & ~0x01) != 0 || // Mask away Immed.
		(ums->cmnd[4] & ~0x03) != 0)        // Mask LoEj, Start.
	{
		ums->lun.sense_data = SS_INVALID_FIELD_IN_CDB;

		return UMS_RES_INVALID_ARG;
	}

	loej  = ums->cmnd[4] & 0x02;
	start = ums->cmnd[4] & 0x01;

	// We do not support re-mounting.
	if (start)
	{
		if (ums->lun.unmounted)
		{
			ums->lun.sense_data = SS_MEDIUM_NOT_PRESENT;

			return UMS_RES_INVALID_ARG;
		}

		return UMS_RES_OK;
	}

	// Check if we are allowed to unload the media.
	if (ums->lun.prevent_medium_removal)
	{
		ums->set_text(ums->label, "#C7EA46 Status:# Unload attempt prevented");
		ums->lun.sense_data = SS_MEDIUM_REMOVAL_PREVENTED;

		return UMS_RES_INVALID_ARG;
	}

	if (!loej)
		return UMS_RES_OK;

	// Unmount means we exit UMS because of ejection.
	ums->lun.unmounted = 1;

	return UMS_RES_OK;
}

static int _scsi_prevent_allow_removal(usbd_gadget_ums_t *ums)
{
	int prevent;

	if (!ums->lun.removable)
	{
		ums->lun.sense_data = SS_INVALID_COMMAND;

		return UMS_RES_INVALID_ARG;
	}

	prevent = ums->cmnd[4] & 0x01;
	if ((ums->cmnd[4] & ~0x01) != 0) // Mask away Prevent.
	{
		ums->lun.sense_data = SS_INVALID_FIELD_IN_CDB;

		return UMS_RES_INVALID_ARG;
	}

	// Notify for possible unmounting?
	// Normally we sync here but we do synced writes to SDMMC.
	if (ums->lun.prevent_medium_removal && !prevent) { /* Do nothing */ }

	ums->lun.prevent_medium_removal = prevent;

	return UMS_RES_OK;
}

static int _scsi_read_format_capacities(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	u8 *buf = (u8 *)bulk_ctxt->bulk_in_buf;

	buf[0] = buf[1] = buf[2] = 0;
	buf[3] = 8; // Only the Current/Maximum Capacity Descriptor.
	buf += 4;

	put_array_le_to_be32(ums->lun.num_sectors, &buf[0]); // Number of blocks.
	put_array_le_to_be32(UMS_DISK_LBA_SIZE, &buf[4]);    // Block length.
	buf[4] = 0x02; // Current capacity.

	return 12;
}

// Check whether the command is properly formed and whether its data size
// and direction agree with the values we already have.
static int _ums_check_scsi_cmd(usbd_gadget_ums_t *ums, u32 cmnd_size,
	enum data_direction data_dir, u32 mask, int needs_medium)
{
//const char dirletter[4] = {'u', 'o', 'i', 'n'};
DPRINTF("SCSI command: %X;  Dc=%d, D%c=%X;  Hc=%d, H%c=%X\n",
		ums->cmnd[0], cmnd_size, dirletter[(int)ums->data_dir],
		ums->data_size_from_cmnd, ums->cmnd_size,
		dirletter[(int)data_dir], ums->data_size);

	// We can't reply if we don't know the direction and size.
	if (ums->data_size_from_cmnd == 0)
		data_dir = DATA_DIR_NONE;

	// This is a phase error but we continue and only transfer as much we can.
	if (ums->data_size < ums->data_size_from_cmnd)
	{
		ums->data_size_from_cmnd = ums->data_size;
		ums->phase_error = 1;
	}

	ums->residue = ums->data_size;
	ums->usb_amount_left = ums->data_size;

	if (ums->data_dir != data_dir && ums->data_size_from_cmnd > 0)
	{
		ums->phase_error = 1;

		return UMS_RES_INVALID_ARG;
	}

	// Cmd length verification.
	if (cmnd_size != ums->cmnd_size)
	{

		// Special case workaround for Windows and Xbox 360.
		if (cmnd_size <= ums->cmnd_size)
			cmnd_size = ums->cmnd_size;
		else
		{
			ums->phase_error = 1;

			return UMS_RES_INVALID_ARG;
		}
	}

	// check that LUN ums->cmnd[1] >> 5 is 0 because of only one.

	if (ums->cmnd[0] != SC_REQUEST_SENSE)
	{
		ums->lun.sense_data = SS_NO_SENSE;
		ums->lun.sense_data_info = 0;
		ums->lun.info_valid = 0;
	}

	// If a unit attention condition exists, only INQUIRY and REQUEST SENSE
	// commands are allowed.
	if (ums->lun.unit_attention_data != SS_NO_SENSE && ums->cmnd[0] != SC_INQUIRY &&
		ums->cmnd[0] != SC_REQUEST_SENSE)
	{
		ums->lun.sense_data = ums->lun.unit_attention_data;
		ums->lun.unit_attention_data = SS_NO_SENSE;

		return UMS_RES_INVALID_ARG;
	}

	// Check that only command bytes listed in the mask are set.
	ums->cmnd[1] &= 0x1F; // Mask away the LUN.
	for (u32 i = 1; i < cmnd_size; ++i)
	{
		if (ums->cmnd[i] && !(mask & BIT(i)))
		{
			ums->lun.sense_data = SS_INVALID_FIELD_IN_CDB;

			return UMS_RES_INVALID_ARG;
		}
	}

	// If the medium isn't mounted and the command needs to access it, return an error.
	if (ums->lun.unmounted && needs_medium)
	{
		ums->lun.sense_data = SS_MEDIUM_NOT_PRESENT;

		return UMS_RES_INVALID_ARG;
	}

	return UMS_RES_OK;
}

static int _ums_parse_scsi_cmd(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	u32 len;
	int reply = UMS_RES_INVALID_ARG;

	ums->phase_error = 0;
	ums->short_packet_received = 0;

	switch (ums->cmnd[0])
	{
	case SC_INQUIRY:
		ums->data_size_from_cmnd = ums->cmnd[4];
		u32 mask = (1<<4);
		if (ums->cmnd[1] == 1 && ums->cmnd[2] == 0x80) // Inquiry S/N.
			mask = (1<<1) | (1<<2) | (1<<4);
		reply = _ums_check_scsi_cmd(ums, 6, DATA_DIR_TO_HOST, mask, 0);
		if (reply == 0)
			reply = _scsi_inquiry(ums, bulk_ctxt);
		break;

	case SC_LOG_SENSE:
		ums->data_size_from_cmnd = get_array_be_to_le16(&ums->cmnd[7]);
		reply = _ums_check_scsi_cmd(ums, 10, DATA_DIR_TO_HOST, (1<<1) | (1<<2) | (3<<7), 0);
		if (reply == 0)
			reply = _scsi_log_sense(ums, bulk_ctxt);
		break;

	case SC_MODE_SELECT_6:
		ums->data_size_from_cmnd = ums->cmnd[4];
		reply = _ums_check_scsi_cmd(ums, 6, DATA_DIR_FROM_HOST, (1<<1) | (1<<4), 0);
		if (reply == 0)
		{
			// We don't support MODE SELECT.
			ums->lun.sense_data = SS_INVALID_COMMAND;
			reply = UMS_RES_INVALID_ARG;
		}
		break;

	case SC_MODE_SELECT_10:
		ums->data_size_from_cmnd = get_array_be_to_le16(&ums->cmnd[7]);
		reply = _ums_check_scsi_cmd(ums, 10, DATA_DIR_FROM_HOST, (1<<1) | (3<<7), 0);
		if (reply == 0)
		{
			// We don't support MODE SELECT.
			ums->lun.sense_data = SS_INVALID_COMMAND;
			reply = UMS_RES_INVALID_ARG;
		}
		break;

	case SC_MODE_SENSE_6:
		ums->data_size_from_cmnd = ums->cmnd[4];
		reply = _ums_check_scsi_cmd(ums, 6, DATA_DIR_TO_HOST,  (1<<1) | (1<<2) | (1<<4), 0);
		if (reply == 0)
			reply = _scsi_mode_sense(ums, bulk_ctxt);
		break;

	case SC_MODE_SENSE_10:
		ums->data_size_from_cmnd = get_array_be_to_le16(&ums->cmnd[7]);
		reply = _ums_check_scsi_cmd(ums, 10, DATA_DIR_TO_HOST, (1<<1) | (1<<2) | (3<<7), 0);
		if (reply == 0)
			reply = _scsi_mode_sense(ums, bulk_ctxt);
		break;

	case SC_PREVENT_ALLOW_MEDIUM_REMOVAL:
		ums->data_size_from_cmnd = 0;
		reply = _ums_check_scsi_cmd(ums, 6, DATA_DIR_NONE, (1<<4), 0);
		if (reply == 0)
			reply = _scsi_prevent_allow_removal(ums);
		break;

	case SC_READ_6:
		len = ums->cmnd[4];
		ums->data_size_from_cmnd = (len == 0 ? 256 : len) << UMS_DISK_LBA_SHIFT;
		reply = _ums_check_scsi_cmd(ums, 6, DATA_DIR_TO_HOST, (7<<1) | (1<<4), 1);
		if (reply == 0)
			reply = _scsi_read(ums, bulk_ctxt);
		break;

	case SC_READ_10:
		ums->data_size_from_cmnd = get_array_be_to_le16(&ums->cmnd[7]) << UMS_DISK_LBA_SHIFT;
		reply = _ums_check_scsi_cmd(ums, 10, DATA_DIR_TO_HOST, (1<<1) | (0xf<<2) | (3<<7), 1);
		if (reply == 0)
			reply = _scsi_read(ums, bulk_ctxt);
		break;

	case SC_READ_12:
		ums->data_size_from_cmnd = get_array_be_to_le32(&ums->cmnd[6]) << UMS_DISK_LBA_SHIFT;
		reply = _ums_check_scsi_cmd(ums, 12, DATA_DIR_TO_HOST, (1<<1) | (0xf<<2) | (0xf<<6), 1);
		if (reply == 0)
			reply = _scsi_read(ums, bulk_ctxt);
		break;

	case SC_READ_CAPACITY:
		ums->data_size_from_cmnd = 8;
		reply = _ums_check_scsi_cmd(ums, 10, DATA_DIR_TO_HOST, (0xf<<2) | (1<<8), 1);
		if (reply == 0)
			reply = _scsi_read_capacity(ums, bulk_ctxt);
		break;
	case SC_READ_FORMAT_CAPACITIES:
		ums->data_size_from_cmnd = get_array_be_to_le16(&ums->cmnd[7]);
		reply = _ums_check_scsi_cmd(ums, 10, DATA_DIR_TO_HOST, (3<<7), 1);
		if (reply == 0)
			reply = _scsi_read_format_capacities(ums, bulk_ctxt);
		break;

	case SC_REQUEST_SENSE:
		ums->data_size_from_cmnd = ums->cmnd[4];
		reply = _ums_check_scsi_cmd(ums, 6, DATA_DIR_TO_HOST, (1<<4), 0);
		if (reply == 0)
			reply = _scsi_request_sense(ums, bulk_ctxt);
		break;

	case SC_START_STOP_UNIT:
		ums->data_size_from_cmnd = 0;
		reply = _ums_check_scsi_cmd(ums, 6, DATA_DIR_NONE, (1<<1) | (1<<4), 0);
		if (reply == 0)
			reply = _scsi_start_stop(ums);
		break;

	case SC_SYNCHRONIZE_CACHE:
		ums->data_size_from_cmnd = 0;
		reply = _ums_check_scsi_cmd(ums, 10, DATA_DIR_NONE, (0xf<<2) | (3<<7), 1);
		if (reply == 0)
			reply = 0; // Don't bother
		break;

	case SC_TEST_UNIT_READY:
		ums->data_size_from_cmnd = 0;
		reply = _ums_check_scsi_cmd(ums, 6, DATA_DIR_NONE, 0, 1);
		break;

	// This command is used by Windows. We support a minimal version and BytChk must be 0.
	case SC_VERIFY:
		ums->data_size_from_cmnd = 0;
		reply = _ums_check_scsi_cmd(ums, 10, DATA_DIR_NONE, (1<<1) | (0xf<<2) | (3<<7), 1);
		if (reply == 0)
			reply = _scsi_verify(ums, bulk_ctxt);
		break;

	case SC_WRITE_6:
		len = ums->cmnd[4];
		ums->data_size_from_cmnd = (len == 0 ? 256 : len) << UMS_DISK_LBA_SHIFT;
		reply = _ums_check_scsi_cmd(ums, 6, DATA_DIR_FROM_HOST, (7<<1) | (1<<4), 1);
		if (reply == 0)
			reply = _scsi_write(ums, bulk_ctxt);
		break;

	case SC_WRITE_10:
		ums->data_size_from_cmnd = get_array_be_to_le16(&ums->cmnd[7]) << UMS_DISK_LBA_SHIFT;
		reply = _ums_check_scsi_cmd(ums, 10, DATA_DIR_FROM_HOST, (1<<1) | (0xf<<2) | (3<<7), 1);
		if (reply == 0)
			reply = _scsi_write(ums, bulk_ctxt);
		break;

	case SC_WRITE_12:
		ums->data_size_from_cmnd = get_array_be_to_le32(&ums->cmnd[6]) << UMS_DISK_LBA_SHIFT;
		reply = _ums_check_scsi_cmd(ums, 12, DATA_DIR_FROM_HOST, (1<<1) | (0xf<<2) | (0xf<<6), 1);
		if (reply == 0)
			reply = _scsi_write(ums, bulk_ctxt);
		break;

	// Mandatory commands that we don't implement. No need.
	case SC_READ_HEADER:
	case SC_READ_TOC:
	case SC_FORMAT_UNIT:
	case SC_RELEASE:
	case SC_RESERVE:
	case SC_SEND_DIAGNOSTIC:
	default:
		ums->data_size_from_cmnd = 0;
		reply = _ums_check_scsi_cmd(ums, ums->cmnd_size, DATA_DIR_UNKNOWN, 0xFF, 0);
		if (reply == 0)
		{
			ums->lun.sense_data = SS_INVALID_COMMAND;
			reply = UMS_RES_INVALID_ARG;
		}
		break;
	}

	if (reply == UMS_RES_INVALID_ARG)
		reply = 0;    // Error reply length.

	// Set up reply buffer for finish_reply(). Otherwise it's already set.
	if (reply >= 0 && ums->data_dir == DATA_DIR_TO_HOST)
	{
		reply = MIN((u32)reply, ums->data_size_from_cmnd);
		bulk_ctxt->bulk_in_length = reply;
		bulk_ctxt->bulk_in_buf_state = BUF_STATE_FULL;
		ums->residue -= reply;
	}

	return UMS_RES_OK;
}

static int pad_with_zeros(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	bulk_ctxt->bulk_in_buf_state = BUF_STATE_EMPTY; // For the first iteration.
	u32 current_len_to_keep = bulk_ctxt->bulk_in_length;
	ums->usb_amount_left = current_len_to_keep + ums->residue;

	while (ums->usb_amount_left > 0)
	{
		u32 nsend = MIN(ums->usb_amount_left, USB_EP_BUFFER_MAX_SIZE);
		memset(bulk_ctxt->bulk_in_buf + current_len_to_keep, 0, nsend - current_len_to_keep);
		bulk_ctxt->bulk_in_length = nsend;
		_ums_transfer_start(ums, bulk_ctxt, bulk_ctxt->bulk_in, USB_XFER_SYNCED_DATA);
		ums->usb_amount_left -= nsend;
		current_len_to_keep = 0;
	}

	return UMS_RES_OK;
}

static int throw_away_data(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	if (bulk_ctxt->bulk_out_buf_state != BUF_STATE_EMPTY || ums->usb_amount_left > 0)
	{
		// Try to submit another request if we need one.
		if (bulk_ctxt->bulk_out_buf_state == BUF_STATE_EMPTY && ums->usb_amount_left > 0)
		{
			u32 amount = MIN(ums->usb_amount_left, USB_EP_BUFFER_MAX_SIZE);

			bulk_ctxt->bulk_out_length = amount;
			_ums_transfer_start(ums, bulk_ctxt, bulk_ctxt->bulk_out, USB_XFER_SYNCED_DATA);
			ums->usb_amount_left -= amount;

			return UMS_RES_OK;
		}

		// Throw away the data in a filled buffer.
		if (bulk_ctxt->bulk_out_buf_state == BUF_STATE_FULL)
			bulk_ctxt->bulk_out_buf_state = BUF_STATE_EMPTY;

		// A short packet or an error ends everything.
		if (bulk_ctxt->bulk_out_length_actual != bulk_ctxt->bulk_out_length ||
			bulk_ctxt->bulk_out_status != USB_RES_OK)
		{
			raise_exception(ums, UMS_STATE_ABORT_BULK_OUT);
			return UMS_RES_PROT_FATAL;
		}
	}
	return UMS_RES_OK;
}

static int finish_reply(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	int rc = UMS_RES_OK;

	switch (ums->data_dir) {
	case DATA_DIR_NONE:
		break; // Nothing to send.

	// If this is a CB or CBI with an unknown command, we mustn't
	// try to send or receive any data. Stall if we can and wait reset.
	case DATA_DIR_UNKNOWN:
		if (ums->can_stall)
		{
			ums_set_stall(bulk_ctxt->bulk_out);
			rc = ums_set_stall(bulk_ctxt->bulk_in);
			ums->set_text(ums->label, "#FFDD00 Error:# Direction unknown. Stalled both EP!");
		} // Else do nothing.
		break;

	// All but the last buffer of data have already been sent.
	case DATA_DIR_TO_HOST:
		if (ums->data_size)
		{
			// If there's no residue, simply send the last buffer.
			if (!ums->residue)
			{
				_ums_transfer_start(ums, bulk_ctxt, bulk_ctxt->bulk_in, USB_XFER_SYNCED_DATA);

			/* For Bulk-only, if we're allowed to stall then send the
			 * short packet and halt the bulk-in endpoint.  If we can't
			 * stall, pad out the remaining data with 0's. */
			}
			else if (ums->can_stall)
			{
				_ums_transfer_start(ums, bulk_ctxt, bulk_ctxt->bulk_in, USB_XFER_SYNCED_DATA);
				rc = ums_set_stall(bulk_ctxt->bulk_in);
				ums->set_text(ums->label, "#FFDD00 Error:# Residue. Stalled EP IN!");
			}
			else
				rc = pad_with_zeros(ums, bulk_ctxt);
		}

		// In case we used SDMMC transfer, reset the buffer address.
		_ums_reset_buffer(bulk_ctxt, bulk_ctxt->bulk_in);
		break;

	// We have processed all we want from the data the host has sent.
	// There may still be outstanding bulk-out requests.
	case DATA_DIR_FROM_HOST:
		if (ums->residue)
		{
			if (ums->short_packet_received) // Did the host stop sending unexpectedly early?
			{
				raise_exception(ums, UMS_STATE_ABORT_BULK_OUT);
				rc = UMS_RES_PROT_FATAL;
			}
			else // We can't stall. Read in the excess data and throw it away.
				rc = throw_away_data(ums, bulk_ctxt);
		}

		break;
	}

	return rc;
}

/*
 * Medium ejection heuristics.
 *
 * Windows:
 * Uses Start/Stop Unit. Only Stop with LoEj. Observed ONLY on very specific windows machines.
 * Uses Prevent/Allow Medium Removal. (For big reads and ANY write.) //////Except trivial writes. Needs check with prefetch ON
 * Sends Test Unit Ready every 1s at idle. (Needs 1 EP Timeout protection: 2s)
 * Does not send data when ejects. In the case it does,
 *  it loops into Request Sense and Test Unit Ready when ejects.
 * Line always at SE0 and only goes in J-State when it ejects.
 *
 * Linux:
 * Uses Start/Stop Unit. Stops with LoEj when Media prevention is off.
 * Uses Prevent/Allow Medium Removal. (For big read and any write.)
 * Sends Test Unit Ready every 2s at idle. (Needs 2 EP Timeouts protection: 4s)
 * Loops into Request Sense and Test Unit Ready when ejects.
 * Line always at SE0.
 *
 * Mac OS:
 * Uses Start/Stop. Stops with LoEj when Allow Medium Removal is enabled.
 * Uses Prevent/Allow Medium Removal. (Properly. Enables at mount and only disables it when ejects.)
 * Does not send Test Unit Ready at idle. But Prevent Medium Removal is enabled.
 * Loops into Request Sense and Test Unit Ready when ejects.
 * Line always at SE0.
 */

static int received_cbw(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	// Was this a real packet?  Should it be ignored?
	if (bulk_ctxt->bulk_out_status || bulk_ctxt->bulk_out_ignore || ums->lun.unmounted)
	{
		if (bulk_ctxt->bulk_out_status || ums->lun.unmounted)
		{
			DPRINTF("USB: EP timeout (%d)\n", bulk_ctxt->bulk_out_status);
			// In case we disconnected, exit UMS.
			// Raise timeout if removable and didn't got a unit ready command inside 4s.
			if (bulk_ctxt->bulk_out_status == USB2_ERROR_XFER_EP_DISABLED ||
				(bulk_ctxt->bulk_out_status == USB_ERROR_TIMEOUT && ums->lun.removable && !ums->lun.prevent_medium_removal))
			{
				if (bulk_ctxt->bulk_out_status == USB_ERROR_TIMEOUT)
				{
					if (usb_ops.usb_device_get_port_in_sleep())
					{
						ums->set_text(ums->label, "#C7EA46 Status:# EP in sleep");
						ums->timeouts += 14;
					}
					else if (!ums->xusb) // Timeout only on USB2.
					{
						ums->timeouts += 4;
						DPRINTF("USB: EP removable\n");
					}
				}
				else
				{
					gfx_printf("USB: EP disabled\n");
					msleep(500);
					ums->timeouts += 4;
				}
			}

			if (ums->lun.unmounted)
			{
				ums->set_text(ums->label, "#C7EA46 Status:# Medium unmounted");
				ums->timeouts++;
				if (!bulk_ctxt->bulk_out_status)
					ums->timeouts += 3;
			}

			if (ums->timeouts > 20)
				raise_exception(ums, UMS_STATE_EXIT);
		}

		if (bulk_ctxt->bulk_out_status || bulk_ctxt->bulk_out_ignore)
			return UMS_RES_INVALID_ARG;
	}

	// Clear request flag to allow a new one to be queued.
	ums->cbw_req_queued = false;

	// Is the CBW valid?
	bulk_recv_pkt_t *cbw = (bulk_recv_pkt_t *)bulk_ctxt->bulk_out_buf;
	if (bulk_ctxt->bulk_out_length_actual != USB_BULK_CB_WRAP_LEN || cbw->Signature != USB_BULK_CB_SIG)
	{
		gfx_printf("USB: invalid CBW: len %X sig 0x%X\n", bulk_ctxt->bulk_out_length_actual, cbw->Signature);

		/*
		 * The Bulk-only spec says we MUST stall the IN endpoint
		 * (6.6.1), so it's unavoidable.  It also says we must
		 * retain this state until the next reset, but there's
		 * no way to tell the controller driver it should ignore
		 * Clear-Feature(HALT) requests.
		 *
		 * We aren't required to halt the OUT endpoint; instead
		 * we can simply accept and discard any data received
		 * until the next reset.
		 */
		ums_wedge_bulk_in_endpoint(ums);
		bulk_ctxt->bulk_out_ignore = 1;
		return UMS_RES_INVALID_ARG;
	}

	// Is the CBW meaningful?
	if (cbw->Lun >= UMS_MAX_LUN || cbw->Flags & ~USB_BULK_IN_FLAG ||
			cbw->Length == 0 || cbw->Length > SCSI_MAX_CMD_SZ)
	{
		gfx_printf("USB: non-meaningful CBW: lun = %X, flags = 0x%X, cmdlen %X\n",
			cbw->Lun, cbw->Flags, cbw->Length);

		/* We can do anything we want here, so let's stall the
		 * bulk pipes if we are allowed to. */
		if (ums->can_stall)
		{
			ums_set_stall(bulk_ctxt->bulk_out);
			ums_set_stall(bulk_ctxt->bulk_in);
			ums->set_text(ums->label, "#FFDD00 Error:# CBW unknown - Stalled both EP!");
		}

		return UMS_RES_INVALID_ARG;
	}

	// Save the command for later.
	ums->cmnd_size = cbw->Length;
	memcpy(ums->cmnd, cbw->CDB, ums->cmnd_size);

	if (cbw->Flags & USB_BULK_IN_FLAG)
		ums->data_dir = DATA_DIR_TO_HOST;
	else
		ums->data_dir = DATA_DIR_FROM_HOST;

	ums->data_size = cbw->DataTransferLength;

	if (ums->data_size == 0)
		ums->data_dir = DATA_DIR_NONE;

	ums->lun_idx = cbw->Lun;
	ums->tag = cbw->Tag;

	if (!ums->lun.unmounted)
		ums->timeouts = 0;

	return UMS_RES_OK;
}

static int get_next_command(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	int rc = UMS_RES_OK;

	/* Wait for the next buffer to become available */
	// while (bulk_ctxt->bulk_out_buf_state != BUF_STATE_EMPTY)
	// {
	// 	//wait irq.
	// }

	bulk_ctxt->bulk_out_length = USB_BULK_CB_WRAP_LEN;

	// Queue a request to read a Bulk-only CBW.
	if (!ums->cbw_req_queued)
		_ums_transfer_start(ums, bulk_ctxt, bulk_ctxt->bulk_out, USB_XFER_SYNCED_CMD);
	else
		_ums_transfer_finish(ums, bulk_ctxt, bulk_ctxt->bulk_out, USB_XFER_SYNCED_CMD);

	/*
	 * On XUSB do not allow multiple requests for CBW to be done.
	 * This avoids an issue with some XHCI controllers and OS combos (e.g. ASMedia and Linux/Mac OS)
	 * which confuse that and concatenate an old CBW request with another write request (SCSI Write)
	 * and create a babble error (transmit overflow).
	 */
	if (ums->xusb)
		ums->cbw_req_queued = true;

	/* We will drain the buffer in software, which means we
	 * can reuse it for the next filling.  No need to advance
	 * next_buffhd_to_fill. */

	/* Wait for the CBW to arrive */
	// while (bulk_ctxt->bulk_out_buf_state != BUF_STATE_FULL)
	// {
	// 	//wait irq.
	// }

	rc = received_cbw(ums, bulk_ctxt);
	bulk_ctxt->bulk_out_buf_state = BUF_STATE_EMPTY;

	return rc;
}

static void send_status(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	u8  status = USB_STATUS_PASS;
	u32 sd = ums->lun.sense_data;

	if (ums->phase_error)
	{
		ums->set_text(ums->label, "#FFDD00 Error:# Phase-error!");
		status = USB_STATUS_PHASE_ERROR;
		sd = SS_INVALID_COMMAND;
	}
	else if (sd != SS_NO_SENSE)
	{
		DPRINTF("USB: CMD fail\n");
		status = USB_STATUS_FAIL;
		DPRINTF("USB:   Sense: SK x%02X, ASC x%02X, ASCQ x%02X; info x%X\n",
			SK(sd), ASC(sd), ASCQ(sd), ums->lun.sense_data_info);
	}

	// Store and send the Bulk-only CSW.
	bulk_send_pkt_t *csw = (bulk_send_pkt_t *)bulk_ctxt->bulk_in_buf;

	csw->Signature = USB_BULK_CS_SIG;
	csw->Tag = ums->tag;
	csw->Residue = ums->residue;
	csw->Status = status;

	bulk_ctxt->bulk_in_length = USB_BULK_CS_WRAP_LEN;
	_ums_transfer_start(ums, bulk_ctxt, bulk_ctxt->bulk_in, USB_XFER_SYNCED_CMD);
}

static void handle_exception(usbd_gadget_ums_t *ums, bulk_ctxt_t *bulk_ctxt)
{
	enum ums_state old_state;

	// Clear out the controller's fifos.
	ums_flush_endpoint(bulk_ctxt->bulk_in);
	ums_flush_endpoint(bulk_ctxt->bulk_out);

	/* Reset the I/O buffer states and pointers, the SCSI
	 * state, and the exception.  Then invoke the handler. */

	bulk_ctxt->bulk_in_buf_state = BUF_STATE_EMPTY;
	bulk_ctxt->bulk_out_buf_state = BUF_STATE_EMPTY;

	old_state = ums->state;

	if (old_state != UMS_STATE_ABORT_BULK_OUT)
	{
		ums->lun.prevent_medium_removal = 0;
		ums->lun.sense_data = SS_NO_SENSE;
		ums->lun.unit_attention_data = SS_NO_SENSE;
		ums->lun.sense_data_info = 0;
		ums->lun.info_valid = 0;
	}

	ums->state = UMS_STATE_NORMAL;

	// Carry out any extra actions required for the exception.
	switch (old_state)
	{
	case UMS_STATE_NORMAL:
		break;
	case UMS_STATE_ABORT_BULK_OUT:
		send_status(ums, bulk_ctxt);
		break;

	case UMS_STATE_PROTOCOL_RESET:
		/* In case we were forced against our will to halt a
		 * bulk endpoint, clear the halt now.  (The SuperH UDC
		 * requires this.) */
		if (bulk_ctxt->bulk_out_ignore)
		{
			bulk_ctxt->bulk_out_ignore = 0;
			ums_clear_stall(bulk_ctxt->bulk_in);
		}
		ums->lun.unit_attention_data = SS_RESET_OCCURRED;
		break;

	case UMS_STATE_EXIT:
		ums->state = UMS_STATE_TERMINATED;	// Stop the thread.
		break;

	default:
		break;
	}
}

static inline void _system_maintainance(usbd_gadget_ums_t *ums)
{
	static u32 timer_dram = 0;
	static u32 timer_status_bar = 0;

	u32 time = get_tmr_ms();

	if (timer_status_bar < time)
	{
		ums->system_maintenance(true);
		timer_status_bar = get_tmr_ms() + 30000;
	}
	else if (timer_dram < time)
	{
		minerva_periodic_training();
		timer_dram = get_tmr_ms() + EMC_PERIODIC_TRAIN_MS;
	}
}

int usb_device_gadget_ums(usb_ctxt_t *usbs)
{
	int res = 0;
	sdmmc_t sdmmc;
	sdmmc_storage_t storage;
	usbd_gadget_ums_t ums = {0};

	// Get USB Controller ops.
	if (hw_get_chip_id() == GP_HIDREV_MAJOR_T210)
		usb_device_get_ops(&usb_ops);
	else
	{
		ums.xusb = true;
		xusb_device_get_ops(&usb_ops);
	}

	usbs->set_text(usbs->label, "#C7EA46 Status:# Started USB");

	if (usb_ops.usb_device_init())
	{
		usb_ops.usbd_end(false, true);
		return 1;
	}

	ums.state = UMS_STATE_NORMAL;
	ums.can_stall = 0;

	ums.bulk_ctxt.bulk_in = USB_EP_BULK_IN;
	ums.bulk_ctxt.bulk_in_buf = (u8 *)USB_EP_BULK_IN_BUF_ADDR;

	ums.bulk_ctxt.bulk_out = USB_EP_BULK_OUT;
	ums.bulk_ctxt.bulk_out_buf = (u8 *)USB_EP_BULK_OUT_BUF_ADDR;

	// Set LUN parameters.
	ums.lun.ro = usbs->ro;
	ums.lun.type = usbs->type;
	ums.lun.partition = usbs->partition;
	ums.lun.offset = usbs->offset;
	ums.lun.removable = 1; // Always removable to force OSes to use prevent media removal.
	ums.lun.unit_attention_data = SS_RESET_OCCURRED;

	// Set system functions
	ums.label = usbs->label;
	ums.set_text = usbs->set_text;
	ums.system_maintenance = usbs->system_maintenance;

	ums.set_text(ums.label, "#C7EA46 Status:# Mounting disk");

	// Initialize sdmmc.
	if (usbs->type == MMC_SD)
	{
		sd_end();
		sd_mount();
		sd_unmount();
		ums.lun.sdmmc = &sd_sdmmc;
		ums.lun.storage = &sd_storage;
	}
	else
	{
		ums.lun.sdmmc = &sdmmc;
		ums.lun.storage = &storage;
		sdmmc_storage_init_mmc(ums.lun.storage, ums.lun.sdmmc, SDMMC_BUS_WIDTH_8, SDHCI_TIMING_MMC_HS400);
		sdmmc_storage_set_mmc_partition(ums.lun.storage, ums.lun.partition - 1);
	}

	ums.set_text(ums.label, "#C7EA46 Status:# Waiting for connection");

	// Initialize Control Endpoint.
	if (usb_ops.usb_device_enumerate(USB_GADGET_UMS))
		goto error;

	ums.set_text(ums.label, "#C7EA46 Status:# Waiting for LUN");

	if (usb_ops.usb_device_class_send_max_lun(0)) // One device for now.
		goto error;

	ums.set_text(ums.label, "#C7EA46 Status:# Started UMS");

	if (usbs->sectors)
		ums.lun.num_sectors = usbs->sectors;
	else
		ums.lun.num_sectors = ums.lun.storage->sec_cnt;

	do
	{
		// Do DRAM training and update system tasks.
		_system_maintainance(&ums);

		// Check for force unmount button combo.
		if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
		{
			// Check if we are allowed to unload the media.
			if (ums.lun.prevent_medium_removal)
				ums.set_text(ums.label, "#C7EA46 Status:# Unload attempt prevented");
			else
				break;
		}

		if (ums.state != UMS_STATE_NORMAL)
		{
			handle_exception(&ums, &ums.bulk_ctxt);
			continue;
		}

		ums_handle_ep0_ctrl(&ums);

		if (get_next_command(&ums, &ums.bulk_ctxt) || (ums.state > UMS_STATE_NORMAL))
			continue;

		ums_handle_ep0_ctrl(&ums);

		if (_ums_parse_scsi_cmd(&ums, &ums.bulk_ctxt) || (ums.state > UMS_STATE_NORMAL))
			continue;

		ums_handle_ep0_ctrl(&ums);

		if (finish_reply(&ums, &ums.bulk_ctxt) || (ums.state > UMS_STATE_NORMAL))
			continue;

		send_status(&ums, &ums.bulk_ctxt);
	} while (ums.state != UMS_STATE_TERMINATED);

	if (ums.lun.prevent_medium_removal)
		ums.set_text(ums.label, "#FFDD00 Error:# Disk unsafely ejected");
	else
		ums.set_text(ums.label, "#C7EA46 Status:# Disk ejected");
	goto exit;

error:
	ums.set_text(ums.label, "#FFDD00 Error:# Timed out or canceled!");
	res = 1;

exit:
	if (ums.lun.type == MMC_EMMC)
		sdmmc_storage_end(ums.lun.storage);

	usb_ops.usbd_end(true, false);

	return res;
}
