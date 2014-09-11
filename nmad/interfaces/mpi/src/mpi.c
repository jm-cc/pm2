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
 * mpi.c
 * =====
 */

#include "mpi.h"
#include "mpi_nmad_private.h"
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include <Padico/Puk.h>
#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);

static puk_instance_t launcher_instance = NULL;

/** internal MadMPI state, pointer shared accross files */
__PUK_SYM_INTERNAL mpir_internal_data_t mpir_internal_data;

/* Custom requests allocator. Inspired by Puk lock-free allocator,
   except that we use int indexes for compatibility with Fortran.
*/
PUK_LFSTACK_TYPE(mpir_request_block, mpir_request_t req __attribute__((aligned(PUK_ALLOCATOR_ALIGN))););
PUK_LFSTACK_TYPE(mpir_request_slice, struct mpir_request_block_lfstack_cell_s block; );
static struct
{
  mpir_request_block_lfstack_t cache;          /**< cache for mpir_request_t blocks */
  mpir_request_slice_lfstack_t slices;         /**< slices of memory containing blocks */
  int slice_size;                              /**< number of blocks per slice */
  MPI_Fint next_id;                            /**< next req id to allocate */
  mpir_request_vect_t request_array;           /**< maps req_id -> mpir_request_t* */
} mpir_requests = { .cache = NULL, .slices = NULL, .slice_size = 256, .next_id = 1, .request_array = NULL };

static void mpir_request_init(void)
{
  mpir_requests.request_array = mpir_request_vect_new();
  /* placeholder so as to never allocate request #0 (easier to debug) */
  mpir_request_vect_push_back(mpir_requests.request_array, (void*)0xDEADBEEF);
}

mpir_request_t*mpir_request_alloc(void)
{
  struct mpir_request_block_lfstack_cell_s*cell = NULL;
 retry:
  cell = mpir_request_block_lfstack_pop(&mpir_requests.cache);
  if(cell == NULL)
    {
      /* cache miss; allocate a new slice */
      int slice_size = mpir_requests.slice_size;
      mpir_requests.slice_size *= 2;
      struct mpir_request_slice_lfstack_cell_s*slice =
	padico_malloc(sizeof(struct mpir_request_slice_lfstack_cell_s) + sizeof(struct mpir_request_block_lfstack_cell_s) * slice_size);
      mpir_request_slice_lfstack_push(&mpir_requests.slices, slice);
      struct mpir_request_block_lfstack_cell_s*blocks = &slice->block;
      int i;
      for(i = 0; i < slice_size; i++)
	{
	  blocks[i].req.request_id = mpir_requests.next_id++;
	  mpir_request_block_lfstack_push(&mpir_requests.cache, &blocks[i]);
	}
      goto retry;
    }
  mpir_request_t*req = &cell->req;
  mpir_request_vect_put(mpir_requests.request_array, req, req->request_id);
  return req;
}

void mpir_request_free(mpir_request_t*req)
{
  struct mpir_request_block_lfstack_cell_s*cell = container_of(req, struct mpir_request_block_lfstack_cell_s, req);
  const int id = req->request_id;
  req->request_type = MPI_REQUEST_ZERO;
  mpir_request_vect_put(mpir_requests.request_array, NULL, id);
  mpir_request_block_lfstack_push(&mpir_requests.cache, cell);
}

mpir_request_t*mpir_request_find(MPI_Fint req_id)
{
  const int id = (int)req_id;
  if(id == MPI_REQUEST_NULL)
    return NULL;
  assert(id >= 0);
  mpir_request_t*mpir_request = mpir_request_vect_at(mpir_requests.request_array, id);
  assert(mpir_request != NULL);
  assert(mpir_request->request_id == id);
  return mpir_request;
}

/* Aliases */

int MPI_Init(int *argc,
             char ***argv) __attribute__ ((alias ("mpi_init")));

int MPI_Init_thread(int *argc,
                    char ***argv,
                    int required TBX_UNUSED,
                    int *provided) __attribute__ ((alias ("mpi_init_thread")));

int MPI_Initialized(int *flag) __attribute__ ((alias ("mpi_initialized")));

