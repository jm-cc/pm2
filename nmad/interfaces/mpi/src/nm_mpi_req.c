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

#include "mpi.h"
#include "mpi_nmad_private.h"
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);


/* ********************************************************* */

int MPI_Waitsome(int incount,
		 MPI_Request *array_of_requests,
		 int *outcount,
		 int *array_of_indices,
		 MPI_Status *array_of_statuses) __attribute__ ((alias ("mpi_waitsome")));

int MPI_Wait(MPI_Request *request,
	     MPI_Status *status) __attribute__ ((alias ("mpi_wait")));

int MPI_Waitall(int count,
		MPI_Request *array_of_requests,
		MPI_Status *array_of_statuses) __attribute__ ((alias ("mpi_waitall")));

int MPI_Waitany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                MPI_Status *status) __attribute__ ((alias ("mpi_waitany")));

int MPI_Test(MPI_Request *request,
             int *flag,
             MPI_Status *status) __attribute__ ((alias ("mpi_test")));

int MPI_Testany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                int *flag,
                MPI_Status *status) __attribute__ ((alias ("mpi_testany")));

int MPI_Testall(int count,
		MPI_Request *array_of_requests,
		int *flag,
		MPI_Status *statuses) __attribute__ ((alias ("mpi_testall")));

int MPI_Testsome(int count,
		 MPI_Request *array_of_requests,
		 int *outcount,
		 int *indices,
		 MPI_Status *statuses) __attribute__ ((alias ("mpi_testsome")));

int MPI_Cancel(MPI_Request *request) __attribute__ ((alias ("mpi_cancel")));

int MPI_Start(MPI_Request *request) __attribute__ ((alias ("mpi_start")));

int MPI_Startall(int count,
                 MPI_Request *array_of_requests) __attribute__ ((alias ("mpi_startall")));

/* ********************************************************* */

int mpi_waitsome(int incount,
		 MPI_Request *array_of_requests,
		 int *outcount,
		 int *array_of_indices,
		 MPI_Status *array_of_statuses) {
#warning Implement MPI_Waitsome !
  return MPI_ERR_UNKNOWN;
}

