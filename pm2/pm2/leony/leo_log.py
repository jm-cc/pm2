import logging
import logging.config
import os
import sys

def log_init(s):
    if s is None:
        pm2_path	= os.getenv('PM2_ROOT')

        if pm2_path is None:
            return

        s = pm2_path + '/leony/leony_log.cfg'

    logging.config.fileConfig(s)
