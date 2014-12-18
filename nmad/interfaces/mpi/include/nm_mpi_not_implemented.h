/*
 * NewMadeleine
 * Copyright (C) 2012-2014 (see AUTHORS file)
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


#ifndef NM_MPI_NOT_IMPLEMENTED_H
#define NM_MPI_NOT_IMPLEMENTED_H


/** \addtogroup mpi_interface */
/* @{ */

/** @name Functions: the implementation of these functions is missing but the prototypes are required to ensure compatibility with other tools/applications */
/* @{ */


typedef int MPI_Win;

#define MPI_ARGV_NULL (NULL)

int MPI_Intercomm_create(MPI_Comm local_comm,
			 int local_leader,
			 MPI_Comm peer_comm,
			 int remote_leader,
			 int tag,
			 MPI_Comm *newintercomm);

int MPI_Comm_remote_size(MPI_Comm comm, int *size);

int MPI_Get(void *origin_addr,
	    int origin_count,
	    MPI_Datatype origin_datatype,
	    int target_rank,
	    MPI_Aint target_disp,
            int target_count,
	    MPI_Datatype target_datatype,
	    MPI_Win win);

int MPI_Put(void *origin_addr,
	    int origin_count,
	    MPI_Datatype origin_datatype,
	    int target_rank,
	    MPI_Aint target_disp,
            int target_count,
	    MPI_Datatype target_datatype,
	    MPI_Win win);

MPI_Win MPI_Win_f2c(MPI_Fint win);

MPI_Fint MPI_Win_c2f(MPI_Win win);

/* @}*/
/* @}*/

#endif /* NM_MPI_NOT_IMPLEMENTED_H */
