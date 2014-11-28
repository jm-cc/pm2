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


#ifndef NM_MPI_DATATYPE_H
#define NM_MPI_DATATYPE_H

/** \addtogroup mpi_interface */
/* @{ */

/** @name Functions: User-defined datatypes and packing */
/* @{ */


/**
 * Returns the total size, in bytes, of the entries in the type
 * signature associated with datatype.
 * @param datatype datatype
 * @param size datatype size
 * @return MPI status
 */
int MPI_Type_size(MPI_Datatype datatype,
		  int *size);

/**
 * Returns the lower bound and the extent of datatype
 * @param datatype datatype
 * @param lb lower bound of datatype
 * @param extent extent of datatype
 * @return MPI status
 */
int MPI_Type_get_extent(MPI_Datatype datatype,
			MPI_Aint *lb,
			MPI_Aint *extent);

/**
 * Returns the extent of the datatype
 * @param datatype datatype
 * @param extent extent of datatype
 * @return MPI status
 */
int MPI_Type_extent(MPI_Datatype datatype,
		    MPI_Aint *extent);

/**
 * Returns the lower bound of the datatype
 * @param datatype datatype
 * @param lb lower bound of datatype
 * @return MPI status
 */
int MPI_Type_lb(MPI_Datatype datatype,
		MPI_Aint *lb);

/**
 * Returns in newtype a new datatype that is identical to oldtype,
 * except that the lower bound of this new datatype is set to be lb,
 * and its upper bound is set to lb + extent.
 * @param oldtype input datatype
 * @param lb new lower bound of datatype
 * @param extent new extent of datatype
 * @param newtype output datatype
 * @return MPI status
 */
int MPI_Type_create_resized(MPI_Datatype oldtype,
			    MPI_Aint lb,
			    MPI_Aint extent,
			    MPI_Datatype *newtype);

/**
 * Commits the datatype.
 * @param datatype datatype that is committed
 * @return MPI status
 */
int MPI_Type_commit(MPI_Datatype *datatype);

/**
 * Marks the datatype object associated with datatype for
 * deallocation.
 * @param datatype datatype that is freed
 * @return MPI status
 */
int MPI_Type_free(MPI_Datatype *datatype);

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into contiguous locations.
 * @param count replication count
 * @param oldtype old datatype
 * @param newtype new datatype
 * @return MPI status
 */
int MPI_Type_contiguous(int count,
                        MPI_Datatype oldtype,
                        MPI_Datatype *newtype);

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into location that consist of equally spaced blocks.
 * @param count number of blocks
 * @param blocklength number of elements in each block
 * @param stride number of elements between start of each block
 * @param oldtype old datatype
 * @param newtype new datatype
 * @return MPI status
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
 * @param count number of blocks
 * @param blocklength number of elements in each block
 * @param stride number of bytes between start of each block
 * @param oldtype old datatype
 * @param newtype new datatype
 * @return MPI status
 */
int MPI_Type_hvector(int count,
                     int blocklength,
                     MPI_Aint stride,
                     MPI_Datatype oldtype,
                     MPI_Datatype*newtype);

/**
 * Constructs a typemap consisting of the replication of a datatype
 * into a sequence of blocks, each block is a concatenation of the old
 * datatype.
 * @param count number of blocks -- also number of entries in array_of_displacements and array_of_blocklengths
 * @param array_of_blocklengths number of elements per block
 * @param array_of_displacements displacement for each block, in multiples of oldtype extent
 * @param oldtype old datatype
 * @param newtype new datatype
 * @return MPI status
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
 * @param count number of blocks -- also number of entries in array_of_displacements and array_of_blocklengths
 * @param array_of_blocklengths number of elements per block
 * @param array_of_displacements byte displacement of each block
 * @param oldtype old datatype
 * @param newtype new datatype
 * @return MPI status
 */
int MPI_Type_hindexed(int count,
                      int *array_of_blocklengths,
                      MPI_Aint *array_of_displacements,
                      MPI_Datatype oldtype,
                      MPI_Datatype *newtype);

/**
 * Constructs a typemap consisting of the replication of different
 * datatypes, with different block sizes.
 * @param count number of blocks -- also number of entries in arrays array_of_types, array_of_displacements and array_of_blocklengths
 * @param array_of_blocklengths number of elements in each block
 * @param array_of_displacements byte displacement of each block
 * @param array_of_types type of elements in each block
 * @param newtype new datatype
 * @return MPI status
 */
int MPI_Type_struct(int count,
                    int *array_of_blocklengths,
                    MPI_Aint *array_of_displacements,
                    MPI_Datatype *array_of_types,
                    MPI_Datatype *newtype);


int MPI_Type_create_hindexed(int count,
			     const int array_of_blocklengths[],
			     const MPI_Aint array_of_displacements[],
			     MPI_Datatype oldtype,
			     MPI_Datatype*newtype);

int MPI_Type_create_hvector(int count,
			    int blocklength,
			    MPI_Aint hstride,
			    MPI_Datatype oldtype,
			    MPI_Datatype*newtype);


int MPI_Type_get_envelope(MPI_Datatype datatype,
			  int*num_integers,
			  int*num_addresses,
			  int*num_datatypes,
			  int*combiner);

int MPI_Type_get_contents(MPI_Datatype datatype,
			  int max_integers,
			  int max_addresses,
			  int max_datatypes,
			  int array_of_integers[],
			  MPI_Aint array_of_addresses[],
			  MPI_Datatype array_of_datatypes[]);


/**
 * Packs a message specified by inbuf, incount, datatype, comm into
 * the buffer space specified by outbuf and outsize. The input buffer
 * can be any communication buffer allowed in MPI_SEND. The output
 * buffer is a contiguous storage area containing outsize bytes,
 * starting at the address outbuf.
 * @param inbuf initial address of send buffer
 * @param incount number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param outbuf
 * @param outsize
 * @param position
 * @param comm communicator
 * @return MPI status
 */
int MPI_Pack(void* inbuf,
             int incount,
             MPI_Datatype datatype,
             void *outbuf,
             int outsize,
             int *position,
             MPI_Comm comm);

/**
 * Unpacks a message into the receive buffer specified by outbuf,
 * outcount, datatype from the buffer space specified by inbuf and
 * insize. The output buffer can be any communication buffer allowed
 * in MPI_RECV. The input buffer is a contiguous storage area
 * containing insize bytes, starting at address inbuf. The input value
 * of position is the position in the input buffer where one wishes
 * the unpacking to begin. The output value of position is incremented
 * by the size of the packed message, so that it can be used as input
 * to a subsequent call to MPI_UNPACK.
 * @param inbuf initial address of receive buffer
 * @param insize number of elements in receive buffer
 * @param position
 * @param outbuf
 * @param outcount
 * @param datatype datatype of each receive buffer element
 * @param comm communicator
 * @return MPI status
 */
int MPI_Unpack(void* inbuf,
               int insize,
               int *position,
               void *outbuf,
               int outcount,
               MPI_Datatype datatype,
               MPI_Comm comm);

/**
 * Returns the upper bound on the amount of space needed to pack a message
 * @param incount count argument to packing call
 * @param datatype datatype argument to packing call
 * @param comm communicator argument to packing call
 * @apram size upper bound on size of packed message, in bytes 
 * @return MPI status
 */
int MPI_Pack_size(int incount,
		  MPI_Datatype datatype,
		  MPI_Comm comm,
		  int*size);
/**
 * Returns the upper bound of a datatype
 */
int MPI_Type_ub(MPI_Datatype datatype, MPI_Aint*displacement);

/* @}*/
/* @}*/

#endif /* NM_MPI_DATATYPE_H */
