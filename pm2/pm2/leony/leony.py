#!/usr/bin/env /usr/bin/python

import leo_args
import leo_comm
import leo_log
import leo_loop
import leo_session

import logging
import os.path
import time
import sys

class Leony:
    def __init__(self, compat_p):
        self.session_dict	= {}
	self.session_number     = 0
        self.next_global_rank	= 0
        self.compatibility_mode	= compat_p
        leo_comm.server_init(self)
#__________

logger	= logging.getLogger()
name	= os.path.basename(sys.argv[0])
if name == 'leonie' or name == 'leonie.py':
    leo	= Leony(True)
else:
    leo = Leony(False)

if leo.compatibility_mode:
    s	= leo_session.Session(leo, 'main', sys.argv[1:], 'compat')
    leo_log.log_init(s.options.trace, leo.compatibility_mode)
    logger.info("opening session 'main'")
    s.init()
else:
    class Settings:
        pass

    settings	= Settings()
    leo_args.cmdline_parse(settings, sys.argv[1:], 'main')
    leo_log.log_init(settings.options.trace, leo.compatibility_mode)
leo_loop.loop(leo)

# end
