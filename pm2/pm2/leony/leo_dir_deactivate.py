import leo_comm

import logging

logger	= logging.getLogger()

def wait_for_ack(client):
    ack = leo_comm.receive_int(client)
    if not ack == -1:
        print 'invalid ack value'

def mcast(ps_list, f, *a, **b):
    for ps in ps_list:
        f(ps.client, *a, **b)

def bcast(s, f, *a, **b):
    mcast(s.process_list, f, *a, **b)

def mcast_string(ps_list, data):
    mcast(ps_list, leo_comm.send_string, data)

def bcast_string(s, data):
    bcast(s, leo_comm.send_string, data)

def mcast_int(ps_list, data):
    mcast(ps_list, leo_comm.send_int, data)

def bcast_int(s, data):
    bcast(s, leo_comm.send_int, data)

def sync(s):
    bcast_int(s, 1)

def barrier(ps_list):
    mcast_int(ps_list, -1)
    mcast(ps_list, wait_for_ack)

def vchannel_disconnect(s, vchannel):
    vchannel_name	= vchannel['name']
    channel_list	= vchannel['channels']
    fchannel_list	= vchannel['fchannels']

    for channel in channel_list:
        channel_name = channel['name']
        ps_list = channel['processes']

        for src_ps in ps_list:
            for (dl, dst_ps) in enumerate(ps_list):
                if dst_ps == src_ps:
                    continue

                src = src_ps.client
                dst = dst_ps.client

                leo_comm.send_string(src, vchannel_name)
                leo_comm.send_string(src, channel_name)
                leo_comm.send_int(src, dl)

                leo_comm.send_string(dst, vchannel_name)
                leo_comm.send_string(dst, channel_name)
                leo_comm.send_int(dst, -1)

                wait_for_ack(src)
                wait_for_ack(dst)

                break

    for fchannel in fchannel_list:
        fchannel_name	= fchannel['name']
        channel		= fchannel['channel']
        ps_list = channel['processes']

        for src_ps in ps_list:
            for (dl, dst_ps) in enumerate(ps_list):
                if dst_ps == src_ps:
                    continue

                src = src_ps.client
                dst = dst_ps.client

                leo_comm.send_string(src, vchannel_name)
                leo_comm.send_string(src, fchannel_name)
                leo_comm.send_int(src, dl)

                leo_comm.send_string(dst, vchannel_name)
                leo_comm.send_string(dst, fchannel_name)
                leo_comm.send_int(dst, -1)

                wait_for_ack(src)
                wait_for_ack(dst)

                break
        

def vchannels_disconnect(s):
    bcast(s, wait_for_ack)

    for vchannel in s.vchannel_dict.values():
        vchannel_disconnect(s, vchannel)

    bcast_string(s, '-')

def xchannel_disconnect(s, xchannel):
    xchannel_name	= xchannel['name']
    channel_list	= xchannel['channels']

    for channel in channel_list:
        channel_name = channel['name']
        ps_list = channel['processes']

        for src_ps in ps_list:
            for (dl, dst_ps) in enumerate(ps_list):
                if dst_ps == src_ps:
                    continue

                src = src_ps.client
                dst = dst_ps.client

                leo_comm.send_string(src, xchannel_name)
                leo_comm.send_string(src, channel_name)
                leo_comm.send_int(src, dl)

                leo_comm.send_string(dst, xchannel_name)
                leo_comm.send_string(dst, channel_name)
                leo_comm.send_int(dst, -1)

                wait_for_ack(src)
                wait_for_ack(dst)

                break
        
def xchannels_disconnect(s):
    for xchannel in s.xchannel_dict.values():
        xchannel_disconnect(s, xchannel)
    bcast_string(s, '-')

def connection_exit(ps_list, loopback):
    for (sl, src_ps) in enumerate(ps_list):
        src = src_ps.client
        for (dl, dst_ps) in enumerate(ps_list):
            if not loopback and sl == dl:
                continue

#            logger.info('connection exit: %s --> %s', str(sl), str(dl))
            
            dst = dst_ps.client
            
            leo_comm.send_int(src, 0)
            leo_comm.send_int(src, dl)
            
            leo_comm.send_int(dst, 1)
            leo_comm.send_int(dst, sl)

            wait_for_ack(src)
            wait_for_ack(dst)

def connections_exit(s, ps_list, loopback):
    connection_exit(ps_list, loopback)
    barrier(ps_list)

    connection_exit(ps_list, loopback)
    barrier(ps_list)

def vchannels_exit(s):
    for vchannel in s.vchannel_dict.values():
#        logger.info('vchannels_exit: %s', vchannel['name'])
        ps_list	= vchannel['processes']
        vchannel_name	= vchannel['name']
        mcast_string(ps_list, vchannel_name)
        connections_exit(s, ps_list, True)

    bcast_string(s, '-')

def xchannels_exit(s):
    bcast_string(s, '-')
    
def fchannels_exit(s):
    for fchannel in s.fchannel_dict.values():
#        logger.info('fchannels_exit: %s', fchannel['name'])
        ps_list	= fchannel['channel']['processes']
        fchannel_name	= fchannel['name']
        mcast_string(ps_list, fchannel_name)
        connections_exit(s, ps_list, False)

    bcast_string(s, '-')

def channels_exit(s):
    for channel in s.channel_dict.values():
#        logger.info('channels_exit: %s', channel['name'])
        ps_list	= channel['processes']
        channel_name	= channel['name']
        mcast_string(ps_list, channel_name)
        connections_exit(s, ps_list, False)

    bcast_string(s, '-')

def drivers_exit(s):
    # pass 1
    for driver_name, driver in s.driver_dict.iteritems():
        ps_list = driver['processes']
        mcast_string(ps_list, driver_name)
        for ps in ps_list:
            for adapter_name in ps.driver_dict[driver_name].keys():
                leo_comm.send_string(ps.client, adapter_name)

            leo_comm.send_string(ps.client, '-')
            
        mcast(ps_list, wait_for_ack)
    
    bcast_string(s, '-')

    # pass 2
    for driver_name, driver in s.driver_dict.iteritems():
        ps_list = driver['processes']
        mcast_string(ps_list, driver_name)
        mcast(ps_list, wait_for_ack)

    bcast_string(s, '-')

def sync_exit(s):
    bcast_string(s, 'clean{directory}')
