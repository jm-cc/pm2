import leo_comm

import logging

logger	= logging.getLogger()

def vchannel_disconnect(s, vchannel):
    vchannel_name	= vchannel.name
    channel_list	= vchannel.channel_list
    fchannel_list	= vchannel.fchannel_list

    for channel in channel_list:
        channel_name	= channel.name
        ps_list	= channel.processes

        for (dl, dst_ps) in enumerate(ps_list):
            dst = dst_ps.client

            for src_ps in ps_list:
                if dst_ps == src_ps:
                    continue

                src = src_ps.client

                leo_comm.send_string(src, vchannel_name)
                leo_comm.send_string(src, channel_name)
                leo_comm.send_int(src, dl)

                leo_comm.send_string(dst, vchannel_name)
                leo_comm.send_string(dst, channel_name)
                leo_comm.send_int(dst, -1)

                leo_comm.wait_for_ack(src)
                leo_comm.wait_for_ack(dst)

                break

    for fchannel in fchannel_list:
        fchannel_name	= fchannel.name
        channel		= fchannel.channel
        ps_list = channel.processes

        for (dl, dst_ps) in enumerate(ps_list):
            dst = dst_ps.client

            for src_ps in ps_list:
                if dst_ps == src_ps:
                    continue

                src = src_ps.client

                leo_comm.send_string(src, vchannel_name)
                leo_comm.send_string(src, fchannel_name)
                leo_comm.send_int(src, dl)

                leo_comm.send_string(dst, vchannel_name)
                leo_comm.send_string(dst, fchannel_name)
                leo_comm.send_int(dst, -1)

                leo_comm.wait_for_ack(src)
                leo_comm.wait_for_ack(dst)

                break
        

def vchannels_disconnect(s):
    leo_comm.bcast(s, leo_comm.wait_for_ack)

    for vchannel in s.vchannel_dict.values():
        vchannel_disconnect(s, vchannel)

    leo_comm.bcast_string(s, '-')

def xchannel_disconnect(s, xchannel):
    xchannel_name	= xchannel.name
    channel_list	= xchannel.channel_list

    for channel in channel_list:
        channel_name	= channel.name
        ps_list	= channel.processes

        for (dl, dst_ps) in enumerate(ps_list):
            for src_ps in ps_list:
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

                leo_comm.wait_for_ack(src)
                leo_comm.wait_for_ack(dst)

                break
        
def xchannels_disconnect(s):
    leo_comm.bcast(s, leo_comm.wait_for_ack)
    for xchannel in s.xchannel_dict.values():
        xchannel_disconnect(s, xchannel)
    leo_comm.bcast_string(s, '-')

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

            leo_comm.wait_for_ack(src)
            leo_comm.wait_for_ack(dst)

def connections_exit(s, ps_list, loopback):
    connection_exit(ps_list, loopback)
    leo_comm.barrier(ps_list)

    connection_exit(ps_list, loopback)
    leo_comm.barrier(ps_list)

def vchannels_exit(s):
    for vchannel in s.vchannel_dict.values():
#        logger.info('vchannels_exit: %s', vchannel['name'])
        ps_list	= vchannel.ps_dict.values()
        leo_comm.mcast_string(ps_list, vchannel.name)
        connections_exit(s, ps_list, True)

    leo_comm.bcast_string(s, '-')

def xchannels_exit(s):
    leo_comm.bcast_string(s, '-')
    
def fchannels_exit(s):
    for fchannel in s.fchannel_dict.values():
#        logger.info('fchannels_exit: %s', fchannel['name'])
        ps_list	= fchannel.channel.processes
        fchannel_name	= fchannel.name
        leo_comm.mcast_string(ps_list, fchannel_name)
        connections_exit(s, ps_list, False)

    leo_comm.bcast_string(s, '-')

def channels_exit(s):
    for channel in s.channel_dict.values():
#        logger.info('channels_exit: %s', channel['name'])
        ps_list	= channel.processes
        channel_name	= channel.name
        leo_comm.mcast_string(ps_list, channel_name)
        connections_exit(s, ps_list, False)

    leo_comm.bcast_string(s, '-')

def drivers_exit(s):
    # pass 1
    for driver_name, driver in s.driver_dict.iteritems():
        leo_comm.mcast_string(driver.processes, driver_name)
        for ps in driver.processes:
            for adapter_name in ps.driver_dict[driver_name].keys():
                leo_comm.send_string(ps.client, adapter_name)
                leo_comm.wait_for_ack(ps.client)

            leo_comm.send_string(ps.client, '-')
            
        # leo_comm.mcast(driver.processes, lambda x: leo_comm.wait_for_specific_ack(x, -2))
    
    leo_comm.bcast_string(s, '-')

    # pass 2
    for driver_name, driver in s.driver_dict.iteritems():
        leo_comm.mcast_string(driver.processes, driver_name)
        leo_comm.mcast(driver.processes, leo_comm.wait_for_ack)

    leo_comm.bcast_string(s, '-')

def sync(s):
    leo_comm.bcast_int(s, 1)

def sync_exit(s):
    leo_comm.bcast_string(s, 'clean{directory}')
