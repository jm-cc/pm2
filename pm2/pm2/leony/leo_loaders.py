import logging
import os
import string
import time

logger = logging.getLogger('loaders')

def default_loader(s, loader_host_list):
    """Provide the default session loader stub."""

    # Application command
    app_args = []
    app_args.extend(['--mad_leonie', s.leo.server.hostname])
    app_args.extend(['--mad_link', str(s.leo.server.port)])

    if not s.leo.compatibility_mode and s.options.dyn_mode:
        app_args.append('--mad_dyn_mode')

    if s.args is not None:
        app_args.extend(s.args)

    app_command         = s.options.appli

    # Relay command (leo-load)
    relay_args		= []
    if s.options.log_mode:
        relay_args.append('-l')

    if s.options.pause_mode:
        relay_args.append('-p')

    if s.options.gdb_mode:
        relay_args.append('-d')

    if s.options.xterm_mode:
        relay_args.append('-x')

    relay_args.extend(['-f', s.options.flavor])
    relay_args.append(app_command)
    relay_args.extend(app_args)
    relay_command	= 'leo-load'


    # Environment command (env)
    env_args		= [relay_command]
    env_args.extend(relay_args)
    env_command         = 'env'


    # Remote shell command (ssh)
    ssh_args		= ['-n', env_command]
    ssh_args.extend(env_args)
    ssh_command         = 'ssh'


    # Spawn loop
    for process in loader_host_list:
        (hostname, id)	= process.location
        command	= [ssh_command, hostname]
        command.extend(ssh_args)
        logger.debug(command)
        pid	= os.spawnvp(os.P_NOWAIT, ssh_command, command)        
        time.sleep(1)
