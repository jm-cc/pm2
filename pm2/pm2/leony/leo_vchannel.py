import leo_pp

def vchannels_process(s):
    s.vchannel_dict = {}

    if not s.appnetdata.has_key('vchannels'):
        return

    vchannel_list = s.appnetdata['vchannels']
    if type(vchannel_list) is not list:
        vchannel_list = [vchannel_list]

    for vchannel in vchannel_list:
        channel_list = vchannel['channels']
        for channel_num, channel_name in enumerate(channel_list):
            channel_list[channel_num] = s.channel_dict[channel_name]

        s.vchannel_dict[vchannel['name']] = vchannel

