/*
 * NewMadeleine
 * Copyright (C) 2014 (see AUTHORS file)
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

#include <nm_public.h>
#include <nm_sendrecv_interface.h>
#include <Padico/Puk.h>

/* ********************************************************* */

PUK_VECT_TYPE(nm_gate, nm_gate_t)

typedef nm_gate_vect_t nm_group_t;

#define NM_GROUP_NULL ((nm_group_t)NULL)

#define NM_GROUP_IDENT   0x01
#define NM_GROUP_SIMILAR 0x02
#define NM_GROUP_UNEQUAL 0x04

/*
 * MPI_Group_size	returns number of processes in group
 * MPI_Group_rank	returns rank of calling process in group
 * MPI_Group_compare	compares group members and group order
 * MPI_Group_union	creates a group by combining two groups
 * MPI_Group_intersection	creates a group from the intersection of two groups
 * MPI_Group_difference	creates a group from the difference between two groups
 * MPI_Group_incl	creates a group from listed members of an existing group
 * MPI_Group_excl	creates a group excluding listed members of an existing group
 * MPI_Group_free	marks a group for deallocation

 TODO-
 MPI_Group_translate_ranks	translates ranks of processes in one group to those in another group
 MPI_Group_range_incl	creates a group according to first rank, stride, last rank
 MPI_Group_range_excl	creates a group by deleting according to first rank, stride, last rank

 MPI_GROUP_NULL
 MPI_GROUP_EMPTY
*/

extern int nm_group_size(nm_group_t group);

extern int nm_group_rank(nm_group_t group);

extern int nm_group_compare(nm_group_t group1, nm_group_t group2);

extern void nm_group_free(nm_group_t group);

extern nm_group_t nm_group_dup(nm_group_t group);

extern nm_group_t nm_group_incl(nm_group_t group, int n, const int ranks[]);

extern nm_group_t nm_group_excl(nm_group_t group, int n, const int ranks[]);

extern nm_group_t nm_group_union(nm_group_t group1, nm_group_t group2);

extern nm_group_t nm_group_intersection(nm_group_t group1, nm_group_t group2);

extern nm_group_t nm_group_difference(nm_group_t group1, nm_group_t group2);

