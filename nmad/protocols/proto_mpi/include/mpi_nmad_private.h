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

/*
 * mpi_nmad_private.h
 * ==================
 */

#ifndef MPI_NMAD_PRIVATE_H
#define MPI_NMAD_PRIVATE_H

#include <tbx.h>

#undef MPI_NMAD_SO_DEBUG

int not_implemented(char *s);

tbx_bool_t test_termination(MPI_Comm comm);

void inc_nb_incoming_msg(void);

void inc_nb_outgoing_msg(void);

#define CHECK_RETURN_CODE(err, message) { if (err != NM_ESUCCESS) { printf("%s return err = %d\n", message, err); return 1; }}

#if defined(MPI_NMAD_SO_DEBUG)
#  define MPI_NMAD_TRACE(...) { fprintf(stderr, __VA_ARGS__) ; }
#else
#  define MPI_NMAD_TRACE(...) { }
#endif /* MPI_NMAD_SO_DEBUG */

#endif /* MPI_NMAD_PRIVATE_H */

