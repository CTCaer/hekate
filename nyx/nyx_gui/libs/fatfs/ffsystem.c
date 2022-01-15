/*------------------------------------------------------------------------*/
/* Sample Code of OS Dependent Functions for FatFs                        */
/* (C) ChaN, 2018                                                          */
/* (C) CTCaer, 2018                                                       */
/*------------------------------------------------------------------------*/

#include <bdk.h>

#include <libs/fatfs/ff.h>
#include "../../config.h"

extern nyx_config n_cfg;

#if FF_USE_LFN == 3	/* Dynamic memory allocation */

/*------------------------------------------------------------------------*/
/* Allocate a memory block                                                */
/*------------------------------------------------------------------------*/

void* ff_memalloc (	/* Returns pointer to the allocated memory block (null if not enough core) */
	UINT msize		/* Number of bytes to allocate */
)
{
	return malloc(msize);	/* Allocate a new memory block with POSIX API */
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

	max77620_rtc_get_time(&time);
	if (n_cfg.timeoff)
	{
		u32 epoch = (u32)((s32)max77620_rtc_date_to_epoch(&time) + (s32)n_cfg.timeoff);
		max77620_rtc_epoch_to_date(epoch, &time);
	}

	return (((DWORD)(time.year - 1980) << 25) | ((DWORD)time.month << 21) | ((DWORD)time.day << 16) |
		((DWORD)time.hour << 11) | ((DWORD)time.min << 5) | (time.sec >> 1));
}

#endif
