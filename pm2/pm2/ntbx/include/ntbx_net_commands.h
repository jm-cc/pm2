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
$Log: ntbx_net_commands.h,v $
Revision 1.2  2000/04/27 09:01:29  oaumage
- fusion de la branche pm2_mad2_multicluster

Revision 1.1.2.1  2000/04/03 13:48:55  oaumage
- fichier de commande swann


______________________________________________________________________________
*/

/*
 * ntbx_net_commands.h
 * ===================
 */

#ifndef NTBX_NET_COMMANDS_H
#define NTBX_NET_COMMANDS_H

typedef enum
{
  /* synchronization */
  ntbx_command_terminate = 0,
  ntbx_command_completed,

  /* file management */
  ntbx_command_open_file,
  ntbx_command_create_file,
  ntbx_command_close_file,
  ntbx_command_destroy_file,
  ntbx_command_read_file_block,
  ntbx_command_write_file_block,

  /* remote command execution */
  ntbx_command_exec,
  ntbx_command_swann_spawn,
  ntbx_command_mad_spawn,

  /* data transfer */
  ntbx_command_send_data_block,
  ntbx_command_receive_data_block,
} ntbx_command_code_t, *p_ntbx_command_code_t;

#endif /* NTBX_NET_COMMANDS_H */
