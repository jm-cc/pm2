import leo_args
import leo_cfg
import leo_comm
import leo_dir_activate
import leo_dir_deactivate
import leo_dir_send
import leo_spawn

import logging

logger	= logging.getLogger()

class Session:

    def __init__(self, leo, name, args, mode):
        self.name		= name
        self.leo		= leo
        self.process_list	= []
        self.node_dict		= {}
        self.driver_dict	= {}
        self.loader_dict	= {}
        self.channel_dict	= {}
        self.fchannel_dict	= {}
        self.vchannel_dict	= {}
        self.xchannel_dict	= {}

        leo.session_dict[name] = self
        leo_args.cmdline_parse(self, args, mode)

    def node_process_dict_get(self, host_name):
        if not self.node_dict.has_key(host_name):
            self.node_dict[host_name] = {}

        return self.node_dict[host_name]

    def loader_process_list_get(self, net_name):
        net	= self.net_dict[net_name]
        loader_name	= net.loader_name

        if not self.loader_dict.has_key(loader_name):
            self.loader_dict[loader_name] = []

        return self.loader_dict[loader_name]

    def net_name_to_dev_name(self, net_name):
        net		= self.net_dict[net_name]
        return net.dev_name

    def driver_process_list_get(self, net_name):
        net	= self.net_dict[net_name]
        driver	= net.get_driver()
        return driver.processes

    def init(self):
        logger.info('0% - parsing configuration file')
        leo_cfg.appcfg_process(self)
        logger.info('10% - spawning session')
        leo_spawn.session_spawn(self)
        logger.info('20% - sending directory')
        leo_dir_send.send_dir(self)
        logger.info('30% - initializing drivers')
        leo_dir_activate.driver_init(self)
        logger.info('40% - initializing channels')
        leo_dir_activate.channels_init(self)
        logger.info('50% - initializing forwarding channels')
        leo_dir_activate.fchannels_init(self)
        logger.info('60% - initializing virtual channels')
        leo_dir_activate.vchannels_init(self)
        logger.info('70% - initializing multiplexing channels')
        leo_dir_activate.xchannels_init(self)
        
        logger.info('80% - building active processes container')
        self.active_process_dict	= dict([(ps, ps) for ps in self.process_list])
        logger.info('90% - initializing remaining internal state structures')
        self.size	= len(self.process_list)
        self.barrier_dict	= None
        logger.info('100% - session ready')


    def exit(self):
        logger.info('0% - synchronizing processes')
        leo_dir_deactivate.sync(self)
        logger.info('10% - disconnecting virtual channels')
        leo_dir_deactivate.vchannels_disconnect(self)
        logger.info('20% - disconnecting multiplexing channels')
        leo_dir_deactivate.xchannels_disconnect(self)
        logger.info('30% - cleaning virtual channels')
        leo_dir_deactivate.vchannels_exit(self)
        logger.info('40% - cleaning multiplexing channels')
        leo_dir_deactivate.xchannels_exit(self)
        logger.info('50% - cleaning forwarding channels')
        leo_dir_deactivate.fchannels_exit(self)
        logger.info('60% - cleaning channels')
        leo_dir_deactivate.channels_exit(self)
        logger.info('70% - cleaning drivers')
        leo_dir_deactivate.drivers_exit(self)
        logger.info('80% - synchronizing processes')
        leo_dir_deactivate.sync_exit(self)
        logger.info('100% - session removed')

        for ps in self.process_list:
            leo_comm.client_close(ps.client)
            ps.client = None


#_____________
