/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#ifndef PIOM_H
#define PIOM_H

#include "tbx_fast_list.h"
#include "piom_sh_sem.h"
#include "tbx_fast_list.h"
#include "piom_sh_sem.h"
#include "piom_ltask.h"

#include "piom_sem.h"


#ifndef MARCEL
/* Without Marcel compliance */
#define piom_time_t int*
#else
#define piom_time_t marcel_time_t
#endif	/* MARCEL */

/* Polling constants. Defines the polling points */
#define PIOM_POLL_AT_TIMER_SIG  1
#define PIOM_POLL_AT_YIELD      2
#define PIOM_POLL_AT_LIB_ENTRY  4
#define PIOM_POLL_AT_IDLE       8
#define PIOM_POLL_AT_CTX_SWITCH 16
#define PIOM_POLL_WHEN_FORCED   32


#define PIOM_EXCEPTION_RAISE(e) abort();

/********************************************************************
 *  INLINE FUNCTIONS
 */


/* Try to poll
 * Called from Marcel
 * Return 0 if we didn't need to poll and 1 otherwise
 */
static inline int piom_check_polling(unsigned polling_point)
{
    int ret = 0;
#ifdef PIOM_ENABLE_SHM
    if(piom_shs_polling_is_required())
	{
	    piom_shs_poll(); 
	    ret = 1;
	}
#endif	/* PIOM_ENABLE_SHM */

    if(piom_ltask_polling_is_required())
	{
	    piom_ltask_schedule();
	    ret = 1;
	}
    return ret;
}


#endif /* PIOM_H */


