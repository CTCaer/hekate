/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs                          */
/* (C) ChaN, 2016                                                        */
/* (C) CTCaer, 2018-2025                                                 */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <string.h>

#include <bdk.h>

#include <libs/fatfs/diskio.h>	/* FatFs lower layer API */

static u32 sd_rsvd_sectors = 0;
static u32 ramdisk_sectors = 0;
static u32 bis_sectors     = 0;
static u32 emummc_sectors  = 0;

static bool bis_write_allowed = false;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return 0;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	return 0;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	switch (pdrv)
	{
	case DRIVE_SD:
		return sdmmc_storage_read(&sd_storage, sector, count, (void *)buff) ? RES_OK : RES_ERROR;
	case DRIVE_RAM:
		return ram_disk_read(sector, count, (void *)buff);
	case DRIVE_EMMC:
		return sdmmc_storage_read(&emmc_storage, sector, count, (void *)buff) ? RES_OK : RES_ERROR;
	case DRIVE_BIS:
	case DRIVE_EMU:
		return nx_emmc_bis_read(sector, count, (void *)buff) ? RES_OK : RES_ERROR;
	}

	return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	switch (pdrv)
	{
	case DRIVE_SD:
		return sdmmc_storage_write(&sd_storage, sector, count, (void *)buff) ? RES_OK : RES_ERROR;
	case DRIVE_RAM:
		return ram_disk_write(sector, count, (void *)buff);
	case DRIVE_EMMC:
		return RES_WRPRT;
	case DRIVE_BIS:
	case DRIVE_EMU:
		if (pdrv == DRIVE_BIS && !bis_write_allowed)
			return RES_WRPRT;
		return nx_emmc_bis_write(sector, count, (void *)buff) ? RES_OK : RES_ERROR;
	}

	return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DWORD *buf = (DWORD *)buff;

	switch (pdrv)
	{
	case DRIVE_SD:
		switch (cmd)
		{
		case GET_SECTOR_COUNT:
			*buf = sd_storage.sec_cnt - sd_rsvd_sectors;
			break;
		case GET_BLOCK_SIZE:
			*buf = 32768; // Align to 16MB.
			break;
		}
		break;

	case DRIVE_RAM:
		switch (cmd)
		{
		case GET_SECTOR_COUNT:
			*buf = ramdisk_sectors;
			break;
		case GET_BLOCK_SIZE:
			*buf = 2048; // Align to 1MB.
			break;
		}
		break;

	case DRIVE_BIS:
		switch (cmd)
		{
		case GET_SECTOR_COUNT:
			*buf = bis_sectors;
			break;
		case GET_BLOCK_SIZE:
			*buf = 32768; // Align to 16MB.
			break;
		}
		break;

	case DRIVE_EMU:
		switch (cmd)
		{
		case GET_SECTOR_COUNT:
			*buf = emummc_sectors;
			break;
		case GET_BLOCK_SIZE:
			*buf = 16384; // Align to 8MB (With BOOT0/1 data will be at 16MB BU).
			break;
		}
		break;

	default: // Catch all for unknown devices.
		switch (cmd)
		{
		case CTRL_SYNC:
			break;
		case GET_SECTOR_COUNT:
		case GET_BLOCK_SIZE:
			*buf = 0; // Zero value to force default or abort.
			break;
		}
		break;
	}

	return RES_OK;
}

DRESULT disk_set_info (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DWORD *buf = (DWORD *)buff;

	if (cmd == SET_SECTOR_COUNT)
	{
		switch (pdrv)
		{
		case DRIVE_SD:
			sd_rsvd_sectors = *buf;
			break;

		case DRIVE_RAM:
			ramdisk_sectors = *buf;
			break;

		case DRIVE_BIS:
			bis_sectors = *buf;
			break;

		case DRIVE_EMU:
			emummc_sectors = *buf;
			break;
		}
	}
	else if (cmd == SET_WRITE_PROTECT && pdrv == DRIVE_BIS)
		bis_write_allowed = *(bool *)buff;

	return RES_OK;
}
