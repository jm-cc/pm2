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
_LEO_COMMAND_PRINT		= 1
_LEO_COMMAND_BARRIER		= 2
_LEO_COMMAND_BARRIER_PASSED	= 3

def prompt():
    sys.stdout.write(prompt_string)
    sys.stdout.flush()

def poll(s):
    ps_dict	= s.active_process_dict
    
    active_client_list	= leo_comm.client_select(ps_dict.keys())
    if len(active_client_list) == 0:
        return

    print

    for active_client in active_client_list:

        command = leo_comm.receive_int(active_client)
        print s.name, 'client command:', command

        if command == _LEO_COMMAND_END:
            if not s.barrier_dict == None:
                print
                print '- inconsistent end of session request: a barrier is ongoing'
                
            del ps_dict[active_client]
            
        elif command == _LEO_COMMAND_PRINT:
            if not s.barrier_dict == None:
                print '- inconsistent print request: a barrier is ongoing'

            string = leo_comm.receive_string(active_client)
            print string
            
        elif command == _LEO_COMMAND_BARRIER:
            if s.barrier_dict == None:
                s.barrier_dict = {active_client: active_client}
                if len(ps_dict) < s.size:
                    print 'inconsistent barrier request: some processes have already leaved the session'
            else:
                if not s.barrier_dict.has_key(active_client):
                    s.barrier_dict[active_client] = active_client
                else:
                    print 'inconsistent barrier request: duplicate barrier command'

                if len(s.barrier_dict) == s.size:
                    # barrier passed...
                    for client in ps_dict.keys():
                        leo_comm.send_int(client, _LEO_COMMAND_BARRIER_PASSED)
                    s.barrier_dict = None
            
        elif command == _LEO_COMMAND_BARRIER_PASSED:
            print 'invalid client command', command
            
        else:
            print 'invalid command', command

        prompt()

def loop(leo):
    prompt()

    while True:
        # logger.info('<loop>')
        for session_name in leo.session_dict.keys():
            session = leo.session_dict[session_name]
            poll(session)
            if len(session.active_process_dict) == 0:
                logger.info("session '"+session.name+"' completed")
                prompt()
                session.exit()
                del leo.session_dict[session_name]

        fd_list = [ 0 ]
        (ilist, olist, elist) = select.select(fd_list, [], [], 0.1)
        if len(ilist) == 1:
            command = sys.stdin.readline()
            command_tokens	= string.split(command)
            if len(command_tokens) == 0:
                prompt()
                continue
            
            command_word	= command_tokens.pop(0)

            if command_word == 'quit':
                print 'quit'
                break
            elif command_word == 'add':
                name	= command_tokens.pop(0)
                print 'add', name
                s	= leo_session.Session(leo, name, command_tokens)
                s.init()
            else:
                print 'invalid command'

            prompt()

        # if len(leo.session_dict) == 0:
        #    break
