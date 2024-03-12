/*------------------------------------------------------------------------*/
/* Sample Code of OS Dependent Functions for FatFs                        */
/* (C) ChaN, 2018                                                         */
/* (C) CTCaer, 2018-2024                                                  */
/*------------------------------------------------------------------------*/

#include <bdk.h>

#include <libs/fatfs/ff.h>

#if FF_USE_LFN == 3	/* Dynamic memory allocation */

/*------------------------------------------------------------------------*/
/* Allocate a memory block                                                */
/*------------------------------------------------------------------------*/

void* ff_memalloc (	/* Returns pointer to the allocated memory block (null if not enough core) */
	UINT msize		/* Number of bytes to allocate */
)
{
	// Ensure size is aligned to SDMMC block size.
	return malloc(ALIGN(msize, SDMMC_DAT_BLOCKSIZE));	/* Allocate a new memory block with POSIX API */
}


/*------------------------------------------------------------------------*/
/* Free a memory block                                                    */
/*------------------------------------------------------------------------*/

void ff_memfree (
	void* mblock	/* Pointer to the memory block to free (nothing to do if null) */
)
{
	free(mblock);	/* Free the memory block with POSIX API */
}

#endif

#if FF_FS_NORTC == 0

/*------------------------------------------------------------------------*/
/* Get real time clock                                                    */
/*------------------------------------------------------------------------*/

DWORD get_fattime (
	void
)
{
	rtc_time_t time;

	max77620_rtc_get_time_adjusted(&time);

	return (((DWORD)(time.year - 1980) << 25) | ((DWORD)time.month << 21) | ((DWORD)time.day << 16) |
		((DWORD)time.hour << 11) | ((DWORD)time.min << 5) | (time.sec >> 1));
}

#endif
