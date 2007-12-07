/*
 *  (C) 2006 by Argonne National Laboratory.
 *   Contact:  Darius Buntinas
 */ 

#ifndef CCS_DATALOOP_H
#define CCS_DATALOOP_H

#include "ccs.h"

/* Note: this is where you define the prefix that will be prepended on
 * all externally visible generic dataloop and segment functions.
 */
#define PREPEND_PREFIX(fn) CCSI_ ## fn

/* These following dataloop-specific types will be used throughout the DLOOP
 * instance:
 */
#define DLOOP_Offset     CCS_aint_t
#define DLOOP_Count      int
#define DLOOP_Handle     CCS_datadesc_t
#define DLOOP_Type       CCS_datadesc_t
#define DLOOP_Buffer     void *
#define DLOOP_VECTOR     CCS_iovec_t
#define DLOOP_VECTOR_LEN len
#define DLOOP_VECTOR_BUF base

/* The following accessor functions must also be defined:
 *
 * DLOOP_Handle_extent()    *
 * DLOOP_Handle_size()      *
 * DLOOP_Handle_loopptr()
 * DLOOP_Handle_loopdepth() *
 * DLOOP_Handle_hasloop()
 * DLOOP_Handle_true_lb()
 * DLOOP_Handle_mpi1_lb()
 * DLOOP_Handle_mpi1_ub()
 *
 * Q: Do we really need ALL of these? *'d ones we need for sure.
 */

/* USE THE NOTATION THAT BILL USED IN MPIIMPL.H AND MAKE THESE MACROS */

/* NOTE: put get size into mpiimpl.h; the others go here until such time
 * as we see that we need them elsewhere.
 */
#define DLOOP_Handle_get_loopdepth_macro(handle_, depth_, hetero_) \
    CCSI_datadesc_get_loopdepth_macro(handle_, depth_)

#define DLOOP_Handle_get_loopsize_macro(handle_, size_, hetero_) \
    CCSI_datadesc_get_loopsize_macro(handle_, size_)

#define DLOOP_Handle_get_loopptr_macro(handle_, lptr_, hetero_) \
    CCSI_datadesc_get_loopptr_macro(handle_, lptr_)

#define DLOOP_Handle_get_size_macro(handle_, size_) do {	\
    if (CCS_datadesc_is_builtin (handle_))			\
	size_ = CCSI_datadesc_builtin_size_macro (handle_);	\
    else							\
	size_ = CCSI_datadesc_handle_ptr (handle_)->size;	\
} while (0)

#define DLOOP_Handle_get_basic_type_macro(handle_, eltype_) \
    CCSI_datadesc_get_basic_type(handle_, eltype_)

#define DLOOP_Handle_get_basic_size_macro(handle_, size_) do {	\
    size_ = CCSI_datadesc_builtin_size_macro (handle_);	\
} while (0)

#define DLOOP_Handle_get_extent_macro(handle_, extent_) do {	\
    if (CCS_datadesc_is_builtin (handle_))			\
	extent_ = CCSI_datadesc_builtin_size_macro (handle_);	\
    else							\
	extent_ = CCSI_datadesc_handle_ptr (handle_)->extent;	\
} while (0)

#define DLOOP_Handle_hasloop_macro(handle_)	\
    (CCS_datadesc_is_builtin(handle_)?0:1)

/* allocate and free functions must also be defined. */
#define DLOOP_Malloc CCS_malloc
#define DLOOP_Free   CCS_free

/* debugging output function */
#define DLOOP_dbg_printf CCSI_dbg_printf

/* assert function */
#define DLOOP_Assert CCSI_Assert

/* Include gen_dataloop.h at the end to get the rest of the prototypes
 * and defines, in terms of the prefixes and types above.
 */
#include "gen_dataloop.h"

/* NOTE: WE MAY WANT TO UNDEF EVERYTHING HERE FOR NON-INTERNAL COMPILATIONS */

/* dataloop construction functions */

int CCSI_Dataloop_create_contiguous(int count,
				     CCS_datadesc_t olddesc,
                                     CCSI_Dataloop **dlp_p,
				     int *dlsz_p,
				     int *dldepth_p,
				     int flags);
int CCSI_Dataloop_create_vector(int count,
				 int blocklength,
				 CCS_aint_t stride,
				 int strideinbytes,
				 CCS_datadesc_t olddesc,
				 CCSI_Dataloop **dlp_p,
				 int *dlsz_p,
				 int *dldepth_p,
				 int flags);
int CCSI_Dataloop_create_blockindexed(int count,
				       int blklen,
				       void *disp_array,
				       int dispinbytes,
				       CCS_datadesc_t olddesc,
				       CCSI_Dataloop **dlp_p,
				       int *dlsz_p,
				       int *dldepth_p,
				       int flags);
int CCSI_Dataloop_create_indexed(int count,
				  int *blocklength_array,
				  void *displacement_array,
				  int dispinbytes,
				  CCS_datadesc_t olddesc,
				  CCSI_Dataloop **dlp_p,
				  int *dlsz_p,
				  int *dldepth_p,
				  int flags);
int CCSI_Dataloop_create_struct(int count,
				 int *blklen_array,
				 CCS_aint_t *disp_array,
				 CCS_datadesc_t *olddesc_array,
				 CCSI_Dataloop **dlp_p,
				 int *dlsz_p,
				 int *dldepth_p,
				 int flags);

#define CCSI_DATALOOP_HOMOGENEOUS 1
#define CCSI_DATALOOP_ALL_BYTES   2

#endif
