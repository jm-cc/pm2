/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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


#ifndef NM_MPI_NMAD_H
#define NM_MPI_NMAD_H

#include <nm_public.h>
#include <nm_session_interface.h>
#include <mpi.h>

/** \defgroup mpi_nmad_interface MadMPI nmad-specific interface to use MPI alongside native nmad
 *
 * @{
 */

/** get session & gate from MPI communicator & rank */
void nm_mpi_nmad_dest(nm_session_t*p_session, nm_gate_t*p_gate, MPI_Comm comm, int rank);

/** build a nm_data from MPI data in ptr, datatype, count */
void nm_mpi_nmad_data(struct nm_data_s*p_data, void*ptr, MPI_Datatype datatype, int count);

/* @} */

#endif /* NM_MPI_NMAD_H */

