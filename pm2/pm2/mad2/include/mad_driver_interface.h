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
$Log: mad_driver_interface.h,v $
Revision 1.6  2000/03/02 15:45:27  oaumage
- support du polling Nexus

Revision 1.5  2000/02/08 17:47:24  oaumage
- prise en compte des types de la net toolbox

Revision 1.4  2000/01/13 14:44:31  oaumage
- adaptation pour la prise en compte de la toolbox
- suppression des fichiers redondant

Revision 1.3  1999/12/15 17:31:21  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_driver_interface.h
 * ======================
 */

#ifndef MAD_DRIVER_INTERFACE_H
#define MAD_DRIVER_INTERFACE_H

typedef struct s_mad_driver_interface
{
  /* Driver initialization */
  void
  (*driver_init)(p_mad_driver_t);
  

  /* Adapter initialization */
  void
  (*adapter_init)(p_mad_adapter_t);

  void
  (*adapter_configuration_init)(p_mad_adapter_t); 


  /* Channel/Connection/Link initialization */
  void
  (*channel_init)(p_mad_channel_t);

  void
  (*before_open_channel)(p_mad_channel_t);

  void
  (*connection_init)(p_mad_connection_t,
		     p_mad_connection_t);
  
  void
  (*link_init)(p_mad_link_t);
  
  /* Point-to-point connection */
  void
  (*accept)(p_mad_channel_t);
  
  void
  (*connect)(p_mad_connection_t);

  /* Channel clean-up functions */
  void
  (*after_open_channel)(p_mad_channel_t);

  void 
  (*before_close_channel)(p_mad_channel_t);

  /* Connection clean-up function */
  void
  (*disconnect)(p_mad_connection_t);
  
  void 
  (*after_close_channel)(p_mad_channel_t);

  /* Deallocation functions */
  void
  (*link_exit)(p_mad_link_t);
  
  void
  (*connection_exit)(p_mad_connection_t,
		     p_mad_connection_t);
  
  void
  (*channel_exit)(p_mad_channel_t);

  void
  (*adapter_exit)(p_mad_adapter_t);
  
  void
  (*driver_exit)(p_mad_driver_t);

  /* Dynamic paradigm selection */
  p_mad_link_t
  (*choice)(p_mad_connection_t,
	    size_t,
            mad_send_mode_t,
	    mad_receive_mode_t);

  /* Static buffers management */
  p_mad_buffer_t
  (*get_static_buffer)(p_mad_link_t);

  void
  (*return_static_buffer)(p_mad_link_t, p_mad_buffer_t);

  /* Message transfer */
  void
  (*new_message)(p_mad_connection_t);

  p_mad_connection_t
  (*receive_message)(p_mad_channel_t);

#ifdef MAD_NEXUS
  p_mad_connection_t
  (*poll_message)(p_mad_channel_t);
#endif /* MAD_NEXUS */
  
  /* Buffer transfer */
  void
  (*send_buffer)(p_mad_link_t,
		 p_mad_buffer_t);
  
  void
  (*receive_buffer)(p_mad_link_t,
		    p_mad_buffer_t *);
  
  /* Buffer group transfer */
  void
  (*send_buffer_group)(p_mad_link_t,
		       p_mad_buffer_group_t);
  
  void
  (*receive_sub_buffer_group)(p_mad_link_t,
			      tbx_bool_t,
			      p_mad_buffer_group_t);

  /* External spawn support */
  void
  (*external_spawn_init)(p_mad_adapter_t, int *, char **);

  void
  (*configuration_init)(p_mad_adapter_t, p_mad_configuration_t);

  void
  (*send_adapter_parameter)(p_mad_adapter_t, ntbx_host_id_t, char *);

  void
  (*receive_adapter_parameter)(p_mad_adapter_t, char **);
  
} mad_driver_interface_t;

#endif /* MAD_DRIVER_INTERFACE_H */
