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
$Log: leo_config.h,v $
Revision 1.1  2000/05/15 13:50:48  oaumage
- Reorganisation des sources de Leonie


______________________________________________________________________________
*/

/*
 * Leo_config.h
 * ============
 */ 
#ifndef __LEO_CONFIG_H
#define __LEO_CONFIG_H

char *
leo_find_string(p_tbx_list_t  list,
		char         *string);

p_leo_clu_host_name_t
leo_get_cluster_entry_point(p_leo_clu_cluster_t  cluster,
			    char                *cluster_id);

void
leo_cluster_setup(p_leonie_t               leonie,
		  p_leo_app_application_t  application,
		  p_leo_clu_cluster_file_t local_cluster_def);

#endif /* __LEO_CONFIG_H */
