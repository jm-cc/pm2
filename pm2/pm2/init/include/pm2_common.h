
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

#ifndef PM2_COMMON_IS_DEF
#define PM2_COMMON_IS_DEF

// Beurk!!!
struct _struct_common_attr_t;
typedef struct _struct_common_attr_t common_attr_t;

#if !defined(MARCEL_KERNEL) && \
    !defined(MAD2_KERNEL) && \
    !defined(PM2_KERNEL) && \
    !defined(TBX_KERNEL) && \
    !defined(NTBX_KERNEL) && \
    !defined(DSM_KERNEL) && \
    !defined(MAD1_KERNEL) && \
    !defined(PROFILE_KERNEL)

#ifdef TBX
#include "tbx.h"
#endif /* TBX */

#ifdef NTBX
#include "ntbx.h"
#endif /* NTBX */

#ifdef MARCEL
#include "marcel.h"
#endif /* MARCEL */

#ifdef MAD1
#include "madeleine.h"
#endif /* MAD1 */

#ifdef MAD2
#include "madeleine.h"
#endif /* MAD2 */

#ifdef PM2
#include "pm2.h"
#endif /* PM2 */

struct _struct_common_attr_t {
#ifdef MAD2
  p_mad_madeleine_t madeleine; // OUT
  int rank; // OUT
  p_mad_adapter_set_t adapter_set; // IN
#ifdef APPLICATION_SPAWN
  char *url; // IN/OUT
#endif
#endif
};

#endif // *_KERNEL

void common_attr_init(common_attr_t *attr);

void common_pre_init(int *argc, char *argv[],
		     common_attr_t *attr);

void common_post_init(int *argc, char *argv[],
		      common_attr_t *attr);

static __inline__ void common_init(int *argc, char *argv[],
				   common_attr_t *attr)
{
  common_pre_init(argc, argv, attr);
  common_post_init(argc, argv, attr);
}

#endif
