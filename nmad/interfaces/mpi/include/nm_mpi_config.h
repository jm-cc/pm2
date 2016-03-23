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


#ifndef NM_MPI_CONFIG_H
#define NM_MPI_CONFIG_H

/** \addtogroup mpi_interface */
/* @{ */

#define MPI_VERSION 3
#define MPI_SUBVERSION 0

/** @name Functions: Environmental management */
/* @{ */

/**
 * This routine must be called before any other MPI routine. It must
 * be called at most once; subsequent calls are erroneous. 
 * @param argc a pointer to the process argc.
 * @param argv a pointer to the process argv.
 * @return MPI status
 */
int MPI_Init(int *argc,
             char ***argv);

/**
 * The following function may be used to initialize MPI, and
 * initialize the MPI thread environment, instead of MPI_Init().
 *
 * @param argc a pointer to the process argc.
 * @param argv a pointer to the process argv.
 * @param required level of thread support (integer).
 * @param provided level of thread support (integer).
 * @return MPI status
 */
int MPI_Init_thread(int *argc,
                    char ***argv,
                    int required,
                    int *provided);

/**
 * Indicates whether MPI_Init has been called.
 * @param flag a pointer to an int set to 1 if MPI_Init has been called, 0 otherwise.
 */
int MPI_Initialized(int *flag);

/**
 * This routine must be called by each process before it exits. The
 * call cleans up all MPI state.
 * @return MPI status
 */
int MPI_Finalize(void);

/**
 * Indicates whether MPI_Finalize has been called.
 * @param flag a pointer to an int set to 1 if MPI_Finalize has been called, 0 otherwise.
 */
int MPI_Finalized(int*flag);

/**
 * This routine makes a ``best attempt'' to abort all tasks in the group of comm.
 * @param comm communicator of tasks to abort
 * @param errorcode error code to return to invoking environment
 * @return MPI status
 */
int MPI_Abort(MPI_Comm comm,
              int errorcode);

/**
 * This routine returns the name of the processor on which it was
 * called at the moment of the call.
 * @param name unique specifier for the actual (as opposed to virtual) node
 * @param resultlen length (in printable characters) of the result returned in name
 * @return MPI status
 */
int MPI_Get_processor_name(char *name,
			   int *resultlen);

/**
 * Returns a floating-point number of seconds, representing elapsed
 * wall-clock time since some time in the past.
 * @return floating-point number of seconds
 */
double MPI_Wtime(void);

/**
 * Returns the resolution of MPI_Wtime() in seconds.
 * @return resolution of MPI_Wtime() in seconds
 */
double MPI_Wtick(void);

/**
 * Returns the error string associated with an error code or class.
 * The argument string must represent storage that is at least
 * MPI_MAX_ERROR_STRING characters long.
 * @param errorcode error code returned by an MPI routine
 * @param string text that corresponds to the errorcode
 * @param resultlen length (in printable characters) of the result returned in string
 * @return MPI status.
 */
int MPI_Error_string(int errorcode,
		     char *string,
		     int *resultlen);


int MPI_Error_class(int errorcode, int *errorclass);

/**
 * Returns the version
 * @param version version number
 * @param subversion subversion number
 * @return MPI status.
 */
int MPI_Get_version(int *version,
		    int *subversion);

int MPI_Pcontrol(const int level, ...);

int MPI_Errhandler_create(MPI_Handler_function*function, MPI_Errhandler*errhandler);

int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);

int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler*errhandler);

int MPI_Errhandler_free(MPI_Errhandler*errhandler);

int MPI_Comm_create_errhandler(MPI_Comm_errhandler_fn*function, MPI_Errhandler*errhandler);

int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler);

int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler*errhandler);

/**
 * Returns the byte address of location.
 * @param location location in caller memory
 * @param address address of location
 * @return MPI status
 */
int MPI_Get_address(void *location,
		    MPI_Aint *address);

/**
 * Returns the byte address of location.
 * @param location location in caller memory
 * @param address address of location
 * @return MPI status
 */
int MPI_Address(void *location,
		MPI_Aint *address);

/** Allocate memory for message passing 
 */
int MPI_Alloc_mem(MPI_Aint size,
		  MPI_Info info,
		  void*baseptr);

/** Free memory allocated with MPI_Alloc_mem
 */
int MPI_Free_mem(void*base);

/** Creates a new info object
 */
int MPI_Info_create(MPI_Info*info);

/** Frees an info object
 */
int MPI_Info_free(MPI_Info*info);

/** Adds a <key, value> pair to info object
 */
int MPI_Info_set(MPI_Info info,
		 char*key,
		 char*value);

/** Retrieves the value associated with a key
 */
int MPI_Info_get(MPI_Info info,
		 char*key,
		 int valuelen,
		 char*value,
		 int*flag);

/** Deletes a <key, value> pair from info object
 */
int MPI_Info_delete(MPI_Info info,
		    char*key);

/** Returns the number of currently defined keys in info
 */
int MPI_Info_get_nkeys(MPI_Info info,
		       int*nkeys);

/** Retrieves the length of the value associated with a key
 */
int MPI_Info_get_valuelen(MPI_Info info,
			  char*key,
			  int*valuelen,
			  int*flag);

/* @} */
/* @}*/

#endif /* NM_MPI_CONFIG_H */

