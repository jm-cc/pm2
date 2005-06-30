import logging
import logging.config
import os
import sys

def log_init(filename, compat_p):
    if filename is None:
        pm2_path	= os.getenv('PM2_ROOT')

        if pm2_path is None:
            return

        if compat_p:
            filename = pm2_path + '/leony/leonie_log.cfg'
        else:
            filename = pm2_path + '/leony/leony_log.cfg'

    logging.config.fileConfig(filename)
