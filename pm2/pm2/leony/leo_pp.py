# leo_post_processing.py

import leo_comm

def id_range_normalize(id_range):
    if type(id_range) is not xrange:
        id_range = xrange(id_range, id_range+1)

    return id_range

def node_process_dict_get(s, hostname):
    if not s.node_dict.has_key(hostname):
        s.node_dict[hostname] = {}

    return s.node_dict[hostname]

def list_normalize(l):
    if type(l) is not list:
        l = [l]

    return l

def host_normalize(host):
    if type(host) is not tuple:
        host = (host, [0])

    (hostname, id_range_list) = host
    
    id_range_list	= map(id_range_normalize, id_range_list)
    hostname		= leo_comm.hostname_normalize(hostname)

    return (hostname, id_range_list)

def host_list_get(channel):
    return map(host_normalize, list_normalize(channel['hosts']))

def loader_get(net):
    if not net.has_key('mandatory_loader'):
        net['mandatory_loader'] = 'default'

    return net['mandatory_loader']

def loader_process_list_get(s, channel):
    loader = loader_get(s.net_dict[channel['net']])

    if not s.loader_dict.has_key(loader):
        s.loader_dict[loader] = []

    return s.loader_dict[loader]

def driver_process_list_get(s, channel):
    net = s.net_dict[channel['net']]
    dev = net['dev']
    if not s.driver_dict.has_key(dev):
        driver	= {}
        driver['processes']	= []
        
        s.driver_dict[dev]	= driver
    else:
        driver = s.driver_dict[dev]
        
    return driver['processes']

def channel_lists_get(s, channel):
    process_l		= []
    loader_process_l	= loader_process_list_get(s, channel)
    driver_process_l	= driver_process_list_get(s, channel)    
    host_l		= host_list_get(channel)
    return (process_l, loader_process_l, driver_process_l, host_l)


def channel_list_get(s):
    return list_normalize(s.appnetdata['channels'])

def vchannel_list_get(s):
    return list_normalize(s.appnetdata['vchannels'])
