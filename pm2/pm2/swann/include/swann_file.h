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
$Log: swann_file.h,v $
Revision 1.1  2000/02/17 09:29:28  oaumage
- ajout des fichiers constitutifs de Swann


______________________________________________________________________________
*/

/*
 * swann_file.h
 * ------------
 */

#ifndef __SWANN_FILE_H
#define __SWANN_FILE_H

typedef enum
{
  swann_file_state_uninitialized = 0,
  swann_file_state_initialized,
  swann_file_state_open,
  swann_file_state_closed,
} swann_file_state_t, *p_swann_file_state_t;

typedef enum
{
  swann_file_mode_uninitialized,
  swann_file_mode_read,
  swann_file_mode_write,
  swann_file_mode_exec,
} swann_file_mode_t, *p_swann_file_mode_t;

typedef struct
{
  char               *pathname;
  int                 descriptor;
  swann_file_state_t  state;
  swann_file_mode_t   mode;
  tbx_bool_t          exist;
  tbx_bool_t          readable;
  tbx_bool_t          writeable;
  tbx_bool_t          executable;
} swann_file_t, *p_swann_file_t;


#endif /* __SWANN_FILE_H */
