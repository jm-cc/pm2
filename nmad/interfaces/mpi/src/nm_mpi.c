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


#include "nm_mpi_private.h"
#include <assert.h>
#include <errno.h>

#include <Padico/Puk.h>
#include <Padico/Module.h>
PADICO_MODULE_BUILTIN(MadMPI, NULL, NULL, NULL);

static int init_done = 0, finalize_done = 0;

NM_MPI_HANDLE_TYPE(errhandler, struct nm_mpi_errhandler_s, _NM_MPI_ERRHANDLER_OFFSET, 8);
static struct nm_mpi_handle_errhandler_s nm_mpi_errhandlers;

NM_MPI_HANDLE_TYPE(info, struct nm_mpi_info_s, _NM_MPI_INFO_OFFSET, 8);
static struct nm_mpi_handle_info_s nm_mpi_infos;

/* ********************************************************* */
/* Aliases */

NM_MPI_ALIAS(MPI_Init, mpi_init);
NM_MPI_ALIAS(MPI_Init_thread, mpi_init_thread);
NM_MPI_ALIAS(MPI_Initialized, mpi_initialized);
NM_MPI_ALIAS(MPI_Finalize, mpi_finalize);
NM_MPI_ALIAS(MPI_Finalized, mpi_finalized);
NM_MPI_ALIAS(MPI_Abort, mpi_abort);
NM_MPI_ALIAS(MPI_Get_processor_name, mpi_get_processor_name);
NM_MPI_ALIAS(MPI_Wtime, mpi_wtime);
NM_MPI_ALIAS(MPI_Wtick, mpi_wtick);
NM_MPI_ALIAS(MPI_Error_string, mpi_error_string);
NM_MPI_ALIAS(MPI_Error_class, mpi_error_class);
NM_MPI_ALIAS(MPI_Errhandler_create, mpi_errhandler_create);
NM_MPI_ALIAS(MPI_Errhandler_set, mpi_errhandler_set);
NM_MPI_ALIAS(MPI_Errhandler_get, mpi_errhandler_get);
NM_MPI_ALIAS(MPI_Errhandler_free, mpi_errhandler_free);
NM_MPI_ALIAS(MPI_Comm_create_errhandler, mpi_comm_create_errhandler);
NM_MPI_ALIAS(MPI_Comm_set_errhandler, mpi_comm_set_errhandler);
NM_MPI_ALIAS(MPI_Comm_get_errhandler, mpi_comm_get_errhandler);
NM_MPI_ALIAS(MPI_Get_version, mpi_get_version);
NM_MPI_ALIAS(MPI_Get_count, mpi_get_count);
NM_MPI_ALIAS(MPI_Get_elements, mpi_get_elements);
NM_MPI_ALIAS(MPI_Get_elements_x, mpi_get_elements_x);
NM_MPI_ALIAS(MPI_Get_address, mpi_get_address);
NM_MPI_ALIAS(MPI_Address, mpi_address);
NM_MPI_ALIAS(MPI_Alloc_mem, mpi_alloc_mem);
NM_MPI_ALIAS(MPI_Free_mem, mpi_free_mem);
NM_MPI_ALIAS(MPI_Info_create, mpi_info_create);
NM_MPI_ALIAS(MPI_Info_free, mpi_info_free);
NM_MPI_ALIAS(MPI_Info_set, mpi_info_set);
NM_MPI_ALIAS(MPI_Info_get, mpi_info_get);
NM_MPI_ALIAS(MPI_Info_delete, mpi_info_delete);
NM_MPI_ALIAS(MPI_Info_get_nkeys, mpi_info_get_nkeys);
NM_MPI_ALIAS(MPI_Info_get_valuelen, mpi_info_get_valuelen);
NM_MPI_ALIAS(MPI_Pcontrol, mpi_pcontrol);
NM_MPI_ALIAS(MPI_Status_c2f, mpi_status_c2f);
NM_MPI_ALIAS(MPI_Status_f2c, mpi_status_f2c);

/* ********************************************************* */

__PUK_SYM_INTERNAL
struct nm_mpi_errhandler_s*nm_mpi_errhandler_get(int errhandler)
{
  return nm_mpi_handle_errhandler_get(&nm_mpi_errhandlers, errhandler);
}

