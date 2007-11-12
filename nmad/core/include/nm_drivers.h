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
#  define CONFIG_IBVERBS_ENABLED 1
#else
#  define CONFIG_IBVERBS_ENABLED 0
#endif

#if defined CONFIG_MX
#  include <nm_mx_public.h>
#  define CONFIG_MX_ENABLED 1
#else
#  define CONFIG_MX_ENABLED 0
#endif

#if defined CONFIG_GM
#  include <nm_gm_public.h>
#  define CONFIG_GM_ENABLED 1
#else
#  define CONFIG_GM_ENABLED 0
#endif

#if defined CONFIG_QSNET
#  include <nm_qsnet_public.h>
#  define CONFIG_QSNET_ENABLED 1
#else
#  define CONFIG_QSNET_ENABLED 0
#endif

#if defined CONFIG_SISCI
#  include <nm_sisci_public.h>
#  define CONFIG_SISCI_ENABLED 1
#else
#  define CONFIG_SISCI_ENABLED 0
#endif

#if defined CONFIG_TCP
#  include <nm_tcpdg_public.h>
#  define CONFIG_TCP_ENABLED 1
#else
#  define CONFIG_TCP_ENABLED 0
#endif

#define NR_ENABLED_DRIVERS (CONFIG_IBVERBS_ENABLED+CONFIG_MX_ENABLED+CONFIG_GM_ENABLED+CONFIG_QSNET_ENABLED+CONFIG_SISCI_ENABLED+CONFIG_TCP_ENABLED)

#include <nm_predictions.h>

#endif /* NM_DRIVERS_H */
