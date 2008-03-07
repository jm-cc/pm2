/*! \file tbx.c
 *  \brief TBX setup/cleanup routines
 *
 *  This file contains the TBX general management functions.
 *
 */

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

/*
 * Tbx.c
 * =====
 */
#include "tbx.h"

static const    char tbx_no_safe_malloc_stats[]="--tbx-no-safe-malloc-stats";
static volatile tbx_bool_t initialized = tbx_false;

int
tbx_initialized(void)
{
        return initialized;
}

void
tbx_dump(unsigned char *p,
         size_t	n) {
        int		c = 0;
        size_t	l = 0;

        while (n) {
                if (!c) {
                        DISP_NO_NL("%016zx ", 16*l);
                }

                DISP_NO_NL("%02x ", *p++);

                c++;
                if (c == 8) {
                        DISP_NO_NL(" ");
                } else if (c == 16) {
                        l++;
                        c = 0;
                        DISP_NO_NL("\n");
                        if (l >= 4) {
                                DISP_NO_NL("...\n");
                                break;
                        }
                } else if (n == 1) {
                        DISP_NO_NL("\n");
                }

                n--;
        }
}

void
tbx_init(int    argc TBX_UNUSED,
	 char **argv TBX_UNUSED)
{
  if (!initialized)
    {

      LOG_IN();

      argc--; argv++;

      while (argc)
	{
#ifdef TBX_SAFE_MALLOC
	  if (!strcmp(*argv, tbx_no_safe_malloc_stats))
	    {
	      tbx_set_print_stats_mode(tbx_false); // tbx_true by default
	    }
#endif /* TBX_SAFE_MALLOC */

	  argc--; argv++;
	}

      initialized = tbx_true;

      /* Safe malloc */
#ifdef TBX_SAFE_MALLOC
      tbx_safe_malloc_init();
#endif /* TBX_SAFE_MALLOC */

      /* Timer */
      tbx_timing_init();

      /* List manager */
      tbx_list_manager_init();

      /* Slist manager */
      tbx_slist_manager_init();

      /* Hash table manager */
      tbx_htable_manager_init();

      /* String manager */
      tbx_string_manager_init();

      /* Dynamic array manager */
      tbx_darray_manager_init();

      LOG_OUT();
    }

}

void
tbx_exit(void)
{
  LOG_IN();
  /* Dynamic array manager */
  tbx_darray_manager_exit();

  /* String manager */
  tbx_string_manager_exit();

  /* Hash table manager */
  tbx_htable_manager_exit();

  /* Slist manager */
  tbx_slist_manager_exit();

  /* List manager */
  tbx_list_manager_exit();

  /* Timer */
  tbx_timing_exit();

#ifdef TBX_SAFE_MALLOC
  tbx_safe_malloc_exit();
#endif /* TBX_SAFE_MALLOC */
  LOG_OUT();
}


void
tbx_purge_cmd_line(int   *_argc TBX_UNUSED,
		   char **_argv TBX_UNUSED)
{
  int     argc = *_argc;
  char ** argv =  _argv;
  LOG_IN();

  argv++; _argv++; argc--;

  while (argc)
    {
#ifdef TBX_SAFE_MALLOC
      if (!strcmp(*_argv, tbx_no_safe_malloc_stats))
	{
	  _argv++; (*_argc)--; argc--;
	  continue;
	}
#endif /* TBX_SAFE_MALLOC */

      *argv++ = *_argv++; argc--;
    }

  LOG_OUT();
}