static void nm_mpi_errhandler_return_function(MPI_Comm*comm, int*err, ...)
{
  fprintf(stderr, "\n# madmpi: WARNING- error %d on communicator %d\n", *err, *comm);
  void*buffer[100];
  int nptrs = backtrace(buffer, 100);
  backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
  fflush(stderr);
}
static void nm_mpi_errhandler_fatal_function(MPI_Comm*comm, int*err, ...)
{
  fprintf(stderr, "\n# madmpi: FATAL- error %d on communicator %d\n", *err, *comm);
  void*buffer[100];
  int nptrs = backtrace(buffer, 100);
  backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);
  fflush(stderr);
  abort();
}

int mpi_init(int*argc, char***argv)
{
  nm_mpi_handle_info_init(&nm_mpi_infos);
  nm_mpi_handle_errhandler_init(&nm_mpi_errhandlers);
  struct nm_mpi_errhandler_s*p_errhandler_return = nm_mpi_handle_errhandler_store(&nm_mpi_errhandlers, MPI_ERRORS_RETURN);
  p_errhandler_return->function = &nm_mpi_errhandler_return_function;
  struct nm_mpi_errhandler_s*p_errhandler_fatal =  nm_mpi_handle_errhandler_store(&nm_mpi_errhandlers, MPI_ERRORS_ARE_FATAL);
  p_errhandler_fatal->function = &nm_mpi_errhandler_fatal_function;
  nm_launcher_init(argc, argv ? *argv : NULL);
  nm_mpi_attrs_init();
  nm_mpi_comm_init();
  nm_mpi_datatype_init();
  nm_mpi_op_init();
  nm_mpi_internal_init();
  nm_mpi_request_init();
  nm_mpi_io_init();
  init_done = 1;
  return MPI_SUCCESS;
}

int mpi_init_thread(int*argc, char***argv, int required, int*provided)
{
  int err;
  err = mpi_init(argc, argv);
#ifndef PIOMAN
  *provided = (required == MPI_THREAD_MULTIPLE) ? MPI_THREAD_FUNNELED : required;
#else /* PIOMAN */
  *provided = required;
#endif /* PIOMAN */
  return err;
}

int mpi_initialized(int*flag)
{
  *flag = init_done;
  return MPI_SUCCESS;
}

int mpi_finalized(int*flag)
{
  *flag = finalize_done;
  return MPI_SUCCESS;
}

int mpi_finalize(void)
{
  int err;
  mpi_barrier(MPI_COMM_WORLD);
  err = nm_mpi_internal_exit();
  nm_mpi_io_exit();
  nm_mpi_datatype_exit();
  nm_mpi_op_exit();
  nm_mpi_comm_exit();
  nm_mpi_request_exit();
  nm_mpi_attrs_exit();
  nm_mpi_handle_errhandler_finalize(&nm_mpi_errhandlers, NULL);
  nm_mpi_handle_info_finalize(&nm_mpi_infos, NULL);
  init_done = 0;
  finalize_done = 1;
  nm_launcher_exit();
  return err;
}

int mpi_abort(MPI_Comm comm TBX_UNUSED, int errorcode)
{
  fprintf(stderr, "# madmpi: FATAL- MPI_Abort err = %d\n", errorcode);
  nm_launcher_abort();
  abort();
  return errorcode;
}


int mpi_get_processor_name(char *name, int *resultlen)
{
  int err;
  MPI_NMAD_LOG_IN();
  err = gethostname(name, MPI_MAX_PROCESSOR_NAME);
  if (!err)
    {
      *resultlen = strlen(name);
      MPI_NMAD_LOG_OUT();
      return MPI_SUCCESS;
    }
  else
    {
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

int mpi_error_class(int errorcode, int*errorclass)
{
  if(errorcode < 32)
    *errorclass = errorcode;
  else
    *errorclass = MPI_ERR_OTHER;
  return MPI_SUCCESS;
}

int mpi_error_string(int errorcode, char *string, int *resultlen)
{
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

int mpi_get_version(int *version, int *subversion)
{
  *version = MADMPI_VERSION;
  *subversion = MADMPI_SUBVERSION;
  return MPI_SUCCESS;
}

int mpi_get_count(MPI_Status*status, MPI_Datatype datatype, int*count)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      *count = MPI_UNDEFINED;
    }
  else
    {
      const size_t size = nm_mpi_datatype_size(p_datatype);
      if(size == 0)
	{
	  *count = 0;
	}
      else if(status->size % size != 0)
	{
	  *count = MPI_UNDEFINED;
	}
      else
	{
	  *count = status->size / size;
	}
    }
  return MPI_SUCCESS;
}

int mpi_get_elements_x(const MPI_Status*status, MPI_Datatype datatype, MPI_Count*count)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  if(p_datatype->elements == 0)
    {
      *count = 0;
      return MPI_SUCCESS;
    }
  const size_t datatype_size = nm_mpi_datatype_size(p_datatype);
  const size_t element_size = datatype_size / p_datatype->elements;
  if(datatype_size == 0)
    {
      *count = 0;
    }
  else if(status->size % element_size != 0)
    {
      *count = MPI_UNDEFINED;
    }
  else
    {
      *count = status->size / element_size;
    }
  return MPI_SUCCESS;
}

