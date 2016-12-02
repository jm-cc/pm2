/*
 * NewMadeleine
 * Copyright (C) 2006-2016 (see AUTHORS file)
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


#ifndef NM_LOG_H
#define NM_LOG_H

#define NM_DISPF(str, ...)						\
  {									\
    if(!(getenv("NMAD_QUIET") || getenv("PADICO_QUIET")))		\
      fprintf(stderr, str, ## __VA_ARGS__);				\
  }

#define NM_WARN(...)							\
  {									\
    fprintf(stderr, "# nmad: WARNING- (%s)-", __TBX_FUNCTION__);	\
    fprintf(stderr, __VA_ARGS__);					\
    fprintf(stderr, "\n");						\
  }

#define NM_TRACEF(str, ...)	padico_trace(str "\n", ## __VA_ARGS__)
#define NM_TRACE_VAL(str, val)	NM_TRACEF("%s = %d", str, (int)val)
#define NM_TRACE_PTR(str, ptr)	NM_TRACEF("%s = %p", str, (void*)ptr)
#define NM_TRACE_STR(str, str2)	NM_TRACEF("%s: %s", str, str2)

#define NM_FATAL(...) {							\
    fprintf(stderr, "\n# nmad: FATAL- %s\n\t", __TBX_FUNCTION__);	\
    fprintf(stderr, __VA_ARGS__);					\
    fprintf(stderr, "\n\n");						\
    void*buffer[100];							\
    int nptrs = backtrace(buffer, 100);					\
    backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);			\
    fflush(stderr);							\
    abort();								\
  }

#endif /* NM_LOG_H */
