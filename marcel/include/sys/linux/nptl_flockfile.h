/* lockfile - Handle locking and unlocking of stream.
   Copyright (C) 1996, 1998, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


#include <sys/types.h>
#include "marcel.h"


#ifdef MA__LIBPTHREAD


typedef struct {
	int _flags;		/* High-order word is _IO_MAGIC; rest is flags. */
#define _IO_file_flags _flags

	/* The following pointers correspond to the C++ streambuf protocol. */
	/* Note:  Tk uses the _IO_read_ptr and _IO_read_end fields directly. */
	char *_IO_read_ptr;	/* Current read pointer */
	char *_IO_read_end;	/* End of get area. */
	char *_IO_read_base;	/* Start of putback+get area. */
	char *_IO_write_base;	/* Start of put area. */
	char *_IO_write_ptr;	/* Current put pointer. */
	char *_IO_write_end;	/* End of put area. */
	char *_IO_buf_base;	/* Start of reserve area. */
	char *_IO_buf_end;	/* End of reserve area. */
	/* The following fields are used to support backing up and undo. */
	char *_IO_save_base;	/* Pointer to start of non-current get area. */
	char *_IO_backup_base;	/* Pointer to first valid character of backup area */
	char *_IO_save_end;	/* Pointer to end of non-current get area. */

	struct _IO_marker *_markers;

	struct _LPT_IO_FILE *_chain;

	int _fileno;
#if 0
	int _blksize;
#else
	int _flags2;
#endif
	/** _IO_off_t <=> __off_t **/
	__off_t _old_offset;	/* This used to be _offset but it's too small.  */

#define __HAVE_COLUMN		/* temporary */
	/* 1+column number of pbase(); 0 is unknown. */
	unsigned short _cur_column;
	signed char _vtable_offset;
	char _shortbuf[1];

	/*  char* _save_gptr;  char* _save_egptr; */

	struct _lpt_fastlock _lock;
} lpt_io_file;


/* __f*lockfile: see nptl_flockfile.c */
void __flockfile(_IO_FILE *stream);
void __funlockfile(_IO_FILE *stream);
int __ftrylockfile(_IO_FILE *stream);


#endif /** MA__LIBPTHREAD **/
