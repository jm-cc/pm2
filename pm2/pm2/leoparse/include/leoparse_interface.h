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
$Log: leoparse_interface.h,v $
Revision 1.4  2000/12/11 08:31:16  oaumage
- support Leonie

Revision 1.3  2000/11/16 13:24:04  oaumage
- mise a jour initialisation

Revision 1.2  2000/11/10 14:17:53  oaumage
- nouvelle procedure d'initialisation

Revision 1.1  2000/11/02 14:25:03  oaumage
Leoparse


______________________________________________________________________________
*/
/*
 * leoparse_interface.h
 * ---------------
 */

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
$Log: leoparse_interface.h,v $
Revision 1.4  2000/12/11 08:31:16  oaumage
- support Leonie

Revision 1.3  2000/11/16 13:24:04  oaumage
- mise a jour initialisation


______________________________________________________________________________
*/

/*
 * Leoparse_interface.h
 * ====================
 */ 

#ifndef __LEOPARSE_INTERFACE_H
#define __LEOPARSE_INTERFACE_H

/* Parsing */
void
leoparse_open_local_parser_file(char *file_name);

p_tbx_htable_t
leoparse_parse_local_file(char* filename);

void
leoparse_close_local_parser_file(void);

#ifdef LEOPARSE_REMOTE
void
leoparse_open_remote_parser_file(p_leoparse_swann_module_t  module,
			    char                 *file_name);

void
leoparse_close_remote_parser_file(void);
#endif /* LEOPARSE_REMOTE */


/* Initialization */
void
leoparse_init(int    argc,
	      char **argv);

void
leoparse_purge_cmd_line(int   *argc,
			char **argv);

/* Yacc interface */
int
leoparse_yy_input(char *buffer,
		  int   max_size);



#endif /* __LEOPARSE_INTERFACE_H */
