# -*- encoding: iso-8859-15 -*-

import logging
import sys
import sets

import leo_pp

logger = logging.getLogger('vchannels')

class Fchannel:

    def __init__(self, channel):
        self.channel	= channel
        self.name	= 'f_' + channel.name
        self.session	= channel.session
        self.session.fchannel_dict[self.name] = self
        

# end


class VXchannel:

    def __init__(self, session, vxchannel_cfg):
        self.name	= vxchannel_cfg['name']
        logger.debug('new vxchannel %s', self.name)

        self.session		= session
        self.channel_list	= []
        self.ps_dict		= {}
        self.rt			= {}
        self.g_set		= sets.ImmutableSet()

        # liste de noms, telle que dans le fichier de configuration
        old_channel_list	= vxchannel_cfg['channels']

        # parcours des noms de canaux et initialisation du canal virtuel
        for channel_name in old_channel_list:

            # r�cup�ration du canal r�gulier
            channel	= self.session.channel_dict[channel_name]

            # changement du status du canal
            channel.public	= False

            # ajout � la liste de canaux
            self.channel_list.append(channel)

            self.call_integrate_channel(channel)

        self.compute_rt()

    def integrate_channel(self, channel, fchannel):
        
        # construction du dictionnaire des processus du canal
        for ps in channel.processes:
            if not self.ps_dict.has_key(ps.global_rank):
                self.ps_dict[ps.global_rank] = ps

        # construction de l'ensemble des rangs globaux au canal r�gulier
        channel_g_set = sets.ImmutableSet([ps.global_rank for ps in channel.processes])

        # remplissage de la rt avec les connexions du canal r�gulier
        for g_src in channel_g_set:
            for g_dst in channel_g_set:
                
                # pas de connexion 'boucle' dans les canaux r�guliers
                if g_src == g_dst:
                    continue

                # la table de routage est index�e par
                # un tuple (rang global source, rang global destination)
                key	= (g_src, g_dst)
                logger.debug('direct route %s', str(key))

                # s'il n'y a pas d�ja une route entre src et dst
                if not self.rt.has_key(key):
                    
                    # on rajoute la connexion du canal r�gulier comme
                    # route directe
                    #
                    # organisation du quartet:
                    # - canal pour la connexion
                    # - fcanal associ�
                    # - rang global de la destination
                    # - booleen indiquant si la route est directe ou non
                    self.rt[key] = (channel, fchannel, g_dst, True)

        logger.debug('channel %s: globals %s', str(channel.name), str(channel_g_set))
        
        # rajout des rangs globaux du canal r�gulier aux rangs
        # globaux du canal virtuel par union des deux ensembles
        self.g_set = self.g_set.union(channel_g_set)
        logger.debug('vchannel: globals %s', str(self.g_set))

    def compute_rt(self):
        
        # boucle de generation de la table de routage du canal virtuel
        while True:

            # a chaque iteration, on utilise les debugs de rt pour construire
            # la table new_rt
            new_rt			= {}

            # 'vrai' des qu'on a detect� un chemin non r�solu � l'it�ration
            # courante
            need_additional_pass	= False

            # 'vrai' des qu'on a ajout� au moins une route durant l'it�ration
            # courante
            converging			= False

            # boucle sur toutes les connexions virtuelles
            for g_src in self.g_set:
                for g_dst in self.g_set:

                    # construction de la cl� de la connexion virtuelle
                    # consid�r�e
                    key	= (g_src, g_dst)
                    logger.debug('routing %s', str(key))

                    # si une route existe pour cette connexion virtuelle
                    if self.rt.has_key(key):

                        # on copie cette route dans la nouvelle table
                        new_rt[key] = self.rt[key]
                        # et on passe � la connexion virtuelle suivante
                        continue

                    # sinon, on essaie de cr�er une route indirecte
                    # qu'on commence par initialiser � None (~ NULL)
                    path = None

                    # et on cherche une passerelle
                    for g_med in self.g_set:
                        # on consid�re un passage par med
                        
                        key_src = (g_src, g_med)
                        # si on ne sait pas aller � med depuis src
                        if not self.rt.has_key(key_src):
                            # on poursuit la recherche d'une passerelle
                            continue
                        
                        key_dst = (g_med, g_dst)
                        # si on ne sait pas aller de med � dst
                        if not self.rt.has_key(key_dst):
                            # on poursuit la recherche d'une passerelle
                            continue

                        # si on arrive ici, c'est qu'on a trouv� un med
                        # accessible depuis src et capable d'atteindre dst
                        logger.debug('new path: %s, %s by %s', str(g_src), str(g_dst), str(g_med))

                        # le premier saut de la route indirecte est le m�me
                        # que src->med, mais avec le flag 'route directe' �
                        # faux
                        (_ch, _fch, _g_rank, _last) = self.rt[key_src]
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
            self.rt = new_rt

            # si on n'a plus de chemins non r�solus, on cesse d'it�rer
            if not need_additional_pass:
                break

# end

class Vchannel(VXchannel):

    def __init__(self, session, vxchannel_cfg):
        self.fchannel_list	= []
        VXchannel.__init__(self, session, vxchannel_cfg)

    def call_integrate_channel(self, channel):
        fchannel	= Fchannel(channel)

        # ajout � la liste de fcanaux
        self.fchannel_list.append(fchannel)
        self.integrate_channel(channel, fchannel)
        self.session.vchannel_dict[self.name] = self

#end

class Xchannel(VXchannel):

    def __init__(self, session, vxchannel_cfg):
        VXchannel.__init__(self, session, vxchannel_cfg)
        if vxchannel_cfg.has_key('sub_channels'):
            self.sub_channel_list = leo_pp.list_normalize(vxchannel_cfg['sub_channels'])
        else:
            self.sub_channel_list = []

        self.session.xchannel_dict[self.name] = self

    def call_integrate_channel(self, channel):
        self.integrate_channel(channel, None)
#end

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
    vxchannel_list = leo_pp.list_normalize(s.appnetdata[cfg_key])

    # on traite chaque canal virtuel en s�quence
    for vxchannel_cfg in vxchannel_list:

        if mux_p:
            vxchannel = Xchannel(s, vxchannel_cfg)
        else:
            vxchannel = Vchannel(s, vxchannel_cfg)        
        


    
