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
$Log: leo_net.h,v $
Revision 1.1  2000/05/15 13:50:49  oaumage
- Reorganisation des sources de Leonie


______________________________________________________________________________
*/

/*
 * Leo_net.h
 * =========
 */ 

#ifndef __LEO_NET_H
#define __LEO_NET_H

void
leo_send_block(p_leo_swann_module_t  module,
	       void                 *ptr,
	       size_t                length);

void
leo_receive_block(p_leo_swann_module_t  module,
		  void                 *ptr,
		  size_t                length);

void
leo_send_string(p_leo_swann_module_t  module,
		char                 *data);

char *
leo_receive_string(p_leo_swann_module_t module);

void
leo_send_raw_int(p_leo_swann_module_t module,
		 int                  data);

int
leo_receive_raw_int(p_leo_swann_module_t module);

void
leo_send_raw_long(p_leo_swann_module_t module,
		  long                 data);

long
leo_receive_raw_long(p_leo_swann_module_t module);

void
leo_send_int(p_leo_swann_module_t module,
	     int                  data);

int
leo_receive_int(p_leo_swann_module_t module);

void
leo_send_long(p_leo_swann_module_t module,
	      long                 data);

long
leo_receive_long(p_leo_swann_module_t module);

void
leo_send_command(p_leo_swann_module_t module,
		 ntbx_command_code_t  code);

p_ntbx_server_t
leo_net_server_init(void);

#endif /* __LEO_NET_H */
