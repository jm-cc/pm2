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

    # sinon, on récupère la liste des canaux virtuels configurés
    vxchannel_list = s.appnetdata[cfg_key]

    # on normalize la liste si l'utilisateur à fait une simplification
    # dans le fichier de configuration (suppression des parenthèses
    # pour une liste à un unique élément)
    if type(vxchannel_list) is not list:
        vxchannel_list = [vxchannel_list]

    # on traite chaque canal virtuel en séquence
    for vxchannel in vxchannel_list:
        
        logger.info('processing vxchannel %s', vxchannel['name'])
        
        # liste de noms, telle que dans le fichier de configuration
        old_channel_list	= vxchannel['channels']

        # liste de canaux réguliers et de forwarding
        channel_list	= []

        if not mux_p:
            fchannel_list	= []

        ps_dict = {}
        
        rt   = {} # routing table

        # ensemble des rangs globaux des processus du canal virtuel
        g_set           = sets.ImmutableSet()

        # parcours des noms de canaux et initialisation du canal virtuel
        for channel_name in old_channel_list:

            # récupération du canal régulier
            channel	= s.channel_dict[channel_name]

            # changement du status du canal
            channel['public']	= False

            # ajout à la liste de canaux
            channel_list.append(channel)

            if mux_p:
                fchannel	= None
            else:
                # initialisation du canal de forwarding
                fchannel	= {}

                # nom du fcanal
                fchannel['name']	= 'f_' + channel_name

                # canal maître du fcanal
                fchannel['channel']	= channel

                # ajout à la liste de fcanaux
                fchannel_list.append(fchannel)

                # ajout du canal de forwarding dans la table de la session
                # (note: le canal régulier est déja répertorié au niveau de
                #  la session)
                fchannel_dict[fchannel['name']] = fchannel

            # récupération de la liste des processus du canal régulier
            ps_l	= channel['processes']

            # construction du dictionnaire des processus du canal
            for ps in ps_l:
                if not ps_dict.has_key(ps.global_rank):
                    ps_dict[ps.global_rank] = ps

            # construction de l'ensemble des rangs globaux au canal régulier
            channel_g_set = sets.ImmutableSet([ps.global_rank for ps in ps_l])

            # remplissage de la rt avec les connexions du canal régulier
            for g_src in channel_g_set:
                for g_dst in channel_g_set:
                    
                    # pas de connexion 'boucle' dans les canaux réguliers
                    if g_src == g_dst:
                        continue

                    # la table de routage est indexée par
                    # un tuple (rang global source, rang global destination)
                    key	= (g_src, g_dst)
                    logger.info('direct route %s', str(key))

                    # s'il n'y a pas déja une route entre src et dst
                    if not rt.has_key(key):
                        
                        # on rajoute la connexion du canal régulier comme
                        # route directe
                        #
                        # organisation du quartet:
                        # - canal pour la connexion
                        # - fcanal associé
                        # - rang global de la destination
                        # - booleen indiquant si la route est directe ou non
                        rt[key] = (channel, fchannel, g_dst, True)

            logger.info('channel %s: globals %s', str(channel_name), str(channel_g_set))
            
            # rajout des rangs globaux du canal régulier aux rangs
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

            # 'vrai' des qu'on a detecté un chemin non résolu à l'itération
            # courante
            need_additional_pass	= False

            # 'vrai' des qu'on a ajouté au moins une route durant l'itération
            # courante
            converging			= False

            # boucle sur toutes les connexions virtuelles
            for g_src in g_set:
                for g_dst in g_set:

                    # construction de la clé de la connexion virtuelle
                    # considérée
                    key	= (g_src, g_dst)
                    logger.info('routing %s', str(key))

                    # si une route existe pour cette connexion virtuelle
                    if rt.has_key(key):

                        # on copie cette route dans la nouvelle table
                        new_rt[key] = rt[key]
                        # et on passe à la connexion virtuelle suivante
                        continue

                    # sinon, on essaie de créer une route indirecte
                    # qu'on commence par initialiser à None (~ NULL)
                    path = None

                    # et on cherche une passerelle
                    for g_med in g_set:
                        # on considère un passage par med
                        
                        key_src = (g_src, g_med)
                        # si on ne sait pas aller à med depuis src
                        if not rt.has_key(key_src):
                            # on poursuit la recherche d'une passerelle
                            continue
                        
                        key_dst = (g_med, g_dst)
                        # si on ne sait pas aller de med à dst
                        if not rt.has_key(key_dst):
                            # on poursuit la recherche d'une passerelle
                            continue

                        # si on arrive ici, c'est qu'on a trouvé un med
                        # accessible depuis src et capable d'atteindre dst
                        logger.info('new path: %s, %s by %s', str(g_src), str(g_dst), str(g_med))

                        # le premier saut de la route indirecte est le même
                        # que src->med, mais avec le flag 'route directe' à
                        # faux
                        (_ch, _fch, _g_rank, _last) = rt[key_src]
                        path = (_ch, _fch, _g_rank, False)
                        break

                    # on fait le point sur la recherche d'une route indirecte

                    # si on n'a pas trouvé de chemin
                    if path == None:

                        # alors, on a besoin d'une passe
                        # supplémentaire (ou la convergence est
                        # impossible, mais on ne le sais pas encore)
                        need_additional_pass = True

                    # si au contraire on a trouvé un chemin indirect
                    else:

                        # on met ce chemin dans la nouvelle table
                        new_rt[key] = path

                        # on progressé, et on le fait savoir
                        converging = True

            # on fait le point sur l'itération globale

            # si on a besoin d'une nouvelle passe alors qu'on n'a pas
            # progressé à celle-ci,
            if need_additional_pass and not converging:

                # on ne vas pas plus loin: le canal virtuel a des
                # composantes non connexes
                logger.critical('routing table generation is not converging')
                sys.exit(1)

            # la nouvelle table devient la table de référence
            rt = new_rt

            # si on n'a plus de chemins non résolus, on cesse d'itérer
            if not need_additional_pass:
                break

        # on sauve la table dans le canal virtuel
        vxchannel['rt'] = rt

        # on sauve le canal virtuel dans la table des canaux virtuels
        vxchannel_dict[vxchannel['name']] = vxchannel


    
