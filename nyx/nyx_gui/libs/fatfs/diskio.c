/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <string.h>
#include "diskio.h"		/* FatFs lower layer API */
#include "../../storage/sdmmc.h"

#define SDMMC_UPPER_BUFFER 0xB8000000
#define DRAM_START         0x80000000

extern sdmmc_storage_t sd_storage;

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
	// Ensure that buffer resides in DRAM and it's DMA aligned.
	if (((u32)buff >= DRAM_START) && !((u32)buff % 8))
		return sdmmc_storage_read(&sd_storage, sector, count, buff) ? RES_OK : RES_ERROR;
	u8 *buf = (u8 *)SDMMC_UPPER_BUFFER;
	if (sdmmc_storage_read(&sd_storage, sector, count, buf))
	{
		memcpy(buff, buf, 512 * count);
		return RES_OK;
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
	// Ensure that buffer resides in DRAM and it's DMA aligned.
	if (((u32)buff >= DRAM_START) && !((u32)buff % 8))
		return sdmmc_storage_write(&sd_storage, sector, count, (void *)buff) ? RES_OK : RES_ERROR;
	u8 *buf = (u8 *)SDMMC_UPPER_BUFFER;
	memcpy(buf, buff, 512 * count);
	if (sdmmc_storage_write(&sd_storage, sector, count, buf))
		return RES_OK;
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
	return RES_OK;
}