int mpi_wait(MPI_Request *request,
	     MPI_Status *status) {
  mpir_request_t  *mpir_request = mpir_request_find(*request);
  int                       err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();

  if(mpir_request == NULL)
    {
      err = MPI_ERR_REQUEST;
      goto out; 
    }
  err = mpir_wait(&mpir_internal_data, mpir_request);

  if (status != MPI_STATUS_IGNORE) {
      err = mpir_set_status(&mpir_internal_data, mpir_request, status);
  }

  if (mpir_request->request_persistent_type == MPI_REQUEST_ZERO) {
    mpir_request->request_type = MPI_REQUEST_ZERO;
  }
  if (mpir_request->request_ptr != NULL) {
    FREE_AND_SET_NULL(mpir_request->request_ptr);
  }

  // Release one active communication for that type
  if (mpir_request->request_datatype > MPI_PACKED) {
    err = mpir_type_unlock(&mpir_internal_data, mpir_request->request_datatype);
  }
  
  mpir_request_free(mpir_request);
 out:
  *request = MPI_REQUEST_NULL;
  
  MPI_NMAD_TRACE("Request completed\n");
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_waitall(int count,
		MPI_Request *array_of_requests,
		MPI_Status *array_of_statuses) {
  int err = NM_ESUCCESS;
  int i;

  MPI_NMAD_LOG_IN();

  if (array_of_statuses == MPI_STATUSES_IGNORE) {
    for (i = 0; i < count; i++) {
      err =  MPI_Wait(&(array_of_requests[i]), MPI_STATUS_IGNORE);
      if (err != NM_ESUCCESS)
        goto out;
    }
  }
  else {
    for (i = 0; i < count; i++) {
      err =  MPI_Wait(&(array_of_requests[i]), &(array_of_statuses[i]));
      if (err != NM_ESUCCESS)
        goto out;
    }
  }

 out:
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_waitany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                MPI_Status *status) {
  int flag, i, err;
  int count_null;

  MPI_NMAD_LOG_IN();

  while (1) {
    count_null = 0;
    for(i=0 ; i<count ; i++) {
      mpir_request_t *mpir_request = mpir_request_find(array_of_requests[i]);
      if (mpir_request->request_type == MPI_REQUEST_ZERO) {
        count_null++;
        continue;
      }
      err = MPI_Test(&(array_of_requests[i]), &flag, status);
      if (flag) {
        *rqindex = i;

	MPI_NMAD_LOG_OUT();
        return err;
      }
    }
    if (count_null == count) {
      *rqindex = MPI_UNDEFINED;
      MPI_NMAD_LOG_OUT();
      return MPI_SUCCESS;
    }
  }
}

int mpi_test(MPI_Request *request,
             int *flag,
             MPI_Status *status) {
  mpir_request_t *mpir_request = mpir_request_find(*request);
  int             err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();
  err = mpir_test(&mpir_internal_data, mpir_request);

  if (err == NM_ESUCCESS) {
    *flag = 1;

    if (status != MPI_STATUS_IGNORE) {
      err = mpir_set_status(&mpir_internal_data, mpir_request, status);
    }

    if (mpir_request->request_persistent_type == MPI_REQUEST_ZERO) {
      mpir_request->request_type = MPI_REQUEST_ZERO;
    }
    if (mpir_request->request_ptr != NULL) {
      FREE_AND_SET_NULL(mpir_request->request_ptr);
    }

    // Release one active communication for that type
    if (mpir_request->request_datatype > MPI_PACKED) {
      err = mpir_type_unlock(&mpir_internal_data, mpir_request->request_datatype);
    }
  }
  else { /* err == -NM_EAGAIN */
    *flag = 0;
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_testany(int count,
                MPI_Request *array_of_requests,
                int *rqindex,
                int *flag,
                MPI_Status *status) {
  int i, err = 0;
  int count_inactive = 0;
  mpir_request_t *mpir_request;

  MPI_NMAD_LOG_IN();

  for(i=0 ; i<count ; i++) {
    mpir_request = mpir_request_find(array_of_requests[i]);
    if (mpir_request->request_type == MPI_REQUEST_ZERO) {
      count_inactive++;
      continue;
    }

    err = MPI_Test(&(array_of_requests[i]), flag, status);
    if (*flag == 1) {
      *rqindex = i;
      MPI_NMAD_LOG_OUT();
      return err;
    }
  }

  *rqindex = MPI_UNDEFINED;
  if (count_inactive == count) {
    *flag = 1;
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_testall(int count,
		MPI_Request *array_of_requests,
		int *flag,
		MPI_Status *statuses) {
  int i, err = 0;
  int count_inactive = 0;
  mpir_request_t *mpir_request;

  MPI_NMAD_LOG_IN();

  /* flag is true only if all requests have completed. Otherwise, flag is
   * false and neither the array_of_requests nor the array_of_statuses is
   * modified.
   */
  for(i=0 ; i<count ; i++) {
    mpir_request = mpir_request_find(array_of_requests[i]);
    if (mpir_request->request_type == MPI_REQUEST_ZERO) {
      count_inactive++;
      continue;
    }

    err = mpir_test(&mpir_internal_data, mpir_request);
    if(err != NM_ESUCCESS) {
      /* at least one request is not completed */
      *flag = 0;
      MPI_NMAD_LOG_OUT();
      return err;
    }
  }

  /* all the requests are completed */
  for(i=0 ; i<count ; i++) {
    mpir_request = mpir_request_find(array_of_requests[i]);
    if (mpir_request->request_type == MPI_REQUEST_ZERO) {
      count_inactive++;
      continue;
    }

    err = MPI_Test(&(array_of_requests[i]), flag, &(statuses[i]));
    if(*flag != 1) {
      /* at least one request is not completed */
      ERROR("Error during MPI_Testall: request #%d should be completed, but it is not !");
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }

  }

  *flag = 1;

  MPI_NMAD_LOG_OUT();
  return err;
}

/* TODO : handle the case where statuses == MPI_STATUSES_IGNORE */
int mpi_testsome(int count,
		 MPI_Request *array_of_requests,
		 int *outcount,
		 int *indices,
		 MPI_Status *statuses) {
  int i, flag, err = 0;
  int count_inactive = 0;
  mpir_request_t *mpir_request;

  MPI_NMAD_LOG_IN();

  *outcount = 0;

  for(i=0 ; i<count ; i++) {
    mpir_request = mpir_request_find(array_of_requests[i]);
    if (mpir_request->request_type == MPI_REQUEST_ZERO) {
      count_inactive++;
      continue;
    }

    err = mpi_test(&(array_of_requests[i]), &flag, &(statuses[*outcount]));
    if (flag == 1) {
      indices[*outcount] = i;
      (*outcount)++;
    }
  }

  if (count_inactive == count)
    *outcount = MPI_UNDEFINED;
  
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_cancel(MPI_Request *request) {
  mpir_request_t *mpir_request = mpir_request_find(*request);
  int             err = NM_ESUCCESS;

  MPI_NMAD_LOG_IN();

  MPI_NMAD_TRACE("Request is cancelled\n");

  err = mpir_cancel(&mpir_internal_data, mpir_request);
  mpir_request->request_type = MPI_REQUEST_CANCELLED;
  if (mpir_request->contig_buffer != NULL) {
    FREE_AND_SET_NULL(mpir_request->contig_buffer);
  }

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_start(MPI_Request *request) {
  mpir_request_t *mpir_request = mpir_request_find(*request);
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Start request\n");

  mpir_request->request_type = mpir_request->request_persistent_type;

  err =  mpir_start(&mpir_internal_data, mpir_request);

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_startall(int count,
                 MPI_Request *array_of_requests) {
  int i;
  int err = MPI_SUCCESS;

  MPI_NMAD_LOG_IN();
  for(i=0 ; i<count ; i++) {
    err = MPI_Start(&(array_of_requests[i]));
    if (err != MPI_SUCCESS) {
      MPI_NMAD_LOG_OUT();
      return err;
    }
  }
  MPI_NMAD_LOG_OUT();
  return err;
}

