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
$Log: ntbx_types.h,v $
Revision 1.4  2000/05/18 11:36:12  oaumage
- Remplacement des types `tableau' par des types `structure de tableau'

Revision 1.3  2000/04/27 09:01:30  oaumage
- fusion de la branche pm2_mad2_multicluster

Revision 1.2.2.1  2000/03/30 14:14:26  oaumage
- modification du type des champs local/remote_host

Revision 1.2  2000/02/17 09:14:28  oaumage
- ajout du support de TCP a la net toolbox
- ajout de fonctions d'empaquetage de donnees numeriques

Revision 1.1  2000/02/08 15:25:24  oaumage
- ajout du sous-repertoire `net' a la toolbox


______________________________________________________________________________
*/

/*
 * ntbx_types.h
 * ============
 */

#ifndef NTBX_TYPES_H
#define NTBX_TYPES_H


/*
 * Simple types
 * ------------
 */
typedef int ntbx_host_id_t,    *p_ntbx_host_id_t;


/*
 * Pointers
 * --------
 */
typedef struct s_ntbx_host   *p_ntbx_host_t;
typedef struct s_ntbx_client *p_ntbx_client_t;
typedef struct s_ntbx_server *p_ntbx_server_t;


/*
 * Pack buffers
 * ------------
 */
#define NTBX_PACK_BUFFER_LEN     17
#define NTBX_PACK_BUFFER_TAG_LEN  4
typedef struct
{
  char buffer[NTBX_PACK_BUFFER_LEN];
} ntbx_pack_buffer_t, *p_ntbx_pack_buffer_t;


/*
 * Connection data
 * ---------------
 * (Attention aux types tableaux !!!)
 */
#define NTBX_CONNECTION_DATA_LEN 11
typedef struct
{
  char data[NTBX_CONNECTION_DATA_LEN];
} ntbx_connection_data_t, *p_ntbx_connection_data_t;


/*
 * Status
 * ------
 */
typedef enum
{
  ntbx_success =  0,
  ntbx_failure = -1,
} ntbx_status_t, *p_ntbx_status_t;


/*
 * Communication objects
 * ---------------------
 */

/*...State Constant.....................*/
typedef enum
{
  ntbx_client_state_uninitialized = 0,
  ntbx_client_state_initialized,
  ntbx_client_state_connected,
  ntbx_client_state_data_ready,
  ntbx_client_state_write_ready,
  ntbx_client_state_peer_closed,
  ntbx_client_state_shutdown,
  ntbx_client_state_closed,
} ntbx_client_state_t, *p_ntbx_client_state_t;

typedef enum
{
  ntbx_server_state_uninitialized = 0,
  ntbx_server_state_initialized,
  ntbx_server_state_accepting,
  ntbx_server_state_connection_ready,
  ntbx_server_state_shutdown,
  ntbx_server_state_closed,
} ntbx_server_state_t, *p_ntbx_server_state_t;


/*...Client/Server structures...........*/
typedef struct s_ntbx_client
{
  ntbx_client_state_t  state;
  char                *local_host;
  char                *remote_host;
  void                *specific;
} ntbx_client_t;

typedef struct s_ntbx_server
{
  ntbx_server_state_t     state;
  char                   *local_host;
  ntbx_connection_data_t  connection_data;
  void                   *specific;
} ntbx_server_t;

#endif /* NTBX_TYPES_H */
