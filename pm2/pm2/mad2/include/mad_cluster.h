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
$Log: mad_cluster.h,v $
Revision 1.2  2000/03/08 17:18:50  oaumage
- support de compilation avec Marcel sans PM2
- pre-support de packages de Threads != Marcel

Revision 1.1  2000/03/07 10:38:54  oaumage
- pre-support multi-cluster

Revision 1.4  2000/03/02 14:51:55  oaumage
- support de detection des protocoles au niveau runtime

Revision 1.3  2000/01/13 14:44:31  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant

Revision 1.2  1999/12/15 17:31:21  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

#ifndef MAD_CLUSTER_H
#define MAD_CLUSTER_H

typedef enum
{
  mad_channel_default = 0,
} mad_channel_description_id_t, *p_mad_channel_description_id_t;

typedef struct s_mad_channel_description
{
  mad_channel_description_id_t    id;
  /* mad_channel_description_alias_t alias; */
  mad_driver_id_t                 driver_id;
  mad_adapter_id_t                adapter_id;
} mad_channel_description_t;

typedef struct s_mad_cluster
{
  TBX_SHARED;
  mad_cluster_id_t            id;
  mad_configuration_t         configuration;
  p_mad_channel_description_t channel_description_array;
} mad_cluster_t;

#endif /* MAD_CLUSTER_H */
