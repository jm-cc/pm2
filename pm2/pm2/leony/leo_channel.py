import leo_pp

class Process:
    pass

def channel_process(s, channel):
    """Channel settings processing."""
    (ps_l, loader_ps_l, driver_ps_l, host_l) = leo_pp.channel_lists_get(s, channel)

    dev = s.net_dict[channel['net']]['dev']

    for host in host_l:
        (hostname, id_range_l) = host
        node_ps_dict	= leo_pp.node_process_dict_get(s, hostname)

        for id_range in id_range_l:
            for id in id_range:
                if node_ps_dict.has_key(id):
                    ps = node_ps_dict[id]
                else:
                    ps			= Process()
                    ps.location		= (hostname, id)

                    adapter	= {}
                    adapter['name']	= 'default'

                    adapter_dict 	= {}
                    adapter_dict[adapter['name']] = adapter
                    
                    ps.driver_dict      = {}
                    ps.driver_dict[dev] = adapter_dict
                    
                    ps.global_rank	= s.leo.next_global_rank
                    s.leo.next_global_rank	= s.leo.next_global_rank + 1
                    
                    node_ps_dict[id]	= ps
                    s.process_list.append(ps)
                    loader_ps_l.append(ps)
                    driver_ps_l.append(ps)

                ps_l.append(ps)

    channel['processes']		= ps_l
    channel['public']			= True
    s.channel_dict[channel['name']]	= channel

def channels_process(s):
    """Channel list processing."""

    channel_l = leo_pp.channel_list_get(s)

    for channel in channel_l:
        channel_process(s, channel)
