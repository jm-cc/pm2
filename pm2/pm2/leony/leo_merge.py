import leo_comm
import leo_session
import leo_loop

import logging
import string

logger	= logging.getLogger()

def extend_dict(dict1, dict2):
    for k, v  in dict2.iteritems():		
 	if not dict1.has_key( k ):
 	     dict1[k] = v    

def merge_sessions( session1, session2 , name ):
    session1.name   = name
    if  session1.session_id > session2.session_id:
	session1.session_id = session2.session_id

    session1.leo.session_number = session1.leo.session_number - 1
    
    #liste des process
    session1.process_list.extend(session2.process_list)
    
    #node_dict
    for k, v  in session2.node_dict.iteritems():		
 	if not session1.node_dict.has_key( k ):
	    session1.node_dict[k] = v
	else:
	    dict = {}
	    for i, v2 in enumerate(session1.process_list):
		dict[i] = v2
	    session1.node_dict[k] = dict	
    session1.node_dict.keys().sort()
	    
    #driver dict	
    for  v in session1.driver_dict.values():
	name  = v.name
	for  v2 in session2.driver_dict.values():
	    name2 = v2.name
	    if name == name2:
		v.processes.extend(v2.processes)		
    extend_dict( session1.driver_dict, session2.driver_dict)
    
    #loader dict	
    for k, v  in session2.loader_dict.iteritems():		
 	if not session1.loader_dict.has_key( k ):
	    session1.loader_dict[k] = v
	else:
	    session1.loader_dict[k].extend(v)
    
    #active_process_dict
    extend_dict( session1.active_process_dict , session2.active_process_dict)
    
    #size 
    session1.size = len(session1.process_list)

    #channel dict	
    for k, v  in session2.channel_dict.iteritems():
     	if not session1.channel_dict.has_key( k ):
	    session1.channel_dict[k] = v
	else:
	    #fusionner les deux canaux qui ont le meme nom
	    if session1.channel_dict[k].mergeable == True and session2.channel_dict[k].mergeable: 
		logger.debug("channels are mergeables ... ")		
		session1.channel_dict[k].processes.extend(v.processes) 
		session1.channel_dict[k].mergeable = True
    	    else:
		logger.debug("channels are NOT mergeables ...")
    
    #fchannel dict	
    extend_dict( session1.fchannel_dict, session2.fchannel_dict)    	
    
    #vchannel dict	
    extend_dict( session1.vchannel_dict, session2.vchannel_dict)
    
    #xchannel dict	
    extend_dict( session1.xchannel_dict, session2.xchannel_dict)
        
    #change session for processes
    for i,v in enumerate(session1.process_list):
	v.session = session1
		    
    return session1

def print_session( s ):
    logger.info('session name:              %s' % s.name)
    logger.info('session id:                %d' % s.session_id)
    logger.info('session process list:')
    for i,v in enumerate(s.process_list):
      print i, v, v.global_rank
      
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
	print k, v, v.processes, v.hosts		
    
    #logger.info('session fchannel dict:')  
    #for k, v  in s.fchannel_dict.iteritems():		
    #	print k,v		
    #logger.info('session vchannel dict:')  
    #for k, v  in s.vchannel_dict.iteritems():		
    #	print k,v			
    #logger.info('session xchannel dict:')  
    #for k, v  in s.xchannel_dict.iteritems():		
    #	print k,v			
	
