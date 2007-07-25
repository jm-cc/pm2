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
 * mpi_datatype.h
 * ==============
 */

#ifndef MPI_DATATYPE_H
#define MPI_DATATYPE_H

/** \addtogroup mpi_interface */
/* @{ */

/**
 * Returns the byte address of location.
 */
int MPI_Get_address(void *location,
		    MPI_Aint *address);

/**
 * Returns the byte address of location.
 */
int MPI_Address(void *location,
		MPI_Aint *address);

/**
 * Returns the total size, in bytes, of the entries in the type
 * signature associated with datatype.
 */
int MPI_Type_size(MPI_Datatype datatype,
		  int *size);

/**
 * Returns the lower bound and the extent of datatype
 */
int MPI_Type_get_extent(MPI_Datatype datatype,
			MPI_Aint *lb,
			MPI_Aint *extent);

/**
 * Returns the extent of the datatype
 */
int MPI_Type_extent(MPI_Datatype datatype,
		    MPI_Aint *extent);

/**
 * Returns the lower bound of the datatype
 */
int MPI_Type_lb(MPI_Datatype datatype,
		MPI_Aint *lb);

/**
 * Returns in newtype a new datatype that is identical to oldtype,
 * except that the lower bound of this new datatype is set to be lb,
 * and its upper bound is set to lb + extent.
 */
int MPI_Type_create_resized(MPI_Datatype oldtype,
			    MPI_Aint lb,
			    MPI_Aint extent,
			    MPI_Datatype *newtype);

/**
 * Commits the datatype.
 */
int MPI_Type_commit(MPI_Datatype *datatype);

/**
 * Marks the datatype object associated with datatype for
 * deallocation.
 */
int MPI_Type_free(MPI_Datatype *datatype);

/**
 * This function does not belong to the MPI standard:
 * Marks the datatype object associated with datatype as being
 * optimized, i.e the pack interface can be used for communications
 * requests using that type, instead of copying the data into a
 * contiguous buffer.
 */
int MPI_Type_optimized(MPI_Datatype *datatype,
                       int optimized);

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into contiguous locations.
 */
int MPI_Type_contiguous(int count,
                        MPI_Datatype oldtype,
                        MPI_Datatype *newtype);

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into location that consist of equally spaced blocks.
 */
int MPI_Type_vector(int count,
                    int blocklength,
                    int stride,
                    MPI_Datatype oldtype,
                    MPI_Datatype *newtype);

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into location that consist of equally spaced blocks, assumes that
 * the stride between successive blocks is a multiple of the oldtype
 * extent.
 */
int MPI_Type_hvector(int count,
                     int blocklength,
                     int stride,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype);

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into a sequence of blocks, each block is a concatenation of the old
 * datatype.
 */
int MPI_Type_indexed(int count,
                     int *array_of_blocklengths,
                     int *array_of_displacements,
                     MPI_Datatype oldtype,
                     MPI_Datatype *newtype);

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into a sequence of blocks, each block is a concatenation of the old
 * datatype; block displacements are specified in bytes, rather than
 * in multiples of the old datatype extent.
 */
int MPI_Type_hindexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype);

/**
 * Constructs a typemap consisting of the replication of different
 * datatypes, with different block sizes.
 */
int MPI_Type_struct(int count,
                    int *array_of_blocklengths,
                    MPI_Aint *array_of_displacements,
                    MPI_Datatype *array_of_types,
                    MPI_Datatype *newtype);

/* @}*/

#endif /* MPI_DATATYPE_H */
