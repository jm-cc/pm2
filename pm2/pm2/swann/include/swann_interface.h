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
$Log: swann_interface.h,v $
Revision 1.2  2000/03/27 12:53:56  oaumage
- progression des fonctionnalites:
  * support reseau
  * support execution
  * extension du support fichier

Revision 1.1  2000/02/17 09:29:27  oaumage
- ajout des fichiers constitutifs de Swann


______________________________________________________________________________
*/

/*
 * swann_interface.h
 * -----------------
 */

#ifndef __SWANN_INTERFACE_H
#define __SWANN_INTERFACE_H


/*
 * File management
 * ---------------.........................................................
 */
swann_status_t
swann_file_init(p_swann_file_t  file,
		char           *pathname);

swann_status_t
swann_file_open(p_swann_file_t file);

swann_status_t
swann_file_create(p_swann_file_t file);

swann_status_t
swann_file_close(p_swann_file_t file);

swann_status_t
swann_file_destroy(p_swann_file_t file);

size_t
swann_file_read_block(p_swann_file_t  file,
		      void           *ptr,
		      size_t          length);

size_t
swann_file_write_block(p_swann_file_t  file,
		      void            *ptr,
		      size_t           length);

/*
 * Process management
 * ------------------......................................................
 */
swann_status_t
swann_run_command(p_swann_file_t  file,
		  char           *argv[],
		  char           *envp[],
		  int            *return_code);

p_swann_command_t
swann_run_async_command(p_swann_file_t  file,
			char           *argv[],
			char           *envp[],
			p_swann_file_t  file_in,
			p_swann_file_t  file_out,
			p_swann_file_t  file_err);


/*
 * Network management
 * ------------------
 */
p_swann_net_server_t
swann_net_server_init(swann_net_server_id_t     id,
		      char                     *master_host_name,
		      p_ntbx_connection_data_t  connection_data);

#endif /* __SWANN_INTERFACE_H */
