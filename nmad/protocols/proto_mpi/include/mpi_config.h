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

int MPI_Init(int *argc,
             char ***argv);

int MPI_Init_thread(int *argc,
                    char ***argv,
                    int required,
                    int *provided);

int MPI_Finalize(void);

int MPI_Abort(MPI_Comm comm,
              int errorcode);

int MPI_Comm_size(MPI_Comm comm,
                  int *size);

int MPI_Comm_rank(MPI_Comm comm,
                  int *rank);

int MPI_Get_processor_name(char *name, int *resultlen);

double MPI_Wtime(void);

double MPI_Wtick(void);

int MPI_Error_string(int errorcode, char *string, int *resultlen);

int MPI_Get_version(int *version, int *subversion);

#endif /* MPI_CONFIG_H */

