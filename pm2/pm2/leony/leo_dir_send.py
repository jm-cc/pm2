import leo_comm

def send_processes(s, cl):
    l = len(s.process_list)
    leo_comm.send_int(cl, l)
    for process in s.process_list:
        leo_comm.send_int(cl, process.global_rank)

    leo_comm.send_string(cl, 'end{processes}')

def send_nodes(s, cl):
    node_keys = s.process_dict.keys()
    node_keys.sort()

    l = len(node_keys)
    leo_comm.send_int(cl, l)

    for hostname in node_keys:
        leo_comm.send_string(cl, hostname)
        node_process_dict = s.process_dict[hostname]
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
    l = len(s.driver_dict)
    leo_comm.send_int(cl, l)
    
    for driver_name, process_driver_list in s.driver_dict.iteritems():
        leo_comm.send_string(cl, driver_name)
        for id, process in enumerate(process_driver_list):
            leo_comm.send_int(cl, process.global_rank)
            leo_comm.send_int(cl, id)
            send_adapters(s, cl)

        leo_comm.send_int(cl, -1)

    leo_comm.send_string(cl, 'end{drivers}')

def send_channels(s, cl):
    l = len(s.channel_dict)
    leo_comm.send_int(cl, l)

    for channel_name, channel in s.channel_dict.iteritems():
        leo_comm.send_string(cl, channel_name)
        leo_comm.send_uint(cl, 1) # channel.public
        leo_comm.send_string(cl, s.net_dict[channel['net']]['dev'])

        # processes
        ps_l = channel['processes']
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
    leo_comm.send_int(cl, 0)
    leo_comm.send_string(cl, 'end{fchannels}')

def send_vchannels(s, cl):
    leo_comm.send_int(cl, 0)
    leo_comm.send_string(cl, 'end{vchannels}')

def send_xchannels(s, cl):
    leo_comm.send_int(cl, 0)
    leo_comm.send_string(cl, 'end{xchannels}')

def send_dir(s):
    for process in s.process_list:
        send_processes(s, process.client)
        send_nodes(s, process.client)
        send_drivers(s, process.client)
        send_channels(s, process.client)
        send_fchannels(s, process.client)
        send_vchannels(s, process.client)
        send_xchannels(s, process.client)
        leo_comm.send_string(process.client, 'end{directory}')
