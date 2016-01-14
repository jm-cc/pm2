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
PADICO_MODULE_HOOK(MadMPI);

/** Internal MPI file */
typedef struct nm_mpi_file_s
{
  /** identifier of the file */
  MPI_File id;
  /** access mode */
  int amode;
  /** communicator attached to file */
  struct nm_mpi_communicator_s*p_comm;
  /** file view */
  struct
  {
    MPI_Offset disp;
    struct nm_mpi_datatype_s*p_etype;
    struct nm_mpi_datatype_s*p_filetype;
  } view;
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

NM_MPI_ALIAS(MPI_File_open,      mpi_file_open);
NM_MPI_ALIAS(MPI_File_get_size,  mpi_file_get_size);
NM_MPI_ALIAS(MPI_File_get_amode, mpi_file_get_amode);
NM_MPI_ALIAS(MPI_File_get_group, mpi_file_get_group);
NM_MPI_ALIAS(MPI_File_get_type_extent, mpi_file_get_type_extent);
NM_MPI_ALIAS(MPI_File_set_view,  mpi_file_set_view);
NM_MPI_ALIAS(MPI_File_close,     mpi_file_close);
NM_MPI_ALIAS(MPI_File_delete,    mpi_file_delete);
NM_MPI_ALIAS(MPI_File_read,      mpi_file_read);
NM_MPI_ALIAS(MPI_File_write,     mpi_file_write);
NM_MPI_ALIAS(MPI_File_read_at,   mpi_file_read_at);
NM_MPI_ALIAS(MPI_File_write_at,  mpi_file_write_at);

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

  fprintf(stderr, "# madmpi: MPI_File_open = %s\n", filename);
  
  nm_mpi_file_t*p_file = nm_mpi_handle_file_alloc(&nm_mpi_files);
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    return MPI_ERR_COMM;
  if(nm_comm_rank(p_comm->p_nm_comm) < 0)
    return MPI_SUCCESS;
  p_file->amode = amode;
  p_file->p_comm = p_comm;
  p_file->kind = NM_MPI_FILE_POSIX;
  p_file->view.disp = 0;
  p_file->view.p_etype = NULL;
  p_file->view.p_filetype = NULL;
  const int mode =
    ((amode & MPI_MODE_RDONLY) ? O_RDONLY : 0) |
    ((amode & MPI_MODE_WRONLY) ? O_WRONLY : 0) |
    ((amode & MPI_MODE_RDWR)   ? O_RDWR   : 0) |
    ((amode & MPI_MODE_CREATE) ? O_CREAT  : 0) |
    ((amode & MPI_MODE_EXCL)   ? O_EXCL   : 0) |
    ((amode & MPI_MODE_APPEND) ? O_APPEND : 0) ;
  int rc = open(filename, mode, 0666);
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

int mpi_file_delete(char*filename, MPI_Info info)
{
  int rc = unlink(filename);
  if(rc != 0)
    return MPI_ERR_NO_SUCH_FILE;
  return MPI_SUCCESS;
}


int mpi_file_get_size(MPI_File fh, MPI_Offset*size)
{
  nm_mpi_file_t*p_file = nm_mpi_handle_file_get(&nm_mpi_files, fh);
  if(p_file == NULL)
    return MPI_ERR_BAD_FILE;
  struct stat stat;
  int rc = fstat(p_file->fd, &stat);
  if(rc != 0)
    return MPI_ERR_IO;
  *size = stat.st_size;
  return MPI_SUCCESS;
}

int mpi_file_get_amode(MPI_File fh, int*amode)
{
  nm_mpi_file_t*p_file = nm_mpi_handle_file_get(&nm_mpi_files, fh);
  if(p_file == NULL)
    return MPI_ERR_BAD_FILE;
  *amode = p_file->amode;
  return MPI_SUCCESS;
}

int mpi_file_get_group(MPI_File fh, MPI_Group*group)
{
  nm_mpi_file_t*p_file = nm_mpi_handle_file_get(&nm_mpi_files, fh);
  if(p_file == NULL)
    return MPI_ERR_BAD_FILE;
  return mpi_comm_group(p_file->p_comm->id, group);
}

int mpi_file_get_type_extent(MPI_File fh, MPI_Datatype datatype, MPI_Aint*extent)
{
  nm_mpi_file_t*p_file = nm_mpi_handle_file_get(&nm_mpi_files, fh);
  if(p_file == NULL)
    return MPI_ERR_BAD_FILE;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  if(extent != NULL)
    *extent = p_datatype->extent;
  return MPI_SUCCESS;
}

int mpi_file_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char*datarep, MPI_Info info)
{
  if(strcmp(datarep, "native") != 0)
    return MPI_ERR_UNSUPPORTED_DATAREP;
  nm_mpi_file_t*p_file = nm_mpi_handle_file_get(&nm_mpi_files, fh);
  if(p_file == NULL)
    return MPI_ERR_BAD_FILE;
  nm_mpi_datatype_t*p_etype = nm_mpi_datatype_get(etype);
  if(p_etype == NULL)
    return MPI_ERR_TYPE;
  nm_mpi_datatype_t*p_filetype = nm_mpi_datatype_get(filetype);
  if(p_filetype == NULL)
    return MPI_ERR_TYPE;
  p_file->view.disp = disp;
  p_file->view.p_etype = p_etype;
  p_file->view.p_filetype = p_filetype;
  lseek(p_file->fd, disp, SEEK_SET);
  return MPI_SUCCESS;
}


int mpi_file_read(MPI_File fh, void*buf, int count, MPI_Datatype datatype, MPI_Status*status)
{
  nm_mpi_file_t*p_file = nm_mpi_handle_file_get(&nm_mpi_files, fh);
  if(p_file == NULL)
    return MPI_ERR_BAD_FILE;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  const size_t size = count * nm_mpi_datatype_size(p_datatype);
  void*ptr = malloc(size);
  ssize_t rc = read(p_file->fd, ptr, size);
  if(rc != size)
    {
      padico_fatal("MadMPI: read error rc = %lld; size = %lld\n", (long long)rc, (long long)size);
    }
  nm_mpi_datatype_unpack(ptr, buf, p_datatype, count);
  free(ptr);
  if(status)
  {
    status->MPI_ERROR = MPI_SUCCESS;
    status->size = size;
  }
  return MPI_SUCCESS;
}

int mpi_file_write(MPI_File fh, void*buf, int count, MPI_Datatype datatype, MPI_Status*status)
{
  nm_mpi_file_t*p_file = nm_mpi_handle_file_get(&nm_mpi_files, fh);
  if(p_file == NULL)
    return MPI_ERR_BAD_FILE;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  const size_t size = count * nm_mpi_datatype_size(p_datatype);
  void*ptr = malloc(size);
  nm_mpi_datatype_pack(ptr, buf, p_datatype, count);
  ssize_t rc = write(p_file->fd, ptr, size);
  if(rc != size)
    {
      padico_fatal("MadMPI: write error rc = %lld; size = %lld\n", (long long)rc, (long long)size);
    }
  free(ptr);
  if(status)
  {
    status->MPI_ERROR = MPI_SUCCESS;
    status->size = size;
  }
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
  if(status)
  {
    status->MPI_ERROR = MPI_SUCCESS;
    status->size = size;
  }
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
  if(status)
  {
    status->MPI_ERROR = MPI_SUCCESS;
    status->size = size;
  }
  return MPI_SUCCESS;
}

