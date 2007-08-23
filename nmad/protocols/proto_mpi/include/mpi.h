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

/*
 * mpi.h
 * =====
 */

#ifndef MPI_H
#define MPI_H

/** \defgroup mpi_interface MPI Interface
 *
 * This is the MPI interface. The implementation called Mad-MPI is a
 * new light implementation of the MPI standard. This simple,
 * straightforward proof-of-concept implementation is a subset of the
 * MPI API, that allows MPI applications to benefit from the
 * NewMadeleine communication engine. Mad-MPI is based on the
 * point-to-point nonblocking posting (isend, irecv) and completion
 * (wait, test) operations of MPI, these four operations being
 * directly mapped to the equivalent operations of NewMadeleine.
 *
 * Mad-MPI also implements some optimizations mechanisms for derived
 * datatypes. MPI derived datatypes deal with noncontiguous
 * memory locations. The advanced optimizations of NewMadeleine
 * allowing to reorder packets lead to a significant gain when sending
 * and receiving data based on derived datatypes.
 *
 * @{
 */

#include "mpi_types.h"

#include "mpi_config.h"

#include "mpi_pt2pt.h"

#include "mpi_persistent.h"

#include "mpi_collective.h"

#include "mpi_datatype.h"

#include "mpi_communicator.h"

/* @} */

#endif /* MPI_H */

