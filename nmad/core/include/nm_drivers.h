/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#ifndef NM_DRIVERS_H
#define NM_DRIVERS_H

#if defined CONFIG_IBVERBS
#  include <nm_ibverbs_public.h>
#endif

#if defined CONFIG_MX
#  include <nm_mx_public.h>
#endif

#if defined CONFIG_GM
#  include <nm_gm_public.h>
#endif

#if defined CONFIG_QSNET
#  include <nm_qsnet_public.h>
#endif

#if defined CONFIG_SISCI
#  include <nm_sisci_public.h>
#endif

#include <nm_tcpdg_public.h>

#include <nm_predictions.h>

#endif /* NM_DRIVERS_H */
