from optparse import OptionParser

def cmdline_parse(session, args, mode):
    """Parse the command line arguments."""
    
    usage = 'usage: %prog [options] FILENAME -- [appli options]'
    version = '%prog 1.0'
    
    parser = OptionParser(usage=usage, version=version)

    if mode == 'compat' or mode == 'session':
        parser.add_option('-a', '--appli',  dest='appli',
                          metavar='APPLI',  help='set application FILENAME')
        parser.add_option('-f', '--flavor', dest='flavor',
                          metavar='FLAVOR', help='set flavor NAME')
        parser.add_option('-n', '--net',    dest='net',
                          metavar='NETCFG',
                          help='set network configuration FILENAME')

    if mode == 'compat' or mode == 'main':
        parser.add_option('-t', '--trace',  dest='trace',
                          metavar='LOGCFG',
                          help='set Python logging module configuration FILENAME')

    if mode == 'compat' or mode == 'session':
        parser.add_option('-x', '--xterm',
                          action='store_true', dest='xterm_mode',
                          default=True)
        parser.add_option('-l', '--log',
                          action='store_true', dest='log_mode',
                          default=False)
        parser.add_option('-p', '--pause',
                          action='store_true', dest='pause_mode',
                          default=True)
        parser.add_option('-d', '--gdb',
                          action='store_true', dest='gdb_mode',
                          default=False)
        
        parser.add_option('-X', '--noxterm',
                          action='store_false', dest='xterm_mode')
        parser.add_option('-L', '--nolog',
                          action='store_false', dest='log_mode')
        parser.add_option('-P', '--nopause',
                          action='store_false', dest='pause_mode')
        parser.add_option('-D', '--nogdb',
                          action='store_false', dest='gdb_mode')

    if mode == 'session':
        parser.add_option('-y', '--dyn',
                          action='store_true',  dest='dyn_mode', default=False)
        parser.add_option('-Y', '--nodyn',
                          action='store_false', dest='dyn_mode')

    (session.options, session.args) = parser.parse_args(args)
    
    if (mode == 'session' or mode == 'compat') and len(session.args) < 1:
        parser.error('incorrect number of arguments')

