import leo_comm

def driver_init(s):
    for driver_name, driver in s.driver_dict.iteritems():
        driver_ps_list = driver['processes']
        for ps in driver_ps_list:
            leo_comm.send_string(ps.client, driver_name)
    
        for ps in driver_ps_list:
            sync = leo_comm.receive_int(ps.client)

        for ps in driver_ps_list:
            for adapter_name, adapter in (ps.driver_dict[driver_name].iteritems()):

                leo_comm.send_string(ps.client, adapter_name) # adapter_name
                param = leo_comm.receive_string(ps.client) # param
                adapter['parameter'] = param            
                mtu   = leo_comm.receive_uint(ps.client)   # mtu
                adapter['mtu'] = mtu

            leo_comm.send_string(ps.client, '-')
                
    for ps in s.process_list:
        leo_comm.send_string(ps.client, '-')

    for ps in s.process_list:
        cl = ps.client
        for driver_name, driver in s.driver_dict.iteritems():
            leo_comm.send_string(cl, driver_name)
            for driver_ps in driver['processes']:
                for adapter_name, adapter in (driver_ps.driver_dict[driver_name].iteritems()):
                    leo_comm.send_int(cl, driver_ps.global_rank)
                    leo_comm.send_string(cl, adapter_name) # adapter_name
                    leo_comm.send_string(cl, adapter['parameter']) # param
                    leo_comm.send_uint(cl, adapter['mtu'])     # mtu
                    
                leo_comm.send_string(cl, '-')
                
            leo_comm.send_int(cl, -1)
            
        leo_comm.send_string(cl, '-')

def channel_init(s, channel_name, channel):
    # reception parametre de connexion
    ps_l	= channel['processes']
    channel_parameter_dict	= {}
        
    for l, ps in enumerate(ps_l):
        leo_comm.send_string(ps.client, channel_name)
        channel_parameter	= leo_comm.receive_string(ps.client)
        channel_parameter_dict[l]	= channel_parameter

    # connexion, passe 1
    for sl, ps_src in enumerate(ps_l):
        channel_param_src	= channel_parameter_dict[sl]
            
        for dl, ps_dst in enumerate(ps_l):
            if sl == dl:
                continue

            channel_param_dst = channel_parameter_dict[dl]
            
            leo_comm.send_int(ps_src.client,  0)
            leo_comm.send_int(ps_src.client, dl)
            leo_comm.send_int(ps_dst.client,  1)
            leo_comm.send_int(ps_dst.client, sl)
            
            param_src = leo_comm.receive_string(ps_src.client)
            param_dst = leo_comm.receive_string(ps_dst.client)

            leo_comm.send_string(ps_src.client, channel_param_dst)
            leo_comm.send_string(ps_src.client, param_dst)
            leo_comm.send_string(ps_dst.client, channel_param_src)
            leo_comm.send_string(ps_dst.client, param_src)

            ack_src = leo_comm.receive_int(ps_src.client)
            ack_dst = leo_comm.receive_int(ps_dst.client)

    # synchro, passe 1
    for l, ps in enumerate(ps_l):
        leo_comm.send_int(ps.client, -1)
        ack	= leo_comm.receive_int(ps.client)

    # connexion, passe 2
    for sl, ps_src in enumerate(ps_l):
        for dl, ps_dst in enumerate(ps_l):
            if sl == dl:
                continue
                
            leo_comm.send_int(ps_src.client,  0)
            leo_comm.send_int(ps_src.client, dl)
            leo_comm.send_int(ps_dst.client,  1)
            leo_comm.send_int(ps_dst.client, sl)
            
            ack_src = leo_comm.receive_int(ps_src.client)
            ack_dst = leo_comm.receive_int(ps_dst.client)

        # synchro, passe 2
    for l, ps in enumerate(ps_l):
        leo_comm.send_int(ps.client, -1)
        ack	= leo_comm.receive_int(ps.client)        
    

def channels_init(s):
    for channel_name, channel in s.channel_dict.iteritems():
        channel_init(s, channel_name, channel)
        
    for ps in s.process_list:
        leo_comm.send_string(ps.client, '-')

def fchannels_init(s):
    for fchannel_name, fchannel in s.fchannel_dict.iteritems():
        channel = fchannel['channel']
        channel_init(s, fchannel_name, channel)
        
    for ps in s.process_list:
        leo_comm.send_string(ps.client, '-')

def vxchannel_init(s, vchannel_name, vchannel):
    # envoi du nom
    ps_l	= vchannel['processes']
        
    for l, ps in enumerate(ps_l):
        leo_comm.send_string(ps.client, vchannel_name)

    # synchro, passe 1
    for l, ps in enumerate(ps_l):
        ack	= leo_comm.receive_int(ps.client)


def vchannels_init(s):
    for vchannel_name, vchannel in s.vchannel_dict.iteritems():
        vxchannel_init(s, vchannel_name, vchannel)

    for ps in s.process_list:
        leo_comm.send_string(ps.client, '-')

def xchannels_init(s):
    for xchannel_name, xchannel in s.xchannel_dict.iteritems():
        vxchannel_init(s, xchannel_name, xchannel)

    for ps in s.process_list:
        leo_comm.send_string(ps.client, '-')
