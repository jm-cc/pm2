import leo_comm
import leo_session

import logging
import os
import select
import string
import sys
import time

logger	= logging.getLogger()
prompt_string = 'leony$ '

_LEO_COMMAND_END		= 0
_LEO_COMMAND_END_ACK		= 1
_LEO_COMMAND_PRINT		= 2
_LEO_COMMAND_BARRIER		= 3
_LEO_COMMAND_BARRIER_PASSED	= 4

command_string_dict = {
    _LEO_COMMAND_END: 'END',
    _LEO_COMMAND_END_ACK: 'END_ACK',
    _LEO_COMMAND_PRINT: 'PRINT',
    _LEO_COMMAND_BARRIER: 'BARRIER',
    _LEO_COMMAND_BARRIER_PASSED: 'BARRIER_PASSED'
    }

def prompt():
    if compat_p:
        return
    
    sys.stdout.write(prompt_string)
    sys.stdout.flush()

def process_user_command(leo):
    command = sys.stdin.readline()
    command = command.rstrip()
    logger.info('user command: %s' % command)
    command_tokens	= string.split(command)
    if len(command_tokens) == 0:
        return        
    
    command_word	= command_tokens.pop(0)

    if command_word == 'quit':
        logger.info('quit')
        sys.exit(0)
    elif command_word == 'add':
        name	= command_tokens.pop(0)
        logger.info("opening session '%s'" % name)
        s	= leo_session.Session(leo, name, command_tokens, 'session')
        s.init()
    else:
        logger.error('invalid command')

def process_client_command(ps):
    s = ps.session
    
    command = leo_comm.receive_int(ps)

    if command_string_dict.has_key(command):
        logger.info('session %s - ps %d: client command = %s', s.name, ps.global_rank, command_string_dict[command])

    if command == _LEO_COMMAND_END:
        if not s.barrier_dict == None:
            logger.warning('- inconsistent end of session request: a barrier is ongoing')
            
        del s.active_process_dict[ps]
        
    elif command == _LEO_COMMAND_PRINT:
        if not s.barrier_dict == None:
            logger.warning('- inconsistent print request: a barrier is ongoing')

        string = leo_comm.receive_string(ps)
        logger.info(string)
        
    elif command == _LEO_COMMAND_BARRIER:
        if s.barrier_dict == None:
            s.barrier_dict = {ps: ps}
            if len(s.active_process_dict) < s.size:
                logger.warning('inconsistent barrier request: some processes have already leaved the session')
        else:
            if not s.barrier_dict.has_key(ps):
                s.barrier_dict[ps] = ps
            else:
                logger.warning('inconsistent barrier request: duplicate barrier command')

            if len(s.barrier_dict) == s.size:
                # barrier passed...
                for client in s.active_process_dict.keys():
                    leo_comm.send_int(client, _LEO_COMMAND_BARRIER_PASSED)
                s.barrier_dict = None
        
    elif command == _LEO_COMMAND_BARRIER_PASSED:
        logger.error('invalid client command: %d' % command)
        
    else:
        logger.error('invalid command: %d' % command)

def loop(leo):
    dummy_stdin_client	= (0, '<stdin>')
    global compat_p
    compat_p	= leo.compatibility_mode
    prompt()

    while True:
        if compat_p:
            ps_list = []
        else:
            ps_list = [ dummy_stdin_client ]
        
        for session_name, session in leo.session_dict.iteritems():
            session = leo.session_dict[session_name]
            ps_list = ps_list + session.active_process_dict.values()

        active_list	= leo_comm.client_select(ps_list)

        for active in active_list:
            if active == dummy_stdin_client:
                process_user_command(leo)
            else:
                process_client_command(active)
                if len(active.session.active_process_dict) == 0:
                    logger.info("closing session '%s'" % session.name)
                    session.exit()
                    del leo.session_dict[session_name]
                    logger.info("session '%s' completed" % session.name)
                    if compat_p:
                        sys.exit(0)
                
        if len(active_list) > 0:
            prompt()
