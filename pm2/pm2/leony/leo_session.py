import leo_args
import leo_cfg
import leo_dir_activate
import leo_dir_deactivate
import leo_dir_send
import leo_spawn

class Session:

    def __init__(self, leo, name, args):
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
        leo_args.cmdline_parse(self, args)

    def init(self):
        leo_cfg.appcfg_process(self)
        leo_spawn.session_spawn(self)
        leo_dir_send.send_dir(self)
        leo_dir_activate.driver_init(self)
        leo_dir_activate.channels_init(self)
        leo_dir_activate.fchannels_init(self)
        leo_dir_activate.vchannels_init(self)
        leo_dir_activate.xchannels_init(self)
        
        self.active_process_dict	= dict([(ps.client, ps) for ps in self.process_list])
        self.size	= len(self.active_process_dict)
        self.barrier_dict	= None


    def exit(self):
        leo_dir_deactivate.sync(self)
        leo_dir_deactivate.vchannels_disconnect(self)
        leo_dir_deactivate.xchannels_disconnect(self)
        leo_dir_deactivate.vchannels_exit(self)
        leo_dir_deactivate.xchannels_exit(self)
        leo_dir_deactivate.fchannels_exit(self)
        leo_dir_deactivate.channels_exit(self)
        leo_dir_deactivate.drivers_exit(self)
        leo_dir_deactivate.sync_exit(self)

#_____________