int mpi_get_elements(const MPI_Status*status, MPI_Datatype datatype, int*count)
{
  MPI_Count large_count;
  int err = mpi_get_elements_x(status, datatype, &large_count);
  if(err == MPI_SUCCESS)
    {
      if(large_count < INT_MAX || large_count == MPI_UNDEFINED)
	{
	  *count = large_count;
	  return err;
	}
      else
	return MPI_ERR_OTHER;
    }
  else
    return err;
}

int mpi_get_address(void *location, MPI_Aint *address)
{
  /* This is the "portable" way to generate an address.
     The difference of two pointers is the number of elements
     between them, so this gives the number of chars between location
     and ptr.  As long as sizeof(char) == 1, this will be the number
     of bytes from 0 to location */
  *address = (uintptr_t)location;
  return MPI_SUCCESS;
}

int mpi_address(void *location, MPI_Aint *address)
{
  return MPI_Get_address(location, address);
}

int mpi_alloc_mem(MPI_Aint size, MPI_Info info, void*_ptr)
{
  void**ptr = _ptr;
  *ptr = malloc(size);
  return MPI_SUCCESS;
}

int mpi_free_mem(void*ptr)
{
  free(ptr);
  return MPI_SUCCESS;
}

int mpi_info_create(MPI_Info*info)
{
  struct nm_mpi_info_s*p_info = nm_mpi_handle_info_alloc(&nm_mpi_infos);
  *info = p_info->id;
  p_info->content = puk_hashtable_new_string();
  return MPI_SUCCESS;
}

int mpi_info_free(MPI_Info*info)
{
  struct nm_mpi_info_s*p_info = nm_mpi_handle_info_get(&nm_mpi_infos, *info);
  if(p_info == NULL)
    return MPI_ERR_INFO;
  puk_hashtable_delete(p_info->content);
  nm_mpi_handle_info_free(&nm_mpi_infos, p_info);
  *info = MPI_INFO_NULL;
  return MPI_SUCCESS;
}

int mpi_info_set(MPI_Info info, char*key, char*value)
{
  struct nm_mpi_info_s*p_info = nm_mpi_handle_info_get(&nm_mpi_infos, info);
  if(p_info == NULL)
    return MPI_ERR_INFO;
  if(puk_hashtable_probe(p_info->content, key))
    {
      void*oldkey = NULL, *oldvalue = NULL;
      puk_hashtable_lookup2(p_info->content, key, &oldkey, &oldvalue);
      puk_hashtable_remove(p_info->content, key);
      free(oldkey);
      free(oldvalue);
    }
  puk_hashtable_insert(p_info->content, strdup(key), strdup(value));
  return MPI_SUCCESS;
}

int mpi_info_get(MPI_Info info, char*key, int valuelen, char*value, int*flag)
{
  struct nm_mpi_info_s*p_info = nm_mpi_handle_info_get(&nm_mpi_infos, info);
  if(p_info == NULL)
    return MPI_ERR_INFO;
  char*v = puk_hashtable_lookup(p_info->content, key);
  if(v == NULL)
    {
      *flag = 0;
    }
  else
    {
      if(strlen(v) > valuelen)
	{
	  return MPI_ERR_OTHER;
	}
      strcpy(value, v);
      *flag = 1;
    }
  return MPI_SUCCESS;
}

