import leo_comm

_LEO_COMMAND_END		= 0
_LEO_COMMAND_PRINT		= 1
_LEO_COMMAND_BARRIER		= 2
_LEO_COMMAND_BARRIER_PASSED	= 3

def loop(s):
    ps_dict	= dict([(ps.client, ps) for ps in s.process_list])
    initial_ps_nb	= len(ps_dict)
    barrier_dict	= None
    
    while len(ps_dict) > 0:
        active_client_list	= leo_comm.client_select(ps_dict.keys())

        for active_client in active_client_list:

            command = leo_comm.receive_int(active_client)
            print 'command', command, 'from', ps.location

            if command == _LEO_COMMAND_END:
                if not barrier_dict == None:
                    print 'inconsistent end of session request: a barrier is ongoing'
                    
                del ps_dict[active_client]
                
            elif command == _LEO_COMMAND_PRINT:
                if not barrier_dict == None:
                    print 'inconsistent print request: a barrier is ongoing'

                string = leo_comm.receive_string(active_client)
                
            elif command == _LEO_COMMAND_BARRIER:
                if barrier_dict == None:
                    barrier_dict = {active_client: active_client}
                    if len(ps_dict) < initial_ps_nb:
                        print 'inconsistent barrier request: some processes have already leaved the session'
                else:
                    if not barrier_dict.has_key(active_client):
                        barrier_dict[active_client] = active_client
                    else:
                        print 'inconsistent barrier request: duplicate barrier command'

                    if len(barrier_dict) == initial_ps_nb:
                        # barrier passed...
                        for client in ps_dict.keys():
                            leo_comm.send_int(client, _LEO_COMMAND_BARRIER_PASSED)
                        barrier_dict = None
                
            elif command == _LEO_COMMAND_BARRIER_PASSED:
                print 'invalid client command', command
                
            else:
                print 'invalid command', command

    print 'the end...'

