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


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <pm2_common.h>

#include "nm_private.h"
#include "nm_public.h"
#include "nm_core.h"
#include "nm_log.h"
#include "nm_errno.h"

/** Main function of the core scheduler loop.
 *
 * This is the heart of NewMadeleine...
 */
int
nm_schedule(struct nm_core *p_core) {
#ifdef PIOMAN
	return 0;
#else
        int err;
        int g;

        /* send
         */
        for (g = 0; g < p_core->nb_gates; g++) {
                err = nm_sched_out_gate(p_core->gate_array + g);

                if (err < 0) {
                        NM_DISPF("nm_sched_out_gate(%d) returned %d",
                             g, err);
                }
        }

        /* receive
         */
        err	= nm_sched_in(p_core);

        if (err < 0) {
                NM_DISPF("nm_sched_in_gate(%d) returned %d",
                     g, err);
        }


        NM_TRACEF("\n");
        err = NM_ESUCCESS;

        return err;
#endif
}
