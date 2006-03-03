# -*- encoding: iso-8859-15 -*-

import leo_comm

class Cnx:
    pass

def cnx_graph_traversal(series_dict, f, g):
    """Parcours du graphe d'ordonnancement des connexions."""
    for name, series in series_dict.iteritems():

        # on applique f à toutes les connexions de la série
        for cnx in series:
            f(cnx)

        # on applique g à toutes les connexions de la série
        for cnx in series:
            g(cnx)


def driver_init(s):
    for net_name, net in s.net_dict.iteritems():
	
	# update to take into account network/devices
        leo_comm.mcast_string(net.processes, net_name)
        leo_comm.mcast(net.processes, leo_comm.wait_for_ack)

        for ps in net.processes:
            for adapter_name, adapter in (ps.driver_dict[net_name].adapter_dict.iteritems()):

                leo_comm.send_string(ps.client, adapter_name) # adapter_name
                adapter.parameter	= leo_comm.receive_string(ps.client) # param
                adapter.mtu	= leo_comm.receive_uint(ps.client)      # mtu

            # End of adapter list
            leo_comm.send_string(ps.client, '-')


    leo_comm.bcast_string(s, '-')

    for ps in s.process_list:
        cl = ps.client
        for net_name, net in s.net_dict.iteritems():
            leo_comm.send_string(cl, net_name)
            for driver_ps in net.processes:
                for adapter_name, adapter in (driver_ps.driver_dict[net_name].adapter_dict.iteritems()):
                    leo_comm.send_int(cl, driver_ps.global_rank)
                    leo_comm.send_string(cl, adapter_name)      # adapter_name
                    leo_comm.send_string(cl, adapter.parameter) # param
                    leo_comm.send_uint(cl, adapter.mtu)         # mtu

                leo_comm.send_string(cl, '-')

            leo_comm.send_int(cl, -1)

        leo_comm.send_string(cl, '-')


def channel_init(s, channel_name, channel):
    # version sequentielle
    
    # reception parametre de connexion
    channel_parameter_dict	= {}

    for l, ps in enumerate(channel.processes):
        leo_comm.send_string(ps.client, channel_name)
        channel_parameter	= leo_comm.receive_string(ps.client)
        channel_parameter_dict[l]	= channel_parameter

    # connexion, passe 1
    for sl, ps_src in enumerate(channel.processes):
        channel_param_src	= channel_parameter_dict[sl]

        for dl, ps_dst in enumerate(channel.processes):
            if sl == dl:
                continue

            channel_param_dst = channel_parameter_dict[dl]

            leo_comm.send_int(ps_src.client,  0)
            leo_comm.send_int(ps_src.client, dl)
            leo_comm.send_int(ps_dst.client,  1)
            leo_comm.send_int(ps_dst.client, sl)

            param_src = leo_comm.receive_string(ps_src.client)
            param_dst = leo_comm.receive_string(ps_dst.client)

            leo_comm.send_string(ps_src.client, channel_param_dst)
            leo_comm.send_string(ps_src.client, param_dst)
            leo_comm.send_string(ps_dst.client, channel_param_src)
            leo_comm.send_string(ps_dst.client, param_src)

            ack_src = leo_comm.receive_int(ps_src.client)
            ack_dst = leo_comm.receive_int(ps_dst.client)

    # synchro, passe 1
    leo_comm.barrier(channel.processes)

    # connexion, passe 2
    for sl, ps_src in enumerate(channel.processes):
        for dl, ps_dst in enumerate(channel.processes):
            if sl == dl:
                continue

            leo_comm.send_int(ps_src.client,  0)
            leo_comm.send_int(ps_src.client, dl)
            leo_comm.send_int(ps_dst.client,  1)
            leo_comm.send_int(ps_dst.client, sl)

            ack_src = leo_comm.receive_int(ps_src.client)
            ack_dst = leo_comm.receive_int(ps_dst.client)

    # synchro, passe 2
    leo_comm.barrier(channel.processes)


