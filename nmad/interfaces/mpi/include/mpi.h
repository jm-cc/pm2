/*
 * NewMadeleine
 * Copyright (C) 2006-2015 (see AUTHORS file)
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


#ifndef MPI_H
#define MPI_H

/** @defgroup mpi_interface MadMPI: MPI Interface
 *
 * MadMPI is the MPI interface for NewMadeleine. It is a lightweight
 * implementation of the MPI standard. It allows MPI applications to
 * benefit from the NewMadeleine communication engine. MadMPI
 * operations are directly mapped to native nmad operations.
 *
 * MadMPI also implements some optimizations mechanisms for derived
 * datatypes. MPI derived datatypes deal with noncontiguous
 * memory locations. The advanced optimizations of NewMadeleine
 * allowing to reorder packets lead to a significant gain when sending
 * and receiving data based on derived datatypes.
 *
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <sys/types.h>
#include "nm_config.h"
#include "nm_mpi_types.h"
#include "nm_mpi_config.h"
#include "nm_mpi_p2p.h"
#include "nm_mpi_persistent.h"
#include "nm_mpi_collective.h"
#include "nm_mpi_datatype.h"
#include "nm_mpi_communicator.h"
#include "nm_mpi_onesided.h"
#include "nm_mpi_not_implemented.h"

#ifndef NMAD_MPI_ENABLE_ROMIO
#include "nm_mpi_io.h"
#endif

#if defined(NMAD_MPI_ENABLE_ROMIO) && !defined(NMAD_BUILD)
#warning "include ROMIO"
#include "mpio.h"
#endif /* NMAD_MPI_ENABLE_ROMIO && !NMAD_BUILD */

#ifdef NMAD_ABT
#define main __abt_app_main
#endif /* PIOMAN_ABT */

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* MPI_H */

