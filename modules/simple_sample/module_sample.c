/*  Sample Hekate Module
	2018 - M4xw
*/

#include <string.h>
#include <module.h>
#include <gfx_utils.h>

void _modInit(void *moduleConfig, bdkParams_t bp)
{
	memcpy(&gfx_con, bp->gfxCon, sizeof(gfx_con_t));
	memcpy(&gfx_ctxt, bp->gfxCtx, sizeof(gfx_ctxt_t));

	gfx_puts("Hello World!");

	memcpy(bp->gfxCon, &gfx_con, sizeof(gfx_con_t));
	memcpy(bp->gfxCtx, &gfx_ctxt, sizeof(gfx_ctxt_t));
}
