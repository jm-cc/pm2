#!/usr/bin/env /usr/bin/python

import leo_comm
import leo_log
import leo_loop
import leo_session

import logging
import time
import sys

class Leony:
    def __init__(self):
        self.session_dict = {}
        self.next_global_rank = 0
        leo_comm.server_init(self)
#__________

logger	= logging.getLogger()
leo	= Leony()
s = leo_session.Session(leo, 'main', sys.argv[1:])
leo_log.log_init(s.options.trace)
s.init()
leo_loop.loop(leo)

# end
