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
 * mpi_nmad_private_coll.c
 * =======================
 */

#include <stdint.h>
#include "mpi.h"
#include "mpi_nmad_private.h"
#include "nm_so_parameters.h"

extern mpir_internal_data_t *get_mpir_internal_data();

void mpir_op_max(void *invec,
		 void *inoutvec,
		 int *len,
		 MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        if (i_invec[i] > i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_MAX */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        if (i_invec[i] > i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_MAX */
    default : {
      ERROR("Datatype %d for MAX Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_min(void *invec,
		 void *inoutvec,
		 int *len,
		 MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
          if (i_invec[i] < i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_MIN */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        if (i_invec[i] < i_inoutvec[i]) i_inoutvec[i] = i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_MAX */
    default : {
      ERROR("Datatype %d for MIN Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_sum(void *invec,
		 void *inoutvec,
		 int *len,
		 MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        MPI_NMAD_TRACE("Summing %d and %d\n", i_inoutvec[i], i_invec[i]);
        i_inoutvec[i] += i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_SUM */
    case MPI_FLOAT : {
      float *i_invec = (float *) invec;
      float *i_inoutvec = (float *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        MPI_NMAD_TRACE("Summing %f and %f\n", i_inoutvec[i], i_invec[i]);
        i_inoutvec[i] += i_invec[i];
      }
      break;
    } /* END MPI_FLOAT FOR MPI_SUM */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] += i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_SUM */
    case MPI_DOUBLE_COMPLEX : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] += i_invec[i];
      }
      break;
    }
    case MPI_LONG : {
      long *i_invec = (long *) invec;
      long *i_inoutvec = (long *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] += i_invec[i];
      }
      break;
    }
    default : {
      ERROR("Datatype %d for SUM Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_prod(void *invec,
		  void *inoutvec,
		  int *len,
		  MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] *= i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_PROD */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] *= i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_PROD */
    case MPI_LONG : {
      long *i_invec = (long *) invec;
      long *i_inoutvec = (long *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] *= i_invec[i];
      }
      break;
    } /* END MPI_LONG FOR MPI_PROD */
    default : {
      ERROR("Datatype %d for PROD Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_land(void *invec,
		  void *inoutvec,
		  int *len,
		  MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] && i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_LAND */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] && i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_LAND */
    case MPI_LONG : {
      long *i_invec = (long *) invec;
      long *i_inoutvec = (long *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] && i_invec[i];
      }
      break;
    } /* END MPI_LONG FOR MPI_LAND */
    default : {
      ERROR("Datatype %d for LAND Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_band(void *invec,
		  void *inoutvec,
		  int *len,
		  MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] & i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_BAND */
    case MPI_LONG : {
      long *i_invec = (long *) invec;
      long *i_inoutvec = (long *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] & i_invec[i];
      }
      break;
    } /* END MPI_LONG FOR MPI_BAND */
    default : {
      ERROR("Datatype %d for BAND Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_lor(void *invec,
		 void *inoutvec,
		 int *len,
		 MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] || i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_LOR */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] || i_invec[i];
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_LOR */
    case MPI_LONG : {
      long *i_invec = (long *) invec;
      long *i_inoutvec = (long *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] || i_invec[i];
      }
      break;
    } /* END MPI_LONG FOR MPI_LOR */
    default : {
      ERROR("Datatype %d for LOR Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_bor(void *invec,
		 void *inoutvec,
		 int *len,
		 MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] | i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_BOR */
    case MPI_LONG : {
      long *i_invec = (long *) invec;
      long *i_inoutvec = (long *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] | i_invec[i];
      }
      break;
    } /* END MPI_LONG FOR MPI_BOR */
    default : {
      ERROR("Datatype %d for BOR Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_lxor(void *invec,
		  void *inoutvec,
		  int *len,
		  MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
	if ((!i_inoutvec[i] && i_invec[i]) ||
	    (i_inoutvec[i] && !i_invec[i]))
	  i_inoutvec[i] = 1;
	else
	  i_inoutvec[i] = 0;
      }
      break;
    } /* END MPI_INT FOR MPI_LXOR */
    case MPI_DOUBLE_PRECISION :
    case MPI_DOUBLE : {
      double *i_invec = (double *) invec;
      double *i_inoutvec = (double *) inoutvec;
      for(i=0 ; i<*len ; i++) {
	if ((!i_inoutvec[i] && i_invec[i]) ||
	    (i_inoutvec[i] && !i_invec[i]))
	  i_inoutvec[i] = 1;
	else
	  i_inoutvec[i] = 0;
      }
      break;
    } /* END MPI_DOUBLE FOR MPI_LXOR */
    case MPI_LONG : {
      long *i_invec = (long *) invec;
      long *i_inoutvec = (long *) inoutvec;
      for(i=0 ; i<*len ; i++) {
	if ((!i_inoutvec[i] && i_invec[i]) ||
	    (i_inoutvec[i] && !i_invec[i]))
	  i_inoutvec[i] = 1;
	else
	  i_inoutvec[i] = 0;
      }
      break;
    } /* END MPI_LONG FOR MPI_LXOR */
    default : {
      ERROR("Datatype %d for LXOR Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_bxor(void *invec,
		  void *inoutvec,
		  int *len,
		  MPI_Datatype *type) {
  int i;
  switch (*type) {
    case MPI_INTEGER :
    case MPI_INT : {
      int *i_invec = (int *) invec;
      int *i_inoutvec = (int *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] ^ i_invec[i];
      }
      break;
    } /* END MPI_INT FOR MPI_BXOR */
    case MPI_LONG : {
      long *i_invec = (long *) invec;
      long *i_inoutvec = (long *) inoutvec;
      for(i=0 ; i<*len ; i++) {
        i_inoutvec[i] = i_inoutvec[i] ^ i_invec[i];
      }
      break;
    } /* END MPI_LONG FOR MPI_BXOR */
    default : {
      ERROR("Datatype %d for BXOR Reduce operation", *type);
      break;
    }
  }
}

