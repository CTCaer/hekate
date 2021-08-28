/*
 * Copyright (c) 2018 naehrwert
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

#ifndef _MC_H_
#define _MC_H_

#include <utils/types.h>
#include <mem/mc_t210.h>

void mc_config_tsec_carveout(u32 bom, u32 size1mb, bool lock);
void mc_config_carveout();
void mc_config_carveout_finalize();
void mc_enable_ahb_redirect(bool full_aperture);
void mc_disable_ahb_redirect();
void mc_enable();

#endif
