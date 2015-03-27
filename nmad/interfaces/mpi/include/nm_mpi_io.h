/*
 * NewMadeleine
 * Copyright (C) 2015 (see AUTHORS file)
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


#ifndef NM_MPI_IO_H
#define NM_MPI_IO_H

typedef int MPI_File;

#define MPI_FILE_NULL ((MPI_File)0)
#define _NM_MPI_FILE_OFFSET 1

typedef uint64_t MPI_Offset;

#define MPI_MODE_RDONLY            0x0001
#define MPI_MODE_RDWR              0x0002
#define MPI_MODE_WRONLY            0x0004
#define MPI_MODE_CREATE            0x0008
#define MPI_MODE_EXCL              0x0010
#define MPI_MODE_APPEND            0x0020
#define MPI_MODE_DELETE_ON_CLOSE   0x0100
#define MPI_MODE_UNIQUE_OPEN       0x0200
#define MPI_MODE_SEQUENTIAL        0x0400

int MPI_File_open(MPI_Comm comm, char*filename, int amode, MPI_Info info, MPI_File*fh);
int MPI_File_close(MPI_File*fh);
int MPI_File_delete(char*filename, MPI_Info info);
int MPI_File_set_size(MPI_File fh, MPI_Offset size);
int MPI_File_preallocate(MPI_File fh, MPI_Offset size);
int MPI_File_get_size(MPI_File fh, MPI_Offset*size);
int MPI_File_get_group(MPI_File fh, MPI_Group*group);
int MPI_File_get_amode(MPI_File fh, int*amode);
int MPI_File_set_info(MPI_File fh, MPI_Info info);
int MPI_File_get_info(MPI_File fh, MPI_Info*info);

int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void*buf, int count, MPI_Datatype datatype, MPI_Status*status);
int MPI_File_write_at(MPI_File fh, MPI_Offset offset, void*buf, int count, MPI_Datatype datatype, MPI_Status*status);


#endif /* NM_MPI_IO_H */
