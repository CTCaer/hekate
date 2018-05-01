#ifndef _MC_H_
#define _MC_H_

#include "types.h"
#include "mc_t210.h"

void mc_config_tsec_carveout(u32 bom, u32 size1mb, int lock);
void mc_config_carveout();
void mc_enable_ahb_redirect();
void mc_disable_ahb_redirect();
void mc_enable();

#endif