void mpir_op_maxloc(void *invec,
		    void *inoutvec,
		    int *len,
		    MPI_Datatype *type) {
  int i, _len = *len;
  mpir_internal_data_t *mpir_internal_data = get_mpir_internal_data();
  mpir_datatype_t *dtype = mpir_get_datatype(mpir_internal_data, *type);

  if ((dtype)->dte_type == MPIR_CONTIG && ((dtype)->elements == 2)) {
    MPI_Datatype oldtype = (dtype)->old_types[0];

    /** Set the actual length */
    _len = *len * (dtype)->elements;

    /** Perform the operation */
    switch (oldtype) {
    case MPI_INT: {
      int *a = (int *)inoutvec; int *b = (int *)invec;
      for ( i=0; i<_len; i+=2 ) {
        if (a[i] == b[i])
          a[i+1] = tbx_min(a[i+1],b[i+1]);
        else if (a[i] < b[i]) {
          a[i]   = b[i];
          a[i+1] = b[i+1];
        }
      }
      break;
    }
    case MPI_LONG: {
      long *a = (long *)inoutvec; long *b = (long *)invec;
      for ( i=0; i<_len; i+=2 ) {
        if (a[i] == b[i])
          a[i+1] = tbx_min(a[i+1],b[i+1]);
        else if (a[i] < b[i]) {
          a[i]   = b[i];
          a[i+1] = b[i+1];
        }
      }
      break;
    }
    case MPI_SHORT: {
      short *a = (short *)inoutvec; short *b = (short *)invec;
      for ( i=0; i<_len; i+=2 ) {
        if (a[i] == b[i])
          a[i+1] = tbx_min(a[i+1],b[i+1]);
        else if (a[i] < b[i]) {
          a[i]   = b[i];
          a[i+1] = b[i+1];
        }
      }
      break;
    }
    case MPI_CHAR: {
      char *a = (char *)inoutvec; char *b = (char *)invec;
      for ( i=0; i<_len; i+=2 ) {
        if (a[i] == b[i])
          a[i+1] = tbx_min(a[i+1],b[i+1]);
        else if (a[i] < b[i]) {
          a[i]   = b[i];
          a[i+1] = b[i+1];
        }
      }
      break;
    }
    case MPI_FLOAT: {
      float *a = (float *)inoutvec; float *b = (float *)invec;
      for ( i=0; i<_len; i+=2 ) {
        if (a[i] == b[i])
          a[i+1] = tbx_min(a[i+1],b[i+1]);
        else if (a[i] < b[i]) {
          a[i]   = b[i];
          a[i+1] = b[i+1];
        }
      }
      break;
    }
    case MPI_DOUBLE: {
      double *a = (double *)inoutvec; double *b = (double *)invec;
      for ( i=0; i<_len; i+=2 ) {
        if (a[i] == b[i])
          a[i+1] = tbx_min(a[i+1],b[i+1]);
        else if (a[i] < b[i]) {
          a[i]   = b[i];
          a[i+1] = b[i+1];
        }
      }
      break;
    }
    default:
      ERROR("Datatype Contiguous(%d) for MAXLOC Reduce operation", *type);
      break;
    }
  }
  else {
    ERROR("Datatype %d for MAXLOC Reduce operation", *type);
  }
}

