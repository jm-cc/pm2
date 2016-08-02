/*
 * NewMadeleine
 * Copyright (C) 2015-2016 (see AUTHORS file)
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

#ifndef NM_MPI_ONESIDED_H
#define NM_MPI_ONESIDED_H

/** \addtogroup mpi_interface */
/* @{ */

/** @name preliminary one-sided interface */
/* @{ */

int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
		   MPI_Comm comm, MPI_Win *win);

int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info,
		     MPI_Comm comm, void *baseptr, MPI_Win *win);

int MPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info,
			    MPI_Comm comm, void *baseptr, MPI_Win *win);

int MPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size,
			 int *disp_unit, void *baseptr);

int MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win);

int MPI_Win_attach(MPI_Win win, void *base, MPI_Aint size);

int MPI_Win_detach(MPI_Win win, const void *base);

int MPI_Win_free(MPI_Win *win);

int MPI_Win_get_group(MPI_Win win, MPI_Group *group);

int MPI_Win_set_info(MPI_Win win, MPI_Info info);

int MPI_Win_create_keyval(MPI_Win_copy_attr_function*copy_fn,
			  MPI_Win_delete_attr_function*delete_fn,
			  int*keyval,
			  void*extra_state);

int MPI_Win_free_keyval(int*keyval);

int MPI_Win_delete_attr(MPI_Win win, int keyval);

int MPI_Win_get_info(MPI_Win win, MPI_Info *info_used);

int MPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val);

int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag);

int MPI_Win_set_name(MPI_Win win, char*win_name);

int MPI_Win_get_name(MPI_Win win, char*win_name, int*resultlen);

int MPI_Win_create_errhandler(MPI_Win_errhandler_fn*function, MPI_Errhandler*errhandler);

int MPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler);

int MPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler*errhandler);

int MPI_Win_call_errhandler(MPI_Win win, int errorcode);
  

int MPI_Put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
	    int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
	    MPI_Win win);

int MPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
	    int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
	    MPI_Win win);

int MPI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
		   int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
		   MPI_Op op, MPI_Win win);


int MPI_Get_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
		       void *result_addr, int result_count, MPI_Datatype result_datatype,
		       int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
		       MPI_Op op, MPI_Win win);

int MPI_Fetch_and_op(const void *origin_addr, void *result_addr,
		     MPI_Datatype datatype, int target_rank, MPI_Aint target_disp,
		     MPI_Op op, MPI_Win win);

int MPI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
			 void *result_addr, MPI_Datatype datatype, int target_rank,
			 MPI_Aint target_disp, MPI_Win win);


int MPI_Rput(const void *origin_addr, int origin_count,
	     MPI_Datatype origin_datatype, int target_rank,
	     MPI_Aint target_disp, int target_count,
	     MPI_Datatype target_datatype, MPI_Win win,
	     MPI_Request *request);

int MPI_Rget(void *origin_addr, int origin_count,
	     MPI_Datatype origin_datatype, int target_rank,
	     MPI_Aint target_disp, int target_count,
	     MPI_Datatype target_datatype, MPI_Win win,
	     MPI_Request *request);

int MPI_Raccumulate(const void *origin_addr, int origin_count,
		    MPI_Datatype origin_datatype, int target_rank,
		    MPI_Aint target_disp, int target_count,
		    MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
		    MPI_Request *request);

int MPI_Rget_accumulate(const void *origin_addr, int origin_count,
			MPI_Datatype origin_datatype, void *result_addr,
			int result_count, MPI_Datatype result_datatype,
			int target_rank, MPI_Aint target_disp, int target_count,
			MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
			MPI_Request *request);



int MPI_Win_fence(int assert, MPI_Win win);

int MPI_Win_start(MPI_Group group, int assert, MPI_Win win);

int MPI_Win_complete(MPI_Win win);

int MPI_Win_post(MPI_Group group, int assert, MPI_Win win);

int MPI_Win_wait(MPI_Win win);

int MPI_Win_test(MPI_Win win, int *flag);

int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win);

int MPI_Win_lock_all(int assert, MPI_Win win);

int MPI_Win_unlock(int rank, MPI_Win win);

int MPI_Win_unlock_all(MPI_Win win);

int MPI_Win_flush(int rank, MPI_Win win);

int MPI_Win_flush_all(MPI_Win win);

int MPI_Win_flush_local(int rank, MPI_Win win);

int MPI_Win_flush_local_all(MPI_Win win);

int MPI_Win_sync(MPI_Win win);



MPI_Win MPI_Win_f2c(MPI_Fint win);

MPI_Fint MPI_Win_c2f(MPI_Win win);

  
/* @}*/
/* @}*/

#endif /* NM_MPI_ONESIDED_H */