int mpi_info_delete(MPI_Info info, char*key)
{
  struct nm_mpi_info_s*p_info = nm_mpi_handle_info_get(&nm_mpi_infos, info);
  if(p_info == NULL)
    return MPI_ERR_INFO;
  if(!puk_hashtable_probe(p_info->content, key))
      return MPI_ERR_INFO_NOKEY;
  void*oldkey = NULL, *oldvalue = NULL;
  puk_hashtable_lookup2(p_info->content, key, &oldkey, &oldvalue);
  puk_hashtable_remove(p_info->content, key);
  free(oldkey);
  free(oldvalue);
  return MPI_SUCCESS;
}

int mpi_info_get_nkeys(MPI_Info info, int*nkeys)
{
  struct nm_mpi_info_s*p_info = nm_mpi_handle_info_get(&nm_mpi_infos, info);
  if(p_info == NULL)
    return MPI_ERR_INFO;
  *nkeys = puk_hashtable_size(p_info->content);
  return MPI_SUCCESS;
}

int mpi_info_get_valuelen(MPI_Info info, char*key, int*valuelen, int*flag)
{
  struct nm_mpi_info_s*p_info = nm_mpi_handle_info_get(&nm_mpi_infos, info);
  if(p_info == NULL)
    return MPI_ERR_INFO;
  char*v = puk_hashtable_lookup(p_info->content, key);
  if(v == NULL)
    {
      *flag = 0;
    }
  else
    {
      *valuelen = strlen(v);
      *flag = 1;
    }
  return MPI_SUCCESS;
}



/** stub implementation to be overriden by profiling library */
int MPI_Pcontrol(const int level, ...) __attribute__((weak));
int mpi_pcontrol(const int level, ...)
{
  return MPI_SUCCESS;
}

int mpi_errhandler_create(MPI_Handler_function*function, MPI_Errhandler*errhandler)
{
  struct nm_mpi_errhandler_s*p_errhandler = nm_mpi_handle_errhandler_alloc(&nm_mpi_errhandlers);
  p_errhandler->function = function;
  *errhandler = p_errhandler->id;
  return MPI_SUCCESS;
}

int mpi_errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler)
{
  struct nm_mpi_errhandler_s*p_errhandler = nm_mpi_handle_errhandler_get(&nm_mpi_errhandlers, errhandler);
  if(p_errhandler == NULL)
    return MPI_ERR_ARG;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  p_comm->p_errhandler = p_errhandler;
  return MPI_SUCCESS;
}

int mpi_errhandler_get(MPI_Comm comm, MPI_Errhandler*errhandler)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  struct nm_mpi_errhandler_s*p_new_errhandler = nm_mpi_handle_errhandler_alloc(&nm_mpi_errhandlers);
  *p_new_errhandler = *p_comm->p_errhandler,
    *errhandler = p_new_errhandler->id;
  return MPI_SUCCESS;
}

int mpi_errhandler_free(MPI_Errhandler*errhandler)
{
  struct nm_mpi_errhandler_s*p_errhandler = nm_mpi_handle_errhandler_get(&nm_mpi_errhandlers, *errhandler);
  if(p_errhandler == NULL)
    return MPI_ERR_ARG;
  nm_mpi_handle_errhandler_free(&nm_mpi_errhandlers, p_errhandler);
  *errhandler = MPI_ERRHANDLER_NULL;
  return MPI_SUCCESS;
}

int mpi_comm_create_errhandler(MPI_Comm_errhandler_fn*function, MPI_Errhandler*errhandler)
{
  return mpi_errhandler_create(function, errhandler);
}

int mpi_comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
  return mpi_errhandler_set(comm, errhandler);
}

int mpi_comm_get_errhandler(MPI_Comm comm, MPI_Errhandler*errhandler)
{
  return mpi_errhandler_get(comm, errhandler);
}

int mpi_status_f2c(MPI_Fint *f_status, MPI_Status *c_status)
{
  if (*f_status && c_status) {
    c_status->MPI_SOURCE = f_status[0];
    c_status->MPI_TAG    = f_status[1];
    c_status->MPI_ERROR  = f_status[2];
    return MPI_SUCCESS;
  }
  return MPI_ERR_ARG;
}

int mpi_status_c2f(MPI_Status *c_status, MPI_Fint *f_status)
{
  if (*f_status && c_status) {
    f_status[0] = c_status->MPI_SOURCE;
    f_status[1] = c_status->MPI_TAG;
    f_status[2] = c_status->MPI_ERROR;
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


int MPI_Put(const void *origin_addr,
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

