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
#include "pioman.h"

#ifdef MARCEL
#include "marcel.h"
#endif	/* MARCEL */

#include <errno.h>


void pioman_init(int*argc, char**argv)
{
#ifdef MARCEL
    marcel_init(argc, argv);
#endif /* MARCEL */
    piom_init_ltasks();
    /*    piom_io_task_init(); */
}

void pioman_exit(void)
{
    /*
      piom_io_task_stop();
    */
    piom_exit_ltasks();
}

/** Polling point. May be called from the application to force polling,
 * from marcel hooks, from timer handler.
 * @return 0 if we didn't need to poll and 1 otherwise
 */
int piom_check_polling(unsigned polling_point)
{
    int ret = 0;
#ifdef PIOM_ENABLE_SHM
    if(piom_shs_polling_is_required())
	{
	    piom_shs_poll(); 
	    ret = 1;
	}
#endif	/* PIOM_ENABLE_SHM */

    void*task = piom_ltask_schedule();
    ret = (task != NULL);
    return ret;
}
