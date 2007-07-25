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
 * mpi_config.h
 * ============
 */

#ifndef MPI_CONFIG_H
#define MPI_CONFIG_H

/** \addtogroup mpi_interface */
/* @{ */

/**
 * This routine must be called before any other MPI routine. It must
 * be called at most once; subsequent calls are erroneous.
 */
int MPI_Init(int *argc,
             char ***argv);

/**
 * The following function may be used to initialize MPI, and
 * initialize the MPI thread environment, instead of MPI_Init.
 *
 * @param argc a pointer to the process argc.
 * @param argv a pointer to the process argv.
 * @param required level of thread support (integer).
 * @param provided level of thread support (integer).
 * @return The MPI status.
 */
int MPI_Init_thread(int *argc,
                    char ***argv,
                    int required,
                    int *provided);

/**
 * This routine must be called by each process before it exits. The
 * call cleans up all MPI state.
 */
int MPI_Finalize(void);

/**
 * This routine makes a ``best attempt'' to abort all tasks in the group of comm.
 */
int MPI_Abort(MPI_Comm comm,
              int errorcode);

/**
 * This function indicates the number of processes involved in a an
 * intracommunicator.
 */
int MPI_Comm_size(MPI_Comm comm,
                  int *size);

/**
 * This function gives the rank of the process in the particular
 * communicator's group.
 */
int MPI_Comm_rank(MPI_Comm comm,
                  int *rank);

/**
 * This routine returns the name of the processor on which it was
 * called at the moment of the call.
 */
int MPI_Get_processor_name(char *name, int *resultlen);

/**
 * Returns a floating-point number of seconds, representing elapsed
 * wall-clock time since some time in the past.
 */
double MPI_Wtime(void);

/**
 * Returns the resolution of MPI_WTIME in seconds.
 */
double MPI_Wtick(void);

/**
 * Returns the error string associated with an error code or class.
 * The argument string must represent storage that is at least
 * MPI_MAX_ERROR_STRING characters long.
 */
int MPI_Error_string(int errorcode, char *string, int *resultlen);

/**
 * Returns the version
 */
int MPI_Get_version(int *version, int *subversion);

/* @}*/

#endif /* MPI_CONFIG_H */

