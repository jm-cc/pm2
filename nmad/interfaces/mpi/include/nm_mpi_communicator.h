/*
 * NewMadeleine
 * Copyright (C) 2006-2014 (see AUTHORS file)
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


#ifndef NM_MPI_COMMUNICATOR_H
#define NM_MPI_COMMUNICATOR_H

/** \addtogroup mpi_interface */
/* @{ */

/** @name Functions: Communicators */
/* @{ */

/**
 * Returns the size of a group
 * @param group group
 * @param size number of processes in the group
 * @return MPI status
 */
int MPI_Group_size(MPI_Group group,
		   int*size);
/**
 * Returns the rank of this process in the given group
 * @param group group
 * @param rank rank of the calling process in group, or MPI_UNDEFINED if the process is not a member
 * @return MPI status
 */
int MPI_Group_rank(MPI_Group group,
		   int *rank);

/**
 * Produces a group by combining two groups
 * @param group1 first group
 * @param group2 second group
 * @param newgroup union group
 * @return MPI status
 */
int MPI_Group_union(MPI_Group group1,
		    MPI_Group group2,
		    MPI_Group*newgroup);

/**
 * Produces a group as the intersection of two existing groups
 * @param group1 first group
 * @param group2 second group
 * @param newgroup intersection group
 * @return MPI status
 */
int MPI_Group_intersection(MPI_Group group1,
			   MPI_Group group2,
			   MPI_Group*newgroup);

/**
 * Produces a group from the difference of two groups
 * @param group1 first group
 * @param group2 second group
 * @param newgroup difference group
 * @return MPI status
 */
int MPI_Group_difference(MPI_Group group1,
			 MPI_Group group2,
			 MPI_Group*newgroup);

/**
 * Compares two groups
 * @param group1 first group
 * @param group2 second group
 * @param result nteger which is MPI_IDENT if the order and members of the two groups are the same, MPI_SIMILAR if only the members are the same, and MPI_UNEQUAL otherwise
 * @return MPI status
 */
int MPI_Group_compare(MPI_Group group1,
		      MPI_Group group2,
		      int*result);

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
 * Creates a new group from ranges of ranks in an existing group
 * @param group group
 * @param n number of triplets in array ranges
 * @param ranges a one-dimensional array of integer triplets, of the form (first rank, last rank, stride) indicating ranks in group or processes to be included in newgroup.
 * @param newgroup new group derived from above, in the order defined by ranges
 * @return MPI status
 */
int MPI_Group_range_incl(MPI_Group group,
			 int n,
			 int ranges[][3], 
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
 * Produces a group by excluding ranges of processes from an existing group
 * @param group group
 * @param n number of triplets in array ranges
 * @param ranges a one-dimensional array of integer triplets of the form (first rank, last rank, stride), indicating the ranks in group of processes to be excluded from the output group newgroup 
 * @param newgroup new group derived from above, preserving the order in group
 * @return MPI status
 */
int MPI_Group_range_excl(MPI_Group group,
			 int n,
			 int ranges[][3], 
			 MPI_Group*newgroup);

/**
 * Frees a group
 * @param group group to free
 * @return MPI status
 */
int MPI_Group_free(MPI_Group*group);

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

/**
 * This function indicates the number of processes involved in a an
 * intracommunicator.
 * @param comm communicator
 * @param size number of processes in the group of comm
 * @return MPI status
 */
int MPI_Comm_size(MPI_Comm comm,
                  int *size);

/**
 * This function gives the rank of the process in the particular
 * communicator's group.
 * @param comm communicator
 * @param rank rank of the calling process in group of comm
 * @return MPI status
 */
int MPI_Comm_rank(MPI_Comm comm,
                  int *rank);

int MPI_Keyval_create(MPI_Copy_function*copy_fn,
		      MPI_Delete_function*delete_fn,
		      int*keyval,
		      void*extra_state);

int MPI_Keyval_free(int *keyval);

/**
 * This function returns attributes values from communicators.
 */
int MPI_Attr_get(MPI_Comm comm,
		 int keyval,
		 void*attr_value,
		 int*flag);

/**
 * Stores attribute value associated with a key
 */
int MPI_Attr_put(MPI_Comm comm,
		 int keyval,
		 void*attr_value);

int MPI_Attr_delete(MPI_Comm comm,
		    int keyval);

/**
 * Creates a new communicator
 * @param comm communicator
 * @param group group, which is a subset of the group of comm
 * @param newcomm new communicator
 */
int MPI_Comm_create(MPI_Comm comm,
		    MPI_Group group,
		    MPI_Comm*newcomm);

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

/**
 * Tests to see if a comm is an inter-communicator
 * @param comm communicator to test
 * @param flag true if this is an inter-communicator
 */
int MPI_Comm_test_inter(MPI_Comm comm, int*flag);

/**
 * Compares two communicators
 */
int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int*result);

int MPI_Cart_create(MPI_Comm comm_old, int ndims, int*dims, int*periods, int reorder, MPI_Comm*_comm_cart);

int MPI_Cart_coords(MPI_Comm comm, int rank, int ndims, int*coords);

int MPI_Cart_rank(MPI_Comm comm, int*coords, int*rank);

int MPI_Cart_shift(MPI_Comm comm, int direction, int displ, int*source, int*dest);

int MPI_Cart_get(MPI_Comm comm, int maxdims, int*dims, int*periods, int*coords);

/* @}*/
/* @}*/

#endif /* NM_MPI_COMMUNICATOR_H */
