#ifndef _MC_H_
#define _MC_H_

#include "../utils/types.h"
#include "../mem/mc_t210.h"

void mc_config_tsec_carveout(u32 bom, u32 size1mb, bool lock);
void mc_config_carveout();
void mc_config_carveout_finalize();
void mc_enable_ahb_redirect();
void mc_disable_ahb_redirect();
void mc_enable();

#endif
