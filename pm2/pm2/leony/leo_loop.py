import leo_comm
import leo_session
import leo_merge
import leo_dir_send
import leo_dir_activate
import leo_dir_deactivate

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
_LEO_COMMAND_SESSION_ADDED      = 5
_LEO_COMMAND_UPDATE_DIR         = 6
_LEO_COMMAND_MERGE_CHANNEL      = 7
_LEO_COMMAND_SHUTDOWN_CHANNEL   = 8



command_string_dict = {
    _LEO_COMMAND_END: 'END',
    _LEO_COMMAND_END_ACK: 'END_ACK',
    _LEO_COMMAND_PRINT: 'PRINT',
    _LEO_COMMAND_BARRIER: 'BARRIER',
    _LEO_COMMAND_BARRIER_PASSED: 'BARRIER_PASSED',
    _LEO_COMMAND_SESSION_ADDED: 'SESSION_ADDED',
    _LEO_COMMAND_UPDATE_DIR: 'UPDATE_DIR',
    _LEO_COMMAND_MERGE_CHANNEL: 'MERGE_CHANNEL',
    _LEO_COMMAND_SHUTDOWN_CHANNEL: 'SHUTDOWN_CHANNEL',
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
	if not s.leo.compatibility_mode and s.options.dyn_mode:
	    found = 0
	    for channel in s.channel_dict.values():
		if channel.mergeable == True:
		    found = found + 1
	    if found > 0:
		for session_name, session in leo.session_dict.iteritems():
		    if not session_name == name:
			session = leo.session_dict[session_name]
			if not session.leo.compatibility_mode and session.options.dyn_mode:
			    found = 0
			    for channel in session.channel_dict.values():
				if channel.mergeable == True:
				    found = found + 1
			    if found > 0:
				leo_comm.bcast_int(session, _LEO_COMMAND_SESSION_ADDED)
				logger.info("Signaling for session '%s'" % session_name)

    elif command_word == 'merge':
	name1  = command_tokens.pop(0)
	name2  = command_tokens.pop(0)
	name3  = command_tokens.pop(0)
	if leo.session_dict == {}:
	    logger.error('No session found: no merge possible')
	else:
	    found = 0
	    if leo.session_dict.has_key( name1 ):
		logger.debug("Session '%s' found" % name1)
		session = leo.session_dict[name1]
		if not session.leo.compatibility_mode and session.options.dyn_mode:
		    found = found + 1
		else:
		    logger.debug("Session '%s' not dynamic" % name1)
	    if leo.session_dict.has_key( name2 ):
		logger.debug("Session '%s' found" % name2)
		session = leo.session_dict[name2]
		if not session.leo.compatibility_mode and session.options.dyn_mode:
		    found = found + 1
		else:
		    logger.debug("Session '%s' not dynamic" % name2)
	    if not found == 2:
		logger.error('One session is missing/not dynamic: no merge possible')
	    else:
		found2 = 0
		for channel in leo.session_dict[name1].channel_dict.values():
		    if channel.mergeable == True:
			found2 = found2 + 1
			channel1 = channel
			break
		for channel in leo.session_dict[name2].channel_dict.values():
		    if channel.mergeable == True:
			found2 = found2 + 1
			channel2 = channel
			break
		if not found2 == 2:
		    logger.error('One session hasn\'t a mergeable channel: no merge possible')
		else:
		    if not channel1.net_name == channel2.net_name:
			logger.error('Channels feature different networks: no merge possible')
			#pour le moment, mais en fait, faire un canal virtuel ...
		    else:
			logger.info("Merging sessions '%s' and '%s' into '%s'" % ( name1 ,name2, name3 ))

			#leo_merge.print_session( leo.session_dict[name1] )
			#print 'closing channel 1', channel1.name
			#ps_list = channel1.processes
			#leo_comm.mcast_int(ps_list, _LEO_COMMAND_SHUTDOWN_CHANNEL)
			#leo_comm.mcast_string(ps_list, channel1.name)
			#for ps in ps_list:
			#    leo_comm.wait_for_ack(ps)
			#leo_dir_deactivate.connections_exit(leo.session_dict[name1], ps_list, False)
			
			#leo_merge.print_session( leo.session_dict[name2] )
			#print 'closing channel 2', channel2.name
			#ps_list = channel2.processes
			#leo_comm.mcast_int(ps_list, _LEO_COMMAND_SHUTDOWN_CHANNEL)
			#leo_comm.mcast_string(ps_list, channel2.name)
			#for ps in ps_list:
			#    leo_comm.wait_for_ack(ps)
			#leo_dir_deactivate.connections_exit(leo.session_dict[name2], ps_list, False)
			    
			    
		    leo.session_dict[name3] = leo_merge.merge_sessions( leo.session_dict[name1] , leo.session_dict[name2] , name3 )
		    leo_comm.bcast_int(leo.session_dict[name3], _LEO_COMMAND_UPDATE_DIR)
		    leo_dir_send.send_dir(leo.session_dict[name3])
		    ps_list = leo.session_dict[name3].active_process_dict.values()
		    for ps in ps_list:
			leo_comm.wait_for_ack(ps)
			    
		    logger.info("Session '%s' created" % name3)
		    del leo.session_dict[name1]
		    logger.info("Session '%s' removed" % name1)			
		    del leo.session_dict[name2]
		    logger.info("Session '%s' removed" % name2)
			
    elif command_word == 'split':
	name1  = command_tokens.pop(0)
	name2  = command_tokens.pop(0)
	if leo.session_dict == {}:
	    logger.error('No session found: no split possible')
	else:
	    found = 0
	    if leo.session_dict.has_key( name1 ):
		logger.debug("Session '%s' found" % name1)
		session = leo.session_dict[name1]
		if not session.leo.compatibility_mode and session.options.dyn_mode:
		    found = found + 1
		else:
		    logger.debug("Session '%s' not dynamic" % name1)
	    if leo.session_dict.has_key( name2 ):
		logger.debug("Session '%s' already exists" % name2)
		found = found - 1
	    if not found == 1:
		logger.error('No split possible')
	    else:
		logger.info("splitting  session '%s' into '%s' and '%s'" % ( name1 , name1, name2) )
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
    
    elif command == _LEO_COMMAND_SESSION_ADDED:
	logger.error('Invalid client command: %d' % command)
		
    elif command == _LEO_COMMAND_UPDATE_DIR:
	logger.error('Invalid client command: %d' % command)
			
    elif command == _LEO_COMMAND_MERGE_CHANNEL:
	name = leo_comm.receive_string(ps)
	
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
		channel = s.channel_dict[name]
		ps_list = channel.processes
		#print 'Mcasting to channel processes', name, len(ps_list)
		leo_comm.mcast_int(ps_list,_LEO_COMMAND_MERGE_CHANNEL)
		leo_dir_activate.driver_init(s)
		leo_dir_activate.channel_init_2(s, name, channel)
		s.barrier_dict = None
		
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

	active_list = leo_comm.client_select(ps_list)

        for active in active_list:
            if active == dummy_stdin_client:
                process_user_command(leo)
            else:
                process_client_command(active)
                if len(active.session.active_process_dict) == 0:
                    logger.info("closing session '%s'" % active.session.name)
		    
		    active.session.exit()
                    del leo.session_dict[active.session.name]
                    leo.session_number = leo.session_number - 1		    
                    
		    logger.info("session '%s' completed" % active.session.name)
                    if compat_p:
                        sys.exit(0)
                
        if len(active_list) > 0:
            prompt()
