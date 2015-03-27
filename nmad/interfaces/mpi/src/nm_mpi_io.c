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

#include "nm_mpi_private.h"

#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);

/** Internal MPI file */
typedef struct nm_mpi_file_s
{
  /** identifier of the file */
  MPI_File id;
  /** access mode */
  int amode;
  /** communicator attached to file */
  struct nm_mpi_communicator_s*p_comm;
  /** file kind */
  enum
    {
      NM_MPI_FILE_POSIX
    } kind;
  /** POSIX file fields */
  int fd;
} nm_mpi_file_t;
 
NM_MPI_HANDLE_TYPE(file, struct nm_mpi_file_s, _NM_MPI_FILE_OFFSET, 256);

static struct nm_mpi_handle_file_s nm_mpi_files;

/* ********************************************************* */

NM_MPI_ALIAS(MPI_File_open,     mpi_file_open);
NM_MPI_ALIAS(MPI_File_close,    mpi_file_close);
NM_MPI_ALIAS(MPI_File_read_at,  mpi_file_read_at);
NM_MPI_ALIAS(MPI_File_write_at, mpi_file_write_at);

/* ********************************************************* */

__PUK_SYM_INTERNAL
void nm_mpi_io_init(void)
{
  nm_mpi_handle_file_init(&nm_mpi_files);
}

__PUK_SYM_INTERNAL
void nm_mpi_io_exit(void)
{
  nm_mpi_handle_file_finalize(&nm_mpi_files, NULL /* &nm_mpi_files_free */);
}

/* ********************************************************* */

int mpi_file_open(MPI_Comm comm, char*filename, int amode, MPI_Info info, MPI_File*fh)
{
  nm_mpi_file_t*p_file = nm_mpi_handle_file_alloc(&nm_mpi_files);
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  p_file->amode = amode;
  p_file->p_comm = p_comm;
  p_file->kind = NM_MPI_FILE_POSIX;
  const int mode =
    ((amode & MPI_MODE_RDONLY) ? O_RDONLY : 0) |
    ((amode & MPI_MODE_WRONLY) ? O_WRONLY : 0) |
    ((amode & MPI_MODE_RDWR)   ? O_RDWR   : 0) |
    ((amode & MPI_MODE_CREATE) ? O_CREAT  : 0) |
    ((amode & MPI_MODE_EXCL)   ? O_EXCL   : 0) |
    ((amode & MPI_MODE_APPEND) ? O_APPEND : 0) ;
  int rc = open(filename, mode);
  if(rc >= 0)
    p_file->fd = rc;
  else
    {
      return MPI_ERR_FILE;
    }
  *fh = p_file->id;
  return MPI_SUCCESS;
}

int mpi_file_close(MPI_File*fh)
{
  nm_mpi_file_t*p_file = nm_mpi_handle_file_get(&nm_mpi_files, *fh);
  if(p_file == NULL)
    return MPI_ERR_BAD_FILE;
  close(p_file->fd);
  nm_mpi_handle_file_free(&nm_mpi_files, p_file);
  *fh = MPI_FILE_NULL;
  return MPI_SUCCESS;
}

int mpi_file_read_at(MPI_File fh, MPI_Offset offset, void*buf, int count, MPI_Datatype datatype, MPI_Status*status)
{
  nm_mpi_file_t*p_file = nm_mpi_handle_file_get(&nm_mpi_files, fh);
  if(p_file == NULL)
    return MPI_ERR_BAD_FILE;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  const size_t size = count * nm_mpi_datatype_size(p_datatype);
  void*ptr = malloc(size);
  ssize_t rc = pread(p_file->fd, ptr, size, offset);
  if(rc != size)
    {
      padico_fatal("MadMPI: read error rc = %lld; size = %lld\n", (long long)rc, (long long)size);
    }
  nm_mpi_datatype_unpack(ptr, buf, p_datatype, count);
  free(ptr);
  return MPI_SUCCESS;
}

int mpi_file_write_at(MPI_File fh, MPI_Offset offset, void*buf, int count, MPI_Datatype datatype, MPI_Status*status)
{
  nm_mpi_file_t*p_file = nm_mpi_handle_file_get(&nm_mpi_files, fh);
  if(p_file == NULL)
    return MPI_ERR_BAD_FILE;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  const size_t size = count * nm_mpi_datatype_size(p_datatype);
  void*ptr = malloc(size);
  nm_mpi_datatype_pack(ptr, buf, p_datatype, count);
  ssize_t rc = pwrite(p_file->fd, ptr, size, offset);
  if(rc != size)
    {
      padico_fatal("MadMPI: write error rc = %lld; size = %lld\n", (long long)rc, (long long)size);
    }
  free(ptr);
  return MPI_SUCCESS;
}

