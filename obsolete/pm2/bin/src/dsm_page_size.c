#include "stdio.h"
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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


unsigned long _dsm_get_page_size()
{
#if defined(SOLARIS_SYS) || defined(IRIX_SYS)
  return sysconf(_SC_PAGESIZE);
#elif defined(UNICOS_SYS)
  return 1024;
#else
  return getpagesize();
#endif
}

int main()
{
 fprintf(stdout, "%d", _dsm_get_page_size());
}