void mpir_op_minloc(void *invec,
		    void *inoutvec,
		    int *len,
		    MPI_Datatype *type) {
  int i, _len = *len;
  mpir_internal_data_t *mpir_internal_data = get_mpir_internal_data();
  mpir_datatype_t *dtype = mpir_get_datatype(mpir_internal_data, *type);

  if ((dtype)->dte_type == MPIR_CONTIG && ((dtype)->elements == 2)) {
    MPI_Datatype oldtype = (dtype)->old_types[0];

    /** Set the actual length */
    _len = *len * (dtype)->elements;

    /** Perform the operation */
    switch (oldtype) {
    case MPI_INT: {
      int *a = (int *)inoutvec; int *b = (int *)invec;
      for ( i=0; i<_len; i+=2 ) {
	if (a[i] == b[i])
	  a[i+1] = tbx_min(a[i+1],b[i+1]);
	else if (a[i] > b[i]) {
	  a[i]   = b[i];
	  a[i+1] = b[i+1];
	}
      }
      break;
    }
    case MPI_LONG: {
      long *a = (long *)inoutvec; long *b = (long *)invec;
      for ( i=0; i<_len; i+=2 ) {
        if (a[i] == b[i])
          a[i+1] = tbx_min(a[i+1],b[i+1]);
        else if (a[i] > b[i]) {
          a[i]   = b[i];
          a[i+1] = b[i+1];
        }
      }
      break;
    }
    case MPI_SHORT: {
      short *a = (short *)inoutvec; short *b = (short *)invec;
      for ( i=0; i<_len; i+=2 ) {
        if (a[i] == b[i])
          a[i+1] = tbx_min(a[i+1],b[i+1]);
        else if (a[i] > b[i]) {
          a[i]   = b[i];
          a[i+1] = b[i+1];
        }
      }
      break;
    }
    case MPI_CHAR: {
      char *a = (char *)inoutvec; char *b = (char *)invec;
      for ( i=0; i<_len; i+=2 ) {
        if (a[i] == b[i])
          a[i+1] = tbx_min(a[i+1],b[i+1]);
        else if (a[i] > b[i]) {
          a[i]   = b[i];
          a[i+1] = b[i+1];
        }
      }
      break;
    }
    case MPI_FLOAT: {
      float *a = (float *)inoutvec; float *b = (float *)invec;
      for ( i=0; i<_len; i+=2 ) {
        if (a[i] == b[i])
          a[i+1] = tbx_min(a[i+1],b[i+1]);
        else if (a[i] > b[i]) {
          a[i]   = b[i];
          a[i+1] = b[i+1];
        }
      }
      break;
    }
    case MPI_DOUBLE: {
      double *a = (double *)inoutvec; double *b = (double *)invec;
      for ( i=0; i<_len; i+=2 ) {
        if (a[i] == b[i])
          a[i+1] = tbx_min(a[i+1],b[i+1]);
        else if (a[i] > b[i]) {
          a[i]   = b[i];
          a[i+1] = b[i+1];
        }
      }
      break;
    }
    default:
      ERROR("Datatype Contiguous(%d) for MINLOC Reduce operation", *type);
      break;
    }
  }
  else {
    ERROR("Datatype %d for MINLOC Reduce operation", *type);
  }
}
