# -*- encoding: iso-8859-1 -*-

import logging
import sys
import sets

import leo_pp

logger = logging.getLogger('vchannels')

def vchannels_process(s, mux_p):

    if mux_p:
        cfg_key = 'xchannels'
        vxchannel_dict = s.xchannel_dict
    else:
        cfg_key = 'vchannels'
        fchannel_dict   = s.fchannel_dict
        vxchannel_dict	= s.vchannel_dict

    # s'il n'y a pas de canaux virtuels dans le fichier de configuration
    if not s.appnetdata.has_key(cfg_key):

        # on ne va pas plus loin
        return

    # sinon, on r�cup�re la liste des canaux virtuels configur�s
    vxchannel_list = s.appnetdata[cfg_key]

    # on normalize la liste si l'utilisateur � fait une simplification
    # dans le fichier de configuration (suppression des parenth�ses
    # pour une liste � un unique �l�ment)
    if type(vxchannel_list) is not list:
        vxchannel_list = [vxchannel_list]

    # on traite chaque canal virtuel en s�quence
    for vxchannel in vxchannel_list:
        
        logger.info('processing vxchannel %s', vxchannel['name'])
        
        # liste de noms, telle que dans le fichier de configuration
        old_channel_list	= vxchannel['channels']

        # liste de canaux r�guliers et de forwarding
        channel_list	= []

        if not mux_p:
            fchannel_list	= []

        ps_dict = {}
        
        rt   = {} # routing table

        # ensemble des rangs globaux des processus du canal virtuel
        g_set           = sets.ImmutableSet()

        # parcours des noms de canaux et initialisation du canal virtuel
        for channel_name in old_channel_list:

            # r�cup�ration du canal r�gulier
            channel	= s.channel_dict[channel_name]

            # changement du status du canal
            channel['public']	= False

            # ajout � la liste de canaux
            channel_list.append(channel)

            if mux_p:
                fchannel	= None
            else:
                # initialisation du canal de forwarding
                fchannel	= {}

                # nom du fcanal
                fchannel['name']	= 'f_' + channel_name

                # canal ma�tre du fcanal
                fchannel['channel']	= channel

                # ajout � la liste de fcanaux
                fchannel_list.append(fchannel)

                # ajout du canal de forwarding dans la table de la session
                # (note: le canal r�gulier est d�ja r�pertori� au niveau de
                #  la session)
                fchannel_dict[fchannel['name']] = fchannel

            # r�cup�ration de la liste des processus du canal r�gulier
            ps_l	= channel['processes']

            # construction du dictionnaire des processus du canal
            for ps in ps_l:
                if not ps_dict.has_key(ps.global_rank):
                    ps_dict[ps.global_rank] = ps

            # construction de l'ensemble des rangs globaux au canal r�gulier
            channel_g_set = sets.ImmutableSet([ps.global_rank for ps in ps_l])

            # remplissage de la rt avec les connexions du canal r�gulier
            for g_src in channel_g_set:
                for g_dst in channel_g_set:
                    
                    # pas de connexion 'boucle' dans les canaux r�guliers
                    if g_src == g_dst:
                        continue

                    # la table de routage est index�e par
                    # un tuple (rang global source, rang global destination)
                    key	= (g_src, g_dst)
                    logger.info('direct route %s', str(key))

                    # s'il n'y a pas d�ja une route entre src et dst
                    if not rt.has_key(key):
                        
                        # on rajoute la connexion du canal r�gulier comme
                        # route directe
                        #
                        # organisation du quartet:
                        # - canal pour la connexion
                        # - fcanal associ�
                        # - rang global de la destination
                        # - booleen indiquant si la route est directe ou non
                        rt[key] = (channel, fchannel, g_dst, True)

            logger.info('channel %s: globals %s', str(channel_name), str(channel_g_set))
            
            # rajout des rangs globaux du canal r�gulier aux rangs
            # globaux du canal virtuel par union des deux ensembles
            g_set = g_set.union(channel_g_set)
            logger.info('vchannel: globals %s', str(g_set))

        # affectation des listes de canaux/fcanaux au canal virtuel
        vxchannel['channels']	= channel_list
        if not mux_p:
            vxchannel['fchannels']	= fchannel_list
            
        vxchannel['g_set']	= g_set
        vxchannel['processes']   = ps_dict.values()

        # boucle de generation de la table de routage du canal virtuel
        while True:

            # a chaque iteration, on utilise les infos de rt pour construire
            # la table new_rt
            new_rt			= {}

            # 'vrai' des qu'on a detect� un chemin non r�solu � l'it�ration
            # courante
            need_additional_pass	= False

            # 'vrai' des qu'on a ajout� au moins une route durant l'it�ration
            # courante
            converging			= False

            # boucle sur toutes les connexions virtuelles
            for g_src in g_set:
                for g_dst in g_set:

                    # construction de la cl� de la connexion virtuelle
                    # consid�r�e
                    key	= (g_src, g_dst)
                    logger.info('routing %s', str(key))

                    # si une route existe pour cette connexion virtuelle
                    if rt.has_key(key):

                        # on copie cette route dans la nouvelle table
                        new_rt[key] = rt[key]
                        # et on passe � la connexion virtuelle suivante
                        continue

                    # sinon, on essaie de cr�er une route indirecte
                    # qu'on commence par initialiser � None (~ NULL)
                    path = None

                    # et on cherche une passerelle
                    for g_med in g_set:
                        # on consid�re un passage par med
                        
                        key_src = (g_src, g_med)
                        # si on ne sait pas aller � med depuis src
                        if not rt.has_key(key_src):
                            # on poursuit la recherche d'une passerelle
                            continue
                        
                        key_dst = (g_med, g_dst)
                        # si on ne sait pas aller de med � dst
                        if not rt.has_key(key_dst):
                            # on poursuit la recherche d'une passerelle
                            continue

                        # si on arrive ici, c'est qu'on a trouv� un med
                        # accessible depuis src et capable d'atteindre dst
                        logger.info('new path: %s, %s by %s', str(g_src), str(g_dst), str(g_med))

                        # le premier saut de la route indirecte est le m�me
                        # que src->med, mais avec le flag 'route directe' �
                        # faux
                        (_ch, _fch, _g_rank, _last) = rt[key_src]
                        path = (_ch, _fch, _g_rank, False)
                        break

                    # on fait le point sur la recherche d'une route indirecte

                    # si on n'a pas trouv� de chemin
                    if path == None:

                        # alors, on a besoin d'une passe
                        # suppl�mentaire (ou la convergence est
                        # impossible, mais on ne le sais pas encore)
                        need_additional_pass = True

                    # si au contraire on a trouv� un chemin indirect
                    else:

                        # on met ce chemin dans la nouvelle table
                        new_rt[key] = path

                        # on progress�, et on le fait savoir
                        converging = True

            # on fait le point sur l'it�ration globale

            # si on a besoin d'une nouvelle passe alors qu'on n'a pas
            # progress� � celle-ci,
            if need_additional_pass and not converging:

                # on ne vas pas plus loin: le canal virtuel a des
                # composantes non connexes
                logger.critical('routing table generation is not converging')
                sys.exit(1)

            # la nouvelle table devient la table de r�f�rence
            rt = new_rt

            # si on n'a plus de chemins non r�solus, on cesse d'it�rer
            if not need_additional_pass:
                break

        # on sauve la table dans le canal virtuel
        vxchannel['rt'] = rt

        # on sauve le canal virtuel dans la table des canaux virtuels
        vxchannel_dict[vxchannel['name']] = vxchannel


    
