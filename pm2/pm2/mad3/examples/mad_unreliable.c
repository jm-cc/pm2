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
         * Initialisation des diverses biblioth�ques.
         */
        common_pre_init (&argc, argv, NULL);
        common_post_init(&argc, argv, NULL);

        /*
         * R�cup�ration de l'objet Madeleine.
         */
        madeleine      = mad_get_madeleine();

        /*
         * R�cup�ration du num�ro global du processus (unique dans la session).
         */
        my_global_rank = madeleine->session->process_rank;

        /*
         * R�cup�ration de l'objet 'channel' correspondant au canal "test"
         * d�fini dans le fichier de configuration.
         */
        channel = tbx_htable_get(madeleine->channel_htable, "test");

        /*
         * Conversion du num�ro global du processus en num�ro local au
         * niveau du canal.
         *
         * Op�ration en deux temps:
         *
         * 1) on r�cup�re le "process container" du canal qui est un tableau
         * � double entr�es rang local/rang global
         *
         * 2) on convertit le num�ro global du processus en un num�ro local
         * dans le contexte du canal. Le contexte est fourni par le process
         * container.
         */
        pc = channel->pc;
        my_local_rank = ntbx_pc_global_to_local(pc, my_global_rank);

        /*
         * On ne s'occupe que des processus de num�ro local 0 ou 1 du canal.
         * Les autres n'interviennent pas dans ce programme.
         */
        if (my_local_rank == 0) {
                /*
                 * Le processus de num�ro local '0' va jouer le r�le de l'�metteur.
                 */

                /*
                 * L'objet "connection" pour �mettre.
                 */
                p_mad_connection_t  out    = NULL;

                int                 len    =    0;
                char               *chaine = "Hello, world";

                /*
                 * On demande l'�mission d'un message sur le canal
                 * "channel", vers le processus de num�ro local "1".
                 */
                out  = mad_begin_packing(channel, 1);

                len = strlen(chaine)+1;

                /*
                 * On envoie la longueur de la cha�ne en Express.
                 */
                mad_pack(out, &len, sizeof(len), mad_send_CHEAPER, mad_receive_EXPRESS);

                /*
                 * On envoie la cha�ne proprement dite, en Cheaper.
                 * On autorise le bloc a �tre perdu.
                 */
                {
                        /*
                         * L'objet "param�tre" qu'on va associer au pack.
                         * Note: il _faut_ allouer _un_ objet param par bloc.
                         * Note 2: la lib�ration est implicitement r�alis�e par
                         *         mad_pack_ext.
                         */
                        p_mad_buffer_slice_parameter_t param = NULL;

                        /*
                         * Une commande sp�ciale est utilis�e pour
                         * l'allocation de l'objet param�tre.
                         */
                        param = mad_alloc_slice_parameter();

                        /*
                         * Opcode du parametre de pack.
                         * Ici, il s'agit de l'opcode indiquant que le transfert
                         * du bloc est optionnel.
                         */
                        param->opcode = mad_op_optional_block;

                        /*
                         * Probabilit� maximale de perte autoris�e, en %.
                         */
                        param->value  = 100;

                        /*
                         * Offset de la section concern�e du bloc.
                         * <ignor� pour l'instant>
                         */
                        param->offset = 0;

                        /*
                         * Longueur de la section concern�e du bloc.
                         * 0 est �quivalent � "jusqu'� la fin du bloc"
                         * <ignor� pour l'instant>
                         */
                        param->length = 0;

                        /*
                         * Fonction pack �tendue
                         *
                         * Les deux drapeaux
                         * mad_send_XXX/mad_receive_XXX peuvent �tre
                         * suivis d'un ou plusieurs objets
                         * "param�tre". Le dernier argument de
                         * mad_pack_ext doit toujours �tre NULL.
                         */
                        mad_pack_ext(out,
                                     chaine, len,
                                     mad_send_CHEAPER, mad_receive_CHEAPER,
                                     param, NULL);
                }

                /*
                 * On indique que le message est enti�rement construit
                 * et que toutes les donn�es non encore envoy�es
                 * doivent partir.
                 */
                mad_end_packing(out);
        } else if (my_local_rank == 1) {
                /*
                 * Le processus de num�ro local '1' va jouer le r�le du r�cepteur.
                 */

                /*
                 * L'objet "connection" pour recevoir.
                 */
                p_mad_connection_t  in           = NULL;

                int                 len          =    0;
                char               *msg          = NULL;
                p_tbx_slist_t       status_slist = NULL;

                /*
                 * On demande la r�ception du premier "message"
                 * disponible. Note: il n'est pas possible de
                 * sp�cifier "de qui" on veut recevoir un message.
                 */
                in = mad_begin_unpacking(channel);

                /*
                 * On re�oit la longueur de la cha�ne en Express car
                 * on a imm�diatement besoin de cette information pour
                 * allouer la m�moire n�cessaire � la r�ception de la
                 * cha�ne proprement dite.
                 */
                mad_unpack(in, &len, sizeof(len), mad_send_CHEAPER, mad_receive_EXPRESS);

                /*
                 * On alloue de la m�moire pour recevoir la cha�ne.
                 */
                msg = calloc(1, len);

                /*
                 * On re�oit la cha�ne de caract�res.
                 */
                mad_unpack(in, msg, len, mad_send_CHEAPER, mad_receive_CHEAPER);

                /*
                 * On indique la fin de la r�ception. C'est seulement
                 * apr�s l'appel � end_unpacking que la disponibilit�
                 * des donn�es marqu�es "receive_CHEAPER" sont
                 * garanties.
                 * On utilise la version �tendue de end_unpacking
                 * pour r�cup�rer la liste des informations de status 
                 * des blocs receive_CHEAPER du message.
                 * Note: pour les blocs receive_EXPRESS, il faut utiliser
                 * mad_unpack_ext qui retourne �galement une liste.
                 */
                status_slist = mad_end_unpacking_ext(in);

                /*
                 * On v�rifie que la liste existe...
                 */
                if (status_slist) {

                        /*
                         * ... et qu'elle n'est pas vide.
                         */
                        if (!tbx_slist_is_nil(status_slist)) {

                                /*
                                 * On (re)place le curseur de lecture
                                 * de liste au d�but de la liste.
                                 */
                                tbx_slist_ref_to_head(status_slist);
                                do {
                                        /*
                                         * Objet status (identique a l'objet param�tre).
                                         */
                                        p_mad_buffer_slice_parameter_t status = NULL;

                                        /*
                                         * On r�cup�re l'objet au niveau du curseur de la liste.
                                         */
                                        status = tbx_slist_ref_get(status_slist);

                                        /*
                                         * On teste l'opcode du status.
                                         */
                                        if (status->opcode == mad_os_lost_block) {

                                                /*
                                                 * L'opcode indique qu'une section de bloc a �t� perdue.
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

                        /* On lib�re la liste. */
                        mad_free_parameter_slist(status_slist);
                }


                /*
                 * On affiche le message pour montrer qu'on l'a bien re�u.
                 */
                DISP("msg = [%s]", msg);
        }

        /*
         * On termine l'ex�cution des biblioth�ques.
         */
        common_exit(NULL);

        return 0;
}
