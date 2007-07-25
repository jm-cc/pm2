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
 * mpi_collective.h
 * ================
 */

#ifndef MPI_COMMUNICATOR_H
#define MPI_COMMUNICATOR_H

/**
 * Returns a handle to the group of the given communicator.
 */
int MPI_Comm_group(MPI_Comm comm,
		   MPI_Group *group);

/**
 * Partitions the group associated to the communicator into disjoint
 * subgroups, one for each value of color.
 */
int MPI_Comm_split(MPI_Comm comm,
		   int color,
		   int key,
		   MPI_Comm *newcomm);

/**
 * Creates a new intracommunicator with the same fixed attributes as
 * the input intracommunicator.
 */
int MPI_Comm_dup(MPI_Comm comm,
		 MPI_Comm *newcomm);

/**
 * Marks the communication object for deallocation.
 */
int MPI_Comm_free(MPI_Comm *comm);

/**
 * Maps the rank of a set of processes in group1 to their rank in
 * group2.
 */
int MPI_Group_translate_ranks(MPI_Group group1,
			      int n,
			      int *ranks1,
			      MPI_Group group2,
			      int *ranks2);

#endif /* MPI_COMMUNICATOR_H */
