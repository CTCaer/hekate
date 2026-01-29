/*  Sample Hekate Module
	2018 - M4xw
*/

#include <string.h>
#include <module.h>
#include <gfx_utils.h>

void mod_entry(void *moduleConfig, bdk_params_t *bp)
{
	memcpy(&gfx_con, bp->gfx_con, sizeof(gfx_con_t));
	memcpy(&gfx_ctxt, bp->gfx_ctx, sizeof(gfx_ctxt_t));

	gfx_puts("Hello World!");

	memcpy(bp->gfx_con, &gfx_con, sizeof(gfx_con_t));
	memcpy(bp->gfx_ctx, &gfx_ctxt, sizeof(gfx_ctxt_t));
}
