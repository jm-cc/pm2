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
$Log: swann_run.h,v $
Revision 1.2  2000/03/27 12:53:58  oaumage
- progression des fonctionnalites:
  * support reseau
  * support execution
  * extension du support fichier

Revision 1.1  2000/02/17 09:29:24  oaumage
- ajout des fichiers constitutifs de Swann


______________________________________________________________________________
*/

/*
 * swann_run.h
 * -----------
 */

#ifndef __SWANN_RUN_H
#define __SWANN_RUN_H

typedef enum
{
  swann_command_state_running = 0,
  swann_command_state_finished
} swann_command_state_t, *p_swann_command_state_t;

typedef struct s_swann_command
{
  swann_command_state_t  command_state;
  p_swann_file_t         file;
  char                 **argv;
  char                 **envp;
  pid_t                  pid;
  int                    return_value;
  p_swann_file_t         file_in;
  p_swann_file_t         file_out;
  p_swann_file_t         file_err;
} swann_command_t;

#endif /* __SWANN_RUN_H */
