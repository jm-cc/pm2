/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/*
 * mad_micro.c
 * ===========
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "pm2_common.h"

int
main(int argc, char **argv) {
        p_mad_madeleine_t          madeleine      = NULL;
        p_mad_channel_t            channel        = NULL;
        p_ntbx_process_container_t pc             = NULL;
        ntbx_process_grank_t       my_global_rank =   -1;
        ntbx_process_lrank_t       my_local_rank  =   -1;

        /*
         * Initialisation des diverses bibliothèques.
         */
        common_pre_init (&argc, argv, NULL);
        common_post_init(&argc, argv, NULL);

        /*
         * Récupération de l'objet Madeleine.
         */
        madeleine      = mad_get_madeleine();

        /*
         * Récupération du numéro global du processus (unique dans la session).
         */
        my_global_rank = madeleine->session->process_rank;

        /*
         * Récupération de l'objet 'channel' correspondant au canal "test"
         * défini dans le fichier de configuration.
         */
        channel = tbx_htable_get(madeleine->channel_htable, "test");

        /*
         * Conversion du numéro global du processus en numéro local au
         * niveau du canal.
         *
         * Opération en deux temps:
         *
         * 1) on récupère le "process container" du canal qui est un tableau
         * à double entrées rang local/rang global
         *
         * 2) on convertit le numéro global du processus en un numéro local
         * dans le contexte du canal. Le contexte est fourni par le process
         * container.
         */
        pc = channel->pc;
        my_local_rank = ntbx_pc_global_to_local(pc, my_global_rank);

        /*
         * On ne s'occupe que des processus de numéro local 0 ou 1 du canal.
         * Les autres n'interviennent pas dans ce programme.
         */
        if (my_local_rank == 0) {
                /*
                 * Le processus de numéro local '0' va jouer le rôle de l'émetteur.
                 */

                /*
                 * L'objet "connection" pour émettre.
                 */
                p_mad_connection_t  out    = NULL;

                int                 len    =    0;
                char               *chaine = "Hello, world";

                /*
                 * On demande l'émission d'un message sur le canal
                 * "channel", vers le processus de numéro local "1".
                 */
                out  = mad_begin_packing(channel, 1);

                len = strlen(chaine)+1;

                /*
                 * On envoie la longueur de la chaîne en Express.
                 */
                mad_pack(out, &len, sizeof(len), mad_send_CHEAPER, mad_receive_EXPRESS);

                /*
                 * On envoie la chaîne proprement dite, en Cheaper.
                 * On autorise le bloc a être perdu.
                 */
                {
                        /*
                         * L'objet "paramètre" qu'on va associer au pack.
                         * Note: il _faut_ allouer _un_ objet param par bloc.
                         * Note 2: la libération est implicitement réalisée par
                         *         mad_pack_ext.
                         */
                        p_mad_buffer_slice_parameter_t param = NULL;

                        /*
                         * Une commande spéciale est utilisée pour
                         * l'allocation de l'objet paramètre.
                         */
                        param = mad_alloc_slice_parameter();

                        /*
                         * Opcode du parametre de pack.
                         * Ici, il s'agit de l'opcode indiquant que le transfert
                         * du bloc est optionnel.
                         */
                        param->opcode = mad_op_optional_block;

                        /*
                         * Probabilité maximale de perte autorisée, en %.
                         */
                        param->value  = 100;

                        /*
                         * Offset de la section concernée du bloc.
                         * <ignoré pour l'instant>
                         */
                        param->offset = 0;

                        /*
                         * Longueur de la section concernée du bloc.
                         * 0 est équivalent à "jusqu'à la fin du bloc"
                         * <ignoré pour l'instant>
                         */
                        param->length = 0;

                        /*
                         * Fonction pack étendue
                         *
                         * Les deux drapeaux
                         * mad_send_XXX/mad_receive_XXX peuvent être
                         * suivis d'un ou plusieurs objets
                         * "paramètre". Le dernier argument de
                         * mad_pack_ext doit toujours être NULL.
                         */
                        mad_pack_ext(out,
                                     chaine, len,
                                     mad_send_CHEAPER, mad_receive_CHEAPER,
                                     param, NULL);
                }

                /*
                 * On indique que le message est entièrement construit
                 * et que toutes les données non encore envoyées
                 * doivent partir.
                 */
                mad_end_packing(out);
        } else if (my_local_rank == 1) {
                /*
                 * Le processus de numéro local '1' va jouer le rôle du récepteur.
                 */

                /*
                 * L'objet "connection" pour recevoir.
                 */
                p_mad_connection_t  in           = NULL;

                int                 len          =    0;
                char               *msg          = NULL;
                p_tbx_slist_t       status_slist = NULL;

                /*
                 * On demande la réception du premier "message"
                 * disponible. Note: il n'est pas possible de
                 * spécifier "de qui" on veut recevoir un message.
                 */
                in = mad_begin_unpacking(channel);

                /*
                 * On reçoit la longueur de la chaîne en Express car
                 * on a immédiatement besoin de cette information pour
                 * allouer la mémoire nécessaire à la réception de la
                 * chaîne proprement dite.
                 */
                mad_unpack(in, &len, sizeof(len), mad_send_CHEAPER, mad_receive_EXPRESS);

                /*
                 * On alloue de la mémoire pour recevoir la chaîne.
                 */
                msg = calloc(1, len);

                /*
                 * On reçoit la chaîne de caractères.
                 */
                mad_unpack(in, msg, len, mad_send_CHEAPER, mad_receive_CHEAPER);

                /*
                 * On indique la fin de la réception. C'est seulement
                 * après l'appel à end_unpacking que la disponibilité
                 * des données marquées "receive_CHEAPER" sont
                 * garanties.
                 * On utilise la version étendue de end_unpacking
                 * pour récupérer la liste des informations de status 
                 * des blocs receive_CHEAPER du message.
                 * Note: pour les blocs receive_EXPRESS, il faut utiliser
                 * mad_unpack_ext qui retourne également une liste.
                 */
                status_slist = mad_end_unpacking_ext(in);

                /*
                 * On vérifie que la liste existe...
                 */
                if (status_slist) {

                        /*
                         * ... et qu'elle n'est pas vide.
                         */
                        if (!tbx_slist_is_nil(status_slist)) {

                                /*
                                 * On (re)place le curseur de lecture
                                 * de liste au début de la liste.
                                 */
                                tbx_slist_ref_to_head(status_slist);
                                do {
                                        /*
                                         * Objet status (identique a l'objet paramètre).
                                         */
                                        p_mad_buffer_slice_parameter_t status = NULL;

                                        /*
                                         * On récupère l'objet au niveau du curseur de la liste.
                                         */
                                        status = tbx_slist_ref_get(status_slist);

                                        /*
                                         * On teste l'opcode du status.
                                         */
                                        if (status->opcode == mad_os_lost_block) {

                                                /*
                                                 * L'opcode indique qu'une section de bloc a été perdue.
                                                 *
                                                 * Les champs base, offset et length localisent cette section.
                                                 */
                                                DISP("lost block slice: base = %p, offset = %u, length = %u",
                                                     status->base, status->offset, status->length);
                                        } else
                                                FAILURE("invalid or unknown status opcode");

                                /*
                                 * On avance le curseur d'un pas.
                                 */
                                } while (tbx_slist_ref_forward(status_slist));
                        }

                        /* On libère la liste. */
                        mad_free_parameter_slist(status_slist);
                }


                /*
                 * On affiche le message pour montrer qu'on l'a bien reçu.
                 */
                DISP("msg = [%s]", msg);
        }

        /*
         * On termine l'exécution des bibliothèques.
         */
        common_exit(NULL);

        return 0;
}
