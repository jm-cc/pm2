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
Revision 1.15  2000/12/11 08:31:17  oaumage
- support Leonie

Revision 1.14  2000/11/16 14:21:50  oaumage
- correction external spawn

Revision 1.13  2000/11/16 13:24:05  oaumage
- mise a jour initialisation

Revision 1.12  2000/11/10 14:17:55  oaumage
- nouvelle procedure d'initialisation

Revision 1.11  2000/06/16 14:03:31  oaumage
- Modification du format de la fonction mad_managers_init

Revision 1.10  2000/05/17 14:32:44  oaumage
- reorganisation des sources au niveau de mad_init

Revision 1.9  2000/03/15 16:59:13  oaumage
- support APPLICATION_SPAWN

Revision 1.8  2000/03/08 17:18:52  oaumage
- support de compilation avec Marcel sans PM2
- pre-support de packages de Threads != Marcel

Revision 1.7  2000/03/02 14:51:55  oaumage
- support de detection des protocoles au niveau runtime

Revision 1.6  2000/02/01 17:22:28  rnamyst
Replaced MAD2_MAD1 by PM2.

Revision 1.5  2000/01/31 15:52:01  oaumage
- mad_main.h  : deplacement de aligned_malloc vers la toolbox
- madeleine.h : detection de l'absence de GCC

Revision 1.4  2000/01/13 14:44:32  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant

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

typedef struct s_mad_settings
{
  char            *rsh_cmd;
  char            *configuration_file;
  tbx_bool_t       debug_mode;
  mad_driver_id_t  external_spawn_driver;
#ifdef MAD_APPLICATION_SPAWN
  char            *url;
#endif /* MAD_APPLICATION_SPAWN */
} mad_settings_t;


typedef struct s_mad_madeleine
{
  TBX_SHARED;
  mad_driver_id_t       nb_driver;
  p_mad_driver_t        driver;
  mad_adapter_id_t      nb_adapter;
  p_mad_adapter_t       adapter;
  mad_channel_id_t      nb_channel;
  tbx_list_t            channel;
  p_mad_settings_t      settings;
  p_mad_configuration_t configuration;
} mad_madeleine_t;

/*
 * Functions 
 * ---------
 */
int
mad_protocol_available(p_mad_madeleine_t  madeleine,
		       char              *name);

p_mad_adapter_set_t
mad_adapter_set_init(int nb_adapter, ...);

/*
 * Private part : these functions should not be called directly
 * --------------...............................................
 */

void 
mad_driver_fill(p_mad_madeleine_t madeleine);

void 
mad_driver_init(p_mad_madeleine_t madeleine);

void 
mad_adapter_fill(p_mad_madeleine_t     madeleine,
		 p_mad_adapter_set_t   adapter_set);

void
mad_adapter_init(p_mad_madeleine_t madeleine);

void
mad_adapter_configuration_init(p_mad_madeleine_t madeleine);

char *
mad_get_mad_root(void);

p_mad_madeleine_t
mad_object_init(int                   argc,
		char                **argv,
		char                 *configuration_file,
		p_mad_adapter_set_t   adapter_set);

void
mad_cmd_line_init(p_mad_madeleine_t   madeleine,
		  int                 argc,
		  char              **argv);

void
mad_configuration_init(p_mad_madeleine_t   madeleine,
		       int                 argc,
		       char              **argv);

void
mad_output_redirection_init(p_mad_madeleine_t   madeleine,
			    int                 argc,
			    char              **argv);

void
mad_network_components_init(p_mad_madeleine_t   madeleine,
			    int                 argc,
			    char              **argv);

void  
mad_connect(p_mad_madeleine_t   madeleine,
	    int                 argc,
	    char              **argv);

void
mad_purge_command_line(p_mad_madeleine_t   madeleine,
		       int                *_argc,
		       char              **_argv);

#endif /* MAD_MAIN_H */
