import leo_comm
import leo_pp

class Process:

    def __init__(self, session, hostname, id):
        self.session		      = session
        self.location		      = (hostname, id)
        self.global_rank	      = session.leo.next_global_rank
        session.leo.next_global_rank  = session.leo.next_global_rank + 1
        self.driver_dict	      = {}

class Driver:

    def __init__(self, session, driver_name):
        self.name	= driver_name
        self.processes	= []

class Net:

    def __init__(self, session, cfg_dict):
        self.name	= cfg_dict['name']
        self.hosts	= cfg_dict['hosts']
        self.dev_name	= cfg_dict['dev']
        self.driver	= None
        self.session	= session
        
        if cfg_dict.has_key('mandatory_loader'):
            loader_name	= cfg_dict['mandatory_loader']
            loader_priority	= 'mandatory'
        elif cfg_dict.has_key('optional_loader'):
            loader_name	= cfg_dict['optional_loader']
            loader_priority	= 'optional'
        else:
            loader_name	= 'default'
            loader_priority	= 'default'

        self.loader_name	= loader_name
        self.loader_priority	= loader_priority

    def get_driver(self):
        if self.driver is None:
            self.driver	= Driver(self.session, self.dev_name)
            self.session.driver_dict[self.driver.name] = self.driver

        return self.driver

class Adapter:

    def __init__(self, name):
        self.name	= name
        self.selector	= '-'
        self.parameter	= '-'
        self.mtu	= 0


class Channel:

    def _id_range_normalize(self, id_range):
        if type(id_range) is not xrange:
            id_range = xrange(id_range, id_range+1)

        return id_range

    def _host_normalize(self, host):
        if type(host) is not tuple:
            host = (host, [0])

        (hostname, id_range_list) = host
    
        id_range_list	= map(self._id_range_normalize, id_range_list)
        hostname	= leo_comm.hostname_normalize(hostname)

        return (hostname, id_range_list)

    def __init__(self, session, cfg_dict):
        self.session	= session

        self.name	= cfg_dict['name']
        self.net_name	= cfg_dict['net']
        self.merge      = cfg_dict['merge']
	
	if self.merge == 'yes' :
	    self.mergeable = True
	else:
	    self.mergeable = False
			
        normalized_list	= leo_pp.list_normalize(cfg_dict['hosts'])
        self.hosts	= map(self._host_normalize, normalized_list)

        self.processes	= []
        self.public	= True

        loader_ps_l	= session.loader_process_list_get(self.net_name)
        driver_ps_l	= session.driver_process_list_get(self.net_name)

        dev_name	= session.net_name_to_dev_name(self.net_name)

        for host in self.hosts:
            (hostname, id_range_l)	= host
            node_ps_dict	= session.node_process_dict_get(hostname)
    
            for id_range in id_range_l:
                for id in id_range:
                    if node_ps_dict.has_key(id):
                        ps = node_ps_dict[id]
                    else:
                        ps = Process(session, hostname, id)
                        
                        node_ps_dict[id]	= ps
                        session.process_list.append(ps)
                        loader_ps_l.append(ps)
                        driver_ps_l.append(ps)
    
                    adapter	= Adapter('default')
    
                    adapter_dict 	= {}
                    adapter_dict[adapter.name] = adapter
                        
                    ps.driver_dict[dev_name] = adapter_dict
    
                    self.processes.append(ps)


def channels_process(s):
    """Channel list processing."""

    channel_l = leo_pp.channel_list_get(s)

    for channel_cfg in channel_l:
        channel	= Channel(s, channel_cfg)
        s.channel_dict[channel.name]	= channel
