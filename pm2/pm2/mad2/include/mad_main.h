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
$Log: mad_main.h,v $
Revision 1.3  1999/12/15 17:31:23  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_main.h
 * ===========
 */

#ifndef MAD_MAIN_H
#define MAD_MAIN_H

/*
 * Structures
 * ----------
 */

/*
 * Connection registration array
 * -----------------------------
 */

typedef struct s_mad_madeleine
{
  PM2_SHARED;
  mad_driver_id_t             nb_driver;
  p_mad_driver_t              driver;
  mad_adapter_id_t            nb_adapter;
  p_mad_adapter_t             adapter;
  mad_configuration_t         configuration;
  mad_channel_id_t            nb_channel;
  mad_list_t                  channel;
} mad_madeleine_t;

/*
 * Functions 
 * ---------
 */
void *mad_aligned_malloc(size_t   size,
			 int      align);

void mad_aligned_free (void  *ptr,
		       int    align);

p_mad_adapter_set_t mad_adapter_set_init(int nb_adapter, ...);

#ifdef MAD2_MAD1
p_mad_madeleine_t mad2_init(int                  *argc,
			    char                **argv,
			    char                 *configuration_file,
			    p_mad_adapter_set_t   adapter_set);
#else /* MAD2_MAD1 */
p_mad_madeleine_t mad_init(int                  *argc,
			   char                **argv,
			   char                 *configuration_file,
			   p_mad_adapter_set_t   adapter_set);
#endif /* MAD2_MAD1 */

#ifdef MAD2_MAD1
void mad2_exit(p_mad_madeleine_t madeleine);
#else /* MAD2_MAD1 */
void mad_exit(p_mad_madeleine_t madeleine);
#endif /* MAD2_MAD1 */

p_mad_channel_t mad_get_channel(mad_channel_id_t id);

#endif /* MAD_MAIN_H */
