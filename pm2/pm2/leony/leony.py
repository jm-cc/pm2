#!/usr/bin/env /usr/bin/python

import leo_args
import leo_cfg
import leo_comm
import leo_spawn
import leo_dir_send
import leo_dir_activate
import leo_loop

import time

class Session:
    pass

s = Session()
leo_args.cmdline_parse(s)
leo_cfg.appcfg_process(s)
leo_comm.server_init(s)
leo_spawn.session_spawn(s)
leo_dir_send.send_dir(s)
leo_dir_activate.driver_init(s)
leo_dir_activate.channels_init(s)
leo_dir_activate.fchannels_init(s)
leo_dir_activate.vchannels_init(s)
leo_dir_activate.xchannels_init(s)
leo_loop.loop(s)

time.sleep(10)
# end
