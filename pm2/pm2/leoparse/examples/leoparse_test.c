
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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
 * leoparse_test.c
 * ===============
 */ 

#include <stdlib.h>
#include <stdio.h>
#include "leoparse.h"

void
usage(void)
{
  fprintf(stderr, "usage: pm2_load leoparse_test <conf_file>\n");
  exit(1);
}

void
process_emails(p_tbx_htable_t emails)
{
  char *s = NULL;

  s = leoparse_read_string(emails, "main");
  DISP_STR("first email", s);

  s = leoparse_read_string(emails, "second");
  DISP_STR("second email", s);
}

void
process_data(p_tbx_htable_t employer)
{
  char *s = NULL;

  s = leoparse_read_id(employer, "first_name");
  DISP_STR("first name", s);

  s = leoparse_read_id(employer, "last_name");
  DISP_STR("last name", s);

  s = leoparse_read_string(employer, "birthdate");
  DISP_STR("birthdate", s);

  process_emails(leoparse_read_htable(employer, "emails"));
}

void
process(p_tbx_htable_t file)
{
  p_tbx_slist_t  data = NULL;
  char          *s    = NULL;

  data = leoparse_read_slist(file, "data");

  s = leoparse_read_id(file, "name");
  DISP_STR("name", s);
  
  if (!tbx_slist_is_nil(data))
    {
      tbx_slist_ref_to_head(data);

      do
	{
	  p_leoparse_object_t object = NULL;
	    
	  object = tbx_slist_ref_get(data);
	  process_data(leoparse_get_htable(object));
	}
      while (tbx_slist_ref_forward(data));
    }
}

int
main(int    argc,
     char **argv)
{
  p_tbx_htable_t result = NULL;

  common_init(&argc, argv);
  
  tbx_init(argc, argv);
  leoparse_init(argc, argv);

  tbx_purge_cmd_line(&argc, argv);
  leoparse_purge_cmd_line(&argc, argv);

  argc--; argv++;
  if (!argc)
    usage();

  result = leoparse_parse_local_file(*argv);
  printf("Parsing succeeded\n");

  process(leoparse_read_htable(result, "file"));
    
  return 0;
}