def channel_init_2(s, channel_name, channel):
    # version parallèle
    
    # reception paramètre de connexion
    channel_parameter_dict	= {}

    ref_count_dict	= {}

    for l, ps in enumerate(channel.processes):
        leo_comm.send_string(ps.client, channel_name)
        channel_parameter	= leo_comm.receive_string(ps.client)
        channel_parameter_dict[l]	= channel_parameter
        ref_count_dict[l]	= 0

    # construction de l'ordonnancement des établissements de connexion
    series_dict	= {}

    processes = channel.processes

    n		= len(processes)
    no_series	= 0

    def store_cnx(num, src, dst):
        cnx = Cnx()
        cnx.sl		= src
        cnx.ps_src	= processes[src]
        cnx.dl		= dst
        cnx.ps_dst	= processes[dst]

        if not series_dict.has_key(num):
            series	= [ cnx ]
            series_dict[num]	= series
        else:
            series	= series_dict[num]
            series.append(cnx)

    if (n % 2) == 0:
        # Pair
        for i in range(n/2):            
            # Connexions impaires
            src = (0 + i) % n
            dst = (n - 1 + i) %n
            
            for j in range(n/2):
                store_cnx(4*i,   src, dst)
                store_cnx(4*i+1, dst, src)
                        
                src = (src + 1) % n
                dst = (dst + n - 1) %n
                
            # Connexions paires
            src = (1 + i) % n
            dst = (n - 1 + i) %n

            for j in range(n/2 - 1):
                    store_cnx(4*i+2,   src, dst)
                    store_cnx(4*i+3, dst, src)
                    
                    src = (src + 1) % n
                    dst = (dst + n - 1) %n

    else:
        # Impair
        for i in range(n):
            if n > 2:
                src = (0 + i) % n
                dst = (n - 1 + i) %n
            
                for j in range(n/2):
                    store_cnx(2*i,   src, dst)
                    store_cnx(2*i+1, dst, src)
                    
                    src = (src + 1) % n
                    dst = (dst + n - 1) %n
            #

    def send_command(cnx):
        leo_comm.send_int(cnx.ps_src.client,  0)
        leo_comm.send_int(cnx.ps_src.client, cnx.dl)
        leo_comm.send_int(cnx.ps_dst.client,  1)
        leo_comm.send_int(cnx.ps_dst.client, cnx.sl)

    def receive_ack(cnx):
        ack_src = leo_comm.receive_int(cnx.ps_src.client)
        ack_dst = leo_comm.receive_int(cnx.ps_dst.client)

    def xchange_param(cnx):
        leo_comm.send_int(cnx.ps_src.client,  0)
        leo_comm.send_int(cnx.ps_src.client, cnx.dl)
        leo_comm.send_int(cnx.ps_dst.client,  1)
        leo_comm.send_int(cnx.ps_dst.client, cnx.sl)
        
        param_src	= leo_comm.receive_string(cnx.ps_src.client)
        param_dst	= leo_comm.receive_string(cnx.ps_dst.client)
        channel_param_src	= channel_parameter_dict[cnx.sl]
        channel_param_dst	= channel_parameter_dict[cnx.dl]
        
        leo_comm.send_string(cnx.ps_src.client, channel_param_dst)
        leo_comm.send_string(cnx.ps_src.client, param_dst)
        leo_comm.send_string(cnx.ps_dst.client, channel_param_src)
        leo_comm.send_string(cnx.ps_dst.client, param_src)

    # connexion, passe 1
    cnx_graph_traversal(series_dict, xchange_param, receive_ack)

    # synchro, passe 1
    leo_comm.barrier(channel.processes)

    # connexion, passe 2
    cnx_graph_traversal(series_dict, send_command, receive_ack)

    # synchro, passe 2
    leo_comm.barrier(channel.processes)


def channels_init(s):
    for channel_name, channel in s.channel_dict.iteritems():
        channel_init_2(s, channel_name, channel)

    leo_comm.bcast_string(s, '-')


def fchannels_init(s):
    for fchannel_name, fchannel in s.fchannel_dict.iteritems():
        channel_init_2(s, fchannel_name, fchannel.channel)

    leo_comm.bcast_string(s, '-')


def vxchannel_init(s, vchannel_name, vchannel):
    ps_l	= vchannel.ps_dict.values()

    leo_comm.mcast_string(ps_l, vchannel_name)
    leo_comm.mcast(ps_l, leo_comm.wait_for_ack)


def vchannels_init(s):
    for vchannel_name, vchannel in s.vchannel_dict.iteritems():
        vxchannel_init(s, vchannel_name, vchannel)

    leo_comm.bcast_string(s, '-')


def xchannels_init(s):
    for xchannel_name, xchannel in s.xchannel_dict.iteritems():
        vxchannel_init(s, xchannel_name, xchannel)

    leo_comm.bcast_string(s, '-')