int MPI_Finalize(void) __attribute__ ((alias ("mpi_finalize")));

int MPI_Abort(MPI_Comm comm TBX_UNUSED,
	      int errorcode) __attribute__ ((alias ("mpi_abort")));

int MPI_Get_processor_name(char *name,
			   int *resultlen) __attribute__ ((alias ("mpi_get_processor_name")));

double MPI_Wtime(void) __attribute__ ((alias ("mpi_wtime")));

double MPI_Wtick(void) __attribute__ ((alias ("mpi_wtick")));

int MPI_Error_string(int errorcode,
		     char *string,
		     int *resultlen) __attribute__ ((alias ("mpi_error_string")));

int MPI_Errhandler_set(MPI_Comm comm,
		       MPI_Errhandler errhandler) __attribute__ ((alias ("mpi_errhandler_set")));

int MPI_Get_version(int *version,
		    int *subversion) __attribute__ ((alias ("mpi_get_version")));

int MPI_Request_free(MPI_Request *request) __attribute__ ((alias ("mpi_request_free")));

int MPI_Get_count(MPI_Status *status,
                  MPI_Datatype datatype TBX_UNUSED,
                  int *count) __attribute__ ((alias ("mpi_get_count")));

int MPI_Request_is_equal(MPI_Request request1,
			 MPI_Request request2) __attribute__ ((alias ("mpi_request_is_equal")));



int MPI_Get_address(void *location,
		    MPI_Aint *address) __attribute__ ((alias ("mpi_get_address")));

int MPI_Address(void *location,
		MPI_Aint *address) __attribute__ ((alias ("mpi_address")));

int MPI_Status_c2f(MPI_Status *c_status,
		   MPI_Fint *f_status) __attribute__ ((alias ("mpi_status_c2f")));

int MPI_Status_f2c(MPI_Fint *f_status,
		   MPI_Status *c_status) __attribute__ ((alias ("mpi_status_f2c")));


/* ********************************************************* */


int mpi_init(int *argc,
             char ***argv) {
  int ret;
  MPI_NMAD_LOG_IN();

  nm_launcher_init(argc, *argv);

  /*
   * Internal initialisation
   */
  ret = mpir_internal_init(&mpir_internal_data);

  mpir_request_init();
  MPI_NMAD_LOG_OUT();
  return ret;
}

