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

/** \addtogroup mpi_interface */
/* @{ */

/** @name Functions: Communicators */
/* @{ */

/**
 * Produces a group by reordering an existing group and taking only listed members
 * @param group group
 * @param n number of elements in array ranks
 * @param ranks ranks of processes in group to appear in newgroup
 * @param newgroup new group derived from above, in the order defined by ranks
 * @return MPI status
 */
int MPI_Group_incl(MPI_Group group,
		   int n,
		   int*ranks,
		   MPI_Group*newgroup);
/**
 * Produces a group by reordering an existing group and taking only unlisted members
 * @param group group
 * @param n number of elements in array ranks
 * @param ranks ranks of processes in group to appear in newgroup
 * @param newgroup new group derived from above, in the order defined by ranks
 * @return MPI status
 */
int MPI_Group_excl(MPI_Group group,
		   int n,
		   int*ranks,
		   MPI_Group*newgroup);

/**
 * Frees a group
 * @param group group to free
 * @return MPI status
 */
int MPI_Group_free(MPI_Group*group);

/**
 * Returns a handle to the group of the given communicator.
 * @param comm communicator
 * @param group group corresponding to comm
 * @return MPI status
 */
int MPI_Comm_group(MPI_Comm comm,
		   MPI_Group *group);

/**
 * Partitions the group associated to the communicator into disjoint
 * subgroups, one for each value of color.
 * @param comm communicator
 * @param color control of subset assignment
 * @param key control of rank assigment
 * @param newcomm new communicator
 * @return MPI status
 */
int MPI_Comm_split(MPI_Comm comm,
		   int color,
		   int key,
		   MPI_Comm *newcomm);

/**
 * Creates a new intracommunicator with the same fixed attributes as
 * the input intracommunicator.
 * @param comm communicator
 * @param newcomm copy of comm
 * @return MPI status
 */
int MPI_Comm_dup(MPI_Comm comm,
		 MPI_Comm *newcomm);

/**
 * Marks the communication object for deallocation.
 * @param comm communicator to be destroyed
 * @return MPI status
 */
int MPI_Comm_free(MPI_Comm *comm);

int MPI_Cart_create(MPI_Comm comm_old, int ndims, int*dims, int*periods, int reorder, MPI_Comm*_comm_cart);

int MPI_Cart_coords(MPI_Comm comm, int rank, int ndims, int*coords);

int MPI_Cart_rank(MPI_Comm comm, int*coords, int*rank);

int MPI_Cart_shift(MPI_Comm comm, int direction, int displ, int*source, int*dest);

/**
 * Maps the rank of a set of processes in group1 to their rank in
 * group2.
 * @param group1 group
 * @param n number of ranks in ranks1 and ranks2 arrays
 * @param ranks1 array of zero or more valid ranks in group1
 * @param group2 group
 * @param ranks2 array of corresponding ranks in group2
 * @return MPI status
 */
int MPI_Group_translate_ranks(MPI_Group group1,
			      int n,
			      int *ranks1,
			      MPI_Group group2,
			      int *ranks2);

/* @}*/
/* @}*/

#endif /* MPI_COMMUNICATOR_H */
