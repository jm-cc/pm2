import leo_comm
import leo_session
import leo_loop
import leo_vchannel

import logging
import string
import sets

logger	= logging.getLogger()

def extend_dict(dict1, dict2):
    for k, v  in dict2.iteritems():		
 	if not dict1.has_key( k ):
	    dict1[k] = v    

def merge_sessions( session1, session2 , name , physical):
    session  = leo_session.Session(session1.leo, name )

    session.options = session1.options
    
    session.child_name_list.append(session1.name)
    session.child_name_list.append(session2.name)
    
    if  session1.session_id > session2.session_id:
	session.session_id = session2.session_id
    else:
	session.session_id = session1.session_id
    
    #liste des process
    session.process_list.extend(session1.process_list)
    session.process_list.extend(session2.process_list)
    
    #node_dict
    for k, v  in session1.node_dict.iteritems():		
	session.node_dict[k] = v
    
    for k, v  in session2.node_dict.iteritems():		
 	if not session.node_dict.has_key( k ):
	    session.node_dict[k] = v
	else:
	    dict = {}
	    for i, v2 in enumerate(session.process_list):
		dict[i] = v2
	    session.node_dict[k] = dict	
    session.node_dict.keys().sort()

    #Net dict
    session.net_dict = {}
    for k, v  in session1.net_dict.iteritems():		
	session.net_dict[k] = v
    extend_dict( session.net_dict, session2.net_dict)
	    
    #driver dict
    for k, v  in session1.driver_dict.iteritems():		
	session.driver_dict[k] = v
	
    for  v in session.driver_dict.values():
	name  = v.name
	for  v2 in session2.driver_dict.values():
	    name2 = v2.name
	    if name == name2:
		v.processes.extend(v2.processes)		
		
    extend_dict( session.driver_dict, session2.driver_dict)

    #loader dict	
    for k, v  in session1.loader_dict.iteritems():		
	session.loader_dict[k] = v
	
    for k, v  in session2.loader_dict.iteritems():		
	if not session.loader_dict.has_key( k ):
	    session.loader_dict[k] = v
	else:
	    session.loader_dict[k].extend(v)
    
    #active_process_dict
    session.active_process_dict = {}
    for k, v  in session1.active_process_dict.iteritems():		
    	session.active_process_dict[k] = v
    extend_dict( session.active_process_dict , session2.active_process_dict)
    
    #size 
    session.size = len(session.process_list)

    #channel dict
    if physical == True:	
	logger.debug("physical channels")
	for k, v  in session1.channel_dict.iteritems():
	    session.channel_dict[k] = v
	for k, v  in session2.channel_dict.iteritems():
	    if not session.channel_dict.has_key( k ):
		session.channel_dict[k] = v
	    else:
		#fusionner les deux canaux qui ont le meme nom
		if session.channel_dict[k].mergeable == True and session2.channel_dict[k].mergeable: 
		    logger.debug("channels are mergeables ... ")		    
		    session.channel_dict[k].processes.extend(v.processes) 		    
		    session.channel_dict[k].mergeable = True
		else:
		    logger.debug("channels are NOT mergeables ...")
    else:
	logger.debug("Not physical channels")
	for k, v  in session1.channel_dict.iteritems():
	    name = session1.name + '_' + k	    
	    session.channel_dict[name]      = v
	    session.channel_dict[name].name = name
	    #print name, session.channel_dict[name].name, k
	for k, v  in session2.channel_dict.iteritems():
	    name = session2.name + '_' + k	    
	    session.channel_dict[name]      = v
	    session.channel_dict[name].name = name
	    #print name, session.channel_dict[name].name, k
	    
    #fchannel dict
    for k, v  in session1.fchannel_dict.iteritems():		
	session.fchannel_dict[k] = v
    extend_dict( session.fchannel_dict, session2.fchannel_dict)    	
    
    #vchannel dict
    for k, v  in session1.vchannel_dict.iteritems():		
	session.vchannel_dict[k] = v
    extend_dict( session.vchannel_dict, session2.vchannel_dict)
    
    if (physical == False):
	channel_list = []
	for channel in session.channel_dict.values():
	    if channel.mergeable == True:
		channel_list.append(channel)
	for k, v  in session1.channel_dict.iteritems():
	    if v.mergeable == True:
		key = k
		break
	session.vchannel_dict[key] = leo_vchannel.Vchannel( session,{},channel_list,key)	

	
    #xchannel dict
    for k, v  in session1.xchannel_dict.iteritems():		
	session.xchannel_dict[k] = v
    extend_dict( session.xchannel_dict, session2.xchannel_dict)
        
    #barrier
    session.barrier_dict  = None
    
    #change session for processes
    for i,v in enumerate(session.process_list):
	v.session = session
		    
    return session

def print_session( s ):
    logger.info('session name:............. %s' % s.name)
    logger.info('session parent name:...... %s' % s.parent_name)
    logger.info('session id:............... %d' % s.session_id)
    logger.info('session process list:')
   
    #for i,v in enumerate(s.process_list):
    #  print i, v, v.global_rank
      
    #logger.info('session node dict:')  
    #for k, v  in s.node_dict.iteritems():		
    #	print k,v
    #logger.info('session driver dict:')  
    #for k, v  in s.driver_dict.iteritems():		
    #	print k,v	
    #logger.info('session loader dict:')  
    #for k, v  in s.loader_dict.iteritems():		
    #	print k,v	
    
    logger.info('session channel dict:')  
    for k, v  in s.channel_dict.iteritems():		
	print k, v, v.processes, v.mergeable		
    
    logger.info('session fchannel dict:')  
    for k, v  in s.fchannel_dict.iteritems():		
    	print k,v		
    
    logger.info('session vchannel dict:')  
    for k, v  in s.vchannel_dict.iteritems():		
    	print k,v			
    
    #logger.info('session xchannel dict:')  
    #for k, v  in s.xchannel_dict.iteritems():		
    #	print k,v			
	
