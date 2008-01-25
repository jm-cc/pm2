import leo_comm

import logging

logger	= logging.getLogger()

def send_processes(s, cl):
    l = len(s.process_list)
    leo_comm.send_int(cl, l)
    for process in s.process_list:
        leo_comm.send_int(cl, process.global_rank)

    leo_comm.send_string(cl, 'end{processes}')

def send_nodes(s, cl):
    node_keys = s.node_dict.keys()
    node_keys.sort()

    l = len(node_keys)
    leo_comm.send_int(cl, l)

    for hostname in node_keys:
        leo_comm.send_string(cl, hostname)
        node_process_dict = s.node_dict[hostname]
        id_keys = node_process_dict.keys()
        id_keys.sort()

        for id in id_keys:
            leo_comm.send_int(cl, node_process_dict[id].global_rank)
            leo_comm.send_int(cl, id)

        leo_comm.send_int(cl, -1)

    leo_comm.send_string(cl, 'end{nodes}')

def send_adapters(s, cl):
    leo_comm.send_int(cl, 1)
    leo_comm.send_string(cl, 'default') # adapter name
    leo_comm.send_string(cl, '-')       # adapter selector

def send_drivers(s, cl):
    l = len(s.net_dict)
    leo_comm.send_int(cl, l)
    
    for net_name, net in s.net_dict.iteritems():
        leo_comm.send_string(cl, net_name)
        leo_comm.send_string(cl, net.dev_name)
        for id, process in enumerate(net.processes):
            leo_comm.send_int(cl, process.global_rank)
            leo_comm.send_int(cl, id)
            send_adapters(s, cl)
            # Driver parameter (for QsNet)
            # not supported for now            
            leo_comm.send_string(cl, '-')

        leo_comm.send_int(cl, -1)

    leo_comm.send_string(cl, 'end{drivers}')

def send_channels(s, cl):
    l = len(s.channel_dict)
    leo_comm.send_int(cl, l)

    for channel_name, channel in s.channel_dict.iteritems():
        leo_comm.send_string(cl, channel_name)
        if channel.public:
            leo_comm.send_uint(cl, 1)
        else:
            leo_comm.send_uint(cl, 0)

	if channel.mergeable == True:
	    leo_comm.send_uint(cl, 1)
	else:
	    leo_comm.send_uint(cl, 0)
        
	leo_comm.send_string(cl, channel.net_name)
	    
        # processes
        ps_l = channel.processes
        for id, ps in enumerate(ps_l):
            leo_comm.send_int(cl, ps.global_rank)
            leo_comm.send_int(cl, id)
            leo_comm.send_string(cl, 'default') # adapter name

        leo_comm.send_int(cl, -1)

        # topology table
        nb = len(ps_l)
        for x in xrange(nb):
            for y in xrange(nb):
                if x == y:
                    leo_comm.send_int(cl, 0)
                else:
                    leo_comm.send_int(cl, 1)

    leo_comm.send_string(cl, 'end{channels}')

def send_fchannels(s, cl):
    l = len(s.fchannel_dict)
    leo_comm.send_int(cl, l)

    for fchannel_name, fchannel in s.fchannel_dict.iteritems():
        leo_comm.send_string(cl, fchannel_name)
        leo_comm.send_string(cl, fchannel.channel.name)
    
    leo_comm.send_string(cl, 'end{fchannels}')

def send_vchannels(s, cl):
    l = len(s.vchannel_dict)
    leo_comm.send_int(cl, l)

    for vchannel_name, vchannel in s.vchannel_dict.iteritems():
        leo_comm.send_string(cl, vchannel_name)
        
        l	= len(vchannel.channel_list)
        leo_comm.send_int(cl, l)

        for channel in vchannel.channel_list:
            leo_comm.send_string(cl, channel.name)
        
        l	= len(vchannel.fchannel_list)
        leo_comm.send_int(cl, l)

        for fchannel in vchannel.fchannel_list:
            leo_comm.send_string(cl, fchannel.name)

        l_src	= 0
        for g_src in vchannel.g_set:
            leo_comm.send_int(cl, g_src)
            leo_comm.send_int(cl, l_src)

            l_dst	= 0
            for g_dst in vchannel.g_set:
                leo_comm.send_int(cl, g_dst)
                leo_comm.send_int(cl, l_dst)
                (channel, fchannel, g_med, final) = vchannel.rt[(g_src, g_dst)]
                if final:
                    leo_comm.send_string(cl, channel.name)
                else:
                    leo_comm.send_string(cl, fchannel.name)
                    
                leo_comm.send_int(cl, g_med)
                l_dst	= l_dst + 1

            leo_comm.send_int(cl, -1)
            l_src	= l_src + 1
        
        leo_comm.send_int(cl, -1)
        
    leo_comm.send_string(cl, 'end{vchannels}')

def send_xchannels(s, cl):
    l = len(s.xchannel_dict)
    leo_comm.send_int(cl, l)

    for xchannel_name, xchannel in s.xchannel_dict.iteritems():
        leo_comm.send_string(cl, xchannel_name)
        
        l	= len(xchannel.channel_list)
        leo_comm.send_int(cl, l)

        for channel in xchannel.channel_list:
            leo_comm.send_string(cl, channel.name)
        
        l	= len(xchannel.sub_channel_list)
        leo_comm.send_int(cl, l)

        for sub_channel_name in xchannel.sub_channel_list:
            leo_comm.send_string(cl, sub_channel_name)
        
        l_src = 0
        for g_src in xchannel.g_set:
            leo_comm.send_int(cl, g_src)
            leo_comm.send_int(cl, l_src)

            l_dst = 0
            for g_dst in xchannel.g_set:
                leo_comm.send_int(cl, g_dst)
                leo_comm.send_int(cl, l_dst)
                (channel, fchannel, g_med, final) = xchannel.rt[(g_src, g_dst)]

                leo_comm.send_string(cl, channel.name)
                leo_comm.send_int(cl, g_med)

                l_dst = l_dst + 1
                
            leo_comm.send_int(cl, -1)
            l_src = l_src + 1
        
        leo_comm.send_int(cl, -1)
        
    leo_comm.send_string(cl, 'end{xchannels}')

def send_dir(s):
    for process in s.process_list:
        send_processes(s, process.client)
        logger.debug('processes sent')
        send_nodes(s, process.client)
        logger.debug('nodes sent')
        send_drivers(s, process.client)
        logger.debug('drivers sent')
        send_channels(s, process.client)
        logger.debug('channels sent')
        send_fchannels(s, process.client)
        logger.debug('fchannels sent')
        send_vchannels(s, process.client)
        logger.debug('vchannels sent')
        send_xchannels(s, process.client)
        logger.debug('xchannels sent')
        leo_comm.send_string(process.client, 'end{directory}')
