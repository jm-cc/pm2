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
 suppong documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.

______________________________________________________________________________
$Log: leoparse_main.h,v $
Revision 1.1  2000/11/02 14:25:03  oaumage
Leoparse

______________________________________________________________________________
*/

/*
 * Leoparse_parsing.h
 * =============
 */ 

#ifndef __LEOPARSE_PARSING_H
#define __LEOPARSE_PARSING_H

int
leoparse_yy_input(char *buffer,
	     int   max_size);

void
leoparse_open_local_parser_file(char *file_name);

void
leoparse_close_local_parser_file(void);

#ifdef LEOPARSE_REMOTE
void
leoparse_open_remote_parser_file(p_leoparse_swann_module_t  module,
			    char                 *file_name);

void
leoparse_close_remote_parser_file(void);
#endif /* LEOPARSE_REMOTE */

void
leoparse_init(void);

#endif /* __LEOPARSE_PARSING_H */
