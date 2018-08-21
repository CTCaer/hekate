/*  Sample Hekate Module
	2018 - M4xw
*/

#include "../../common/common_module.h"
#include "../../common/common_gfx.h"
#include "gfx/gfx.h"

void _modInit(void *moduleConfig, bdkParams_t bp)
{
	gfx_puts(bp->gfxCon, "Hello World!");
}
