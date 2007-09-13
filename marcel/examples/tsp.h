
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

#ifdef MARCEL
#  ifdef MT
#    undef MT
#  endif /* MT */
#  define MT
#endif /* MARCEL */

#ifdef MT
#  ifdef MARCEL
#    include "marcel.h"
#  else
#    include <pthread.h>
#  endif
#endif

#include "tsp-types.h"
#include "tsp-job.h"

extern void tsp(int hops, int len, Path_t path, int *cuts, int num_worker);

extern void distributor(int hops, int len, Path_t path, struct s_tsp_queue *q);

extern void generate_jobs(void);

extern void *worker(void *);
