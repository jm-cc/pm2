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
$Log: mad_modes.h,v $
Revision 1.2  1999/12/15 17:31:25  oaumage
Ajout de la commande de logging de CVS

______________________________________________________________________________
*/

/*
 * Mad_modes.h
 * ===========
 */

#ifndef MAD_MODES_H
#define MAD_MODES_H

/*
 * Send/Receive modes
 * ------------------
 */
typedef enum
{
  mad_send_SAFER,
  mad_send_LATER,
  mad_send_CHEAPER
} mad_send_mode_t, *p_mad_send_mode_t;

typedef enum
{
  mad_receive_EXPRESS,
  mad_receive_CHEAPER
} mad_receive_mode_t, *p_mad_receive_mode_t;

/*
 * Link mode
 * ----------
 */
typedef enum
{
  mad_link_mode_buffer,      /* when possible, and buffer is full */
  mad_link_mode_buffer_group,       /* as much data as possible
				XXX: WARNING !!!
				This mode may require special support
				and link buffering at the low level 
				layer on the receiving side for EXPRESS
				data reception
				XXX: WARNING 2 !!!
				This mode is automatically selected
				for EXPRESS data after 'delayed_send'
				is set to True. */
  mad_link_mode_link_group   /* send all buffers in one unique chunk
			      	XXX: WARNING !!!
				This mode cannot be used for
			       	EXPRESS data (to prevent dead-locks)
			      	group mode for EXPRESS data will automatically
			      	be changed into chunk mode
			      	XXX: WARNING 2 !!!
			      	This mode is automatically selected
			      	for no-EXPRESS data after 'delayed_send'
			      	is set to True.
				XXX: WARNING 3 !!!
				This mode is automatically selected
				for no-EXPRESS/LATER data */
			 
} mad_link_mode_t, *p_mad_link_mode_t;

typedef enum
{
  mad_buffer_mode_dynamic, /* buffers must be dynamically allocated
			      when needed (send_mode != LATER)
			      -> the buffer size corresponds to the user
			      data size	*/
  mad_buffer_mode_static /* buffers are given by the low level layer
			    (connection layer) and may either be
			    system buffers, preallocated buffers or
			    buffers allocated on the fly by the connection
			    layer
			    -> the buffer size is fixed by the connection
			    layer and user data may be splitted onto one
			    one more consecutive buffer */
} mad_buffer_mode_t, *p_mad_buffer_mode_t;

#endif /* MAD_MODES_H */