int mpi_init_thread(int *argc,
                    char ***argv,
                    int required TBX_UNUSED,
                    int *provided) {
  int err;

  MPI_NMAD_LOG_IN();

  err = mpi_init(argc, argv);
#ifndef PIOMAN
  *provided = MPI_THREAD_SINGLE;
#else
  *provided = MPI_THREAD_MULTIPLE;
#endif

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_initialized(int *flag) {
  *flag = (launcher_instance != NULL);
  return MPI_SUCCESS;
}

int mpi_finalize(void) {
  int err;

  MPI_NMAD_LOG_IN();

  mpi_barrier(MPI_COMM_WORLD);
  err = mpir_internal_exit(&mpir_internal_data);

  puk_instance_destroy(launcher_instance);
  launcher_instance = NULL;

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_abort(MPI_Comm comm TBX_UNUSED,
              int errorcode) {
  int err;
  MPI_NMAD_LOG_IN();

  err = mpir_internal_exit(&mpir_internal_data);

  MPI_NMAD_LOG_OUT();
  exit(errorcode);
  return errorcode;
}


int mpi_get_processor_name(char *name,
			   int *resultlen) {
  int err;

  MPI_NMAD_LOG_IN();

  err = gethostname(name, MPI_MAX_PROCESSOR_NAME);
  if (!err) {
    *resultlen = strlen(name);
    MPI_NMAD_LOG_OUT();
    return MPI_SUCCESS;
  }
  else {
    MPI_NMAD_LOG_OUT();
    return errno;
  }
}

double mpi_wtime(void)
{
  static tbx_tick_t orig_time;
  static int orig_done = 0;

  MPI_NMAD_LOG_IN();
  if(!orig_done)
    {
      TBX_GET_TICK(orig_time);
      orig_done = 1;
    }
  tbx_tick_t time;
  TBX_GET_TICK(time);
  const double usec = TBX_TIMING_DELAY(orig_time, time);

  MPI_NMAD_LOG_OUT();
  return usec / 1000000.0;
}

double mpi_wtick(void) {
  return 1e-7;
}

int mpi_error_string(int errorcode,
		     char *string,
		     int *resultlen) {
  *resultlen = 100;
  string = malloc(*resultlen * sizeof(char));
  switch (errorcode) {
    case MPI_SUCCESS : {
      snprintf(string, *resultlen, "Error MPI_SUCCESS\n");
      break;
    }
    case MPI_ERR_BUFFER : {
      snprintf(string, *resultlen, "Error MPI_ERR_BUFFER\n");
      break;
    }
    case MPI_ERR_COUNT : {
      snprintf(string, *resultlen, "Error MPI_ERR_COUNT\n");
      break;
    }
    case MPI_ERR_TYPE : {
      snprintf(string, *resultlen, "Error MPI_ERR_TYPE\n");
      break;
    }
    case MPI_ERR_TAG : {
      snprintf(string, *resultlen, "Error MPI_ERR_TAG\n");
      break;
    }
    case MPI_ERR_COMM : {
      snprintf(string, *resultlen, "Error MPI_ERR_COMM\n");
      break;
    }
    case MPI_ERR_RANK : {
      snprintf(string, *resultlen, "Error MPI_ERR_RANK\n");
      break;
    }
    case MPI_ERR_ROOT : {
      snprintf(string, *resultlen, "Error MPI_ERR_ROOT\n");
      break;
    }
    case MPI_ERR_GROUP : {
      snprintf(string, *resultlen, "Error MPI_ERR_GROUP\n");
      break;
    }
    case MPI_ERR_OP : {
      snprintf(string, *resultlen, "Error MPI_ERR_OP\n");
      break;
    }
    case MPI_ERR_TOPOLOGY : {
      snprintf(string, *resultlen, "Error MPI_ERR_TOPOLOGY\n");
      break;
    }
    case MPI_ERR_DIMS : {
      snprintf(string, *resultlen, "Error MPI_ERR_DIMS\n");
      break;
    }
    case MPI_ERR_ARG : {
      snprintf(string, *resultlen, "Error MPI_ERR_ARG\n");
      break;
    }
    case MPI_ERR_UNKNOWN : {
      snprintf(string, *resultlen, "Error MPI_ERR_UNKNOWN\n");
      break;
    }
    case MPI_ERR_TRUNCATE : {
      snprintf(string, *resultlen, "Error MPI_ERR_TRUNCATE\n");
      break;
    }
    case MPI_ERR_OTHER : {
      snprintf(string, *resultlen, "Error MPI_ERR_OTHER\n");
      break;
    }
    case MPI_ERR_INTERN : {
      snprintf(string, *resultlen, "Error MPI_ERR_INTERN\n");
      break;
    }
    case MPI_ERR_IN_STATUS : {
      snprintf(string, *resultlen, "Error MPI_ERR_IN_STATUS\n");
      break;
    }
    case MPI_ERR_PENDING : {
      snprintf(string, *resultlen, "Error MPI_ERR_PENDING\n");
      break;
    }
    case MPI_ERR_REQUEST : {
      snprintf(string, *resultlen, "Error MPI_ERR_REQUEST\n");
      break;
    }
    case MPI_ERR_LASTCODE : {
      snprintf(string, *resultlen, "Error MPI_ERR_LASTCODE\n");
      break;
    }
    default : {
      snprintf(string, *resultlen, "Error %d unknown\n", errorcode);
    }
  }
  return MPI_SUCCESS;
}

int mpi_errhandler_set(MPI_Comm comm,
		       MPI_Errhandler errhandler) {
  int err;
  switch(errhandler) {
  case MPI_ERRORS_RETURN:
    err = MPI_SUCCESS;
    break;
  case MPI_ERRORS_ARE_FATAL:
  default:
    err = MPI_ERR_UNKNOWN;
    break;
  }
  return err;
}

int mpi_get_version(int *version,
		    int *subversion) {
  *version = MADMPI_VERSION;
  *subversion = MADMPI_SUBVERSION;
  return MPI_SUCCESS;
}

int mpi_request_free(MPI_Request *request) {
  mpir_request_t *mpir_request = mpir_request_find(*request);

  MPI_NMAD_LOG_IN();
  mpir_request->request_type = MPI_REQUEST_ZERO;
  mpir_request->request_persistent_type = MPI_REQUEST_ZERO;
  if (mpir_request->contig_buffer != NULL) {
    FREE_AND_SET_NULL(mpir_request->contig_buffer);
  }
  mpir_request_free(mpir_request);
  *request = MPI_REQUEST_NULL;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int mpi_get_count(MPI_Status *status,
                  MPI_Datatype datatype TBX_UNUSED,
                  int *count) {
  MPI_NMAD_LOG_IN();
  *count = status->count;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int mpi_request_is_equal(MPI_Request request1,
			 MPI_Request request2) {
  mpir_request_t *mpir_request1 = mpir_request_find(request1);
  mpir_request_t *mpir_request2 = mpir_request_find(request2);
  if (mpir_request1->request_type == MPI_REQUEST_ZERO) {
    return (mpir_request1->request_type == mpir_request2->request_type);
  }
  else if (mpir_request2->request_type == MPI_REQUEST_ZERO) {
    return (mpir_request1->request_type == mpir_request2->request_type);
  }
  else {
    return (mpir_request1 == mpir_request2);
  }
}



int mpi_get_address(void *location,
		    MPI_Aint *address) {
  /* This is the "portable" way to generate an address.
     The difference of two pointers is the number of elements
     between them, so this gives the number of chars between location
     and ptr.  As long as sizeof(char) == 1, this will be the number
     of bytes from 0 to location */
  //MPI_NMAD_LOG_IN();
  *address = (MPI_Aint) ((char *)location - (char *)MPI_BOTTOM);
  //MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int mpi_address(void *location,
		MPI_Aint *address) {
  return MPI_Get_address(location, address);
}


int mpi_status_f2c(MPI_Fint *f_status, MPI_Status *c_status)
{
  if (*f_status && c_status) {
    f_status[0] = c_status->MPI_SOURCE;
    f_status[1] = c_status->MPI_TAG;
    f_status[2] = c_status->MPI_ERROR;
    return MPI_SUCCESS;
  }
  return MPI_ERR_ARG;
}

int mpi_status_c2f(MPI_Status *c_status, MPI_Fint *f_status)
{
  if (*f_status && c_status) {
    c_status->MPI_SOURCE = f_status[0];
    c_status->MPI_TAG    = f_status[1];
    c_status->MPI_ERROR  = f_status[2];
    return MPI_SUCCESS;
  }
  return MPI_ERR_ARG;
}


/* Not implemented */

int MPI_Get(void *origin_addr,
	    int origin_count,
	    MPI_Datatype origin_datatype,
	    int target_rank,
	    MPI_Aint target_disp,
            int target_count,
	    MPI_Datatype target_datatype,
	    MPI_Win win)
{
  ERROR("<%s> not implemented\n", __FUNCTION__);
  return MPI_ERR_UNKNOWN;
}


int MPI_Put(void *origin_addr,
	    int origin_count,
	    MPI_Datatype origin_datatype,
	    int target_rank,
	    MPI_Aint target_disp,
            int target_count,
	    MPI_Datatype target_datatype,
	    MPI_Win win)
{
  ERROR("<%s> not implemented\n", __FUNCTION__);
  return MPI_ERR_UNKNOWN;
}


MPI_Win MPI_Win_f2c(MPI_Fint win)
{
  ERROR("<%s> not implemented\n", __FUNCTION__);
  return MPI_ERR_UNKNOWN;
}


MPI_Fint MPI_Win_c2f(MPI_Win win)
{
  ERROR("<%s> not implemented\n", __FUNCTION__);
  return MPI_ERR_UNKNOWN;
}

int MPI_Group_size(MPI_Group group, int *size)
{
  ERROR("<%s> not implemented\n", __FUNCTION__);
  return MPI_ERR_UNKNOWN;
}
