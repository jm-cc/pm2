/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: leoparse_test.c,v $
Revision 1.3  2000/12/19 16:57:46  oaumage
- finalisation de leoparse
- exemples pour leoparse
- modification des macros de logging
- version typesafe de certaines macros
- finalisation des tables de hachage
- finalisation des listes de recherche

Revision 1.2  2000/11/16 13:24:03  oaumage
- mise a jour initialisation


______________________________________________________________________________
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

