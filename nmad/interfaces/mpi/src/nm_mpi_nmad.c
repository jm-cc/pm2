/*
 * NewMadeleine
 * Copyright (C) 2017 (see AUTHORS file)
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

#include "nm_mpi_private.h"
#include "nm_mpi_nmad.h"

#include <Padico/Module.h>
PADICO_MODULE_HOOK(MadMPI);

/* ********************************************************* */


void nm_mpi_nmad_dest(nm_session_t*p_session, nm_gate_t*p_gate, MPI_Comm comm, int rank)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  *p_session = nm_mpi_communicator_get_session(p_comm);
  *p_gate = nm_mpi_communicator_get_gate(p_comm, rank);
}


void nm_mpi_nmad_data(struct nm_data_s*p_data, void*ptr, MPI_Datatype datatype, int count)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  nm_mpi_data_build(p_data, ptr, p_datatype, count);
}

