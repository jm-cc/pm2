# leo_post_processing.py

import leo_comm

def list_normalize(l):
    if type(l) is not list:
        l = [l]

    return l

def channel_list_get(s):
    return list_normalize(s.appnetdata['channels'])

def vchannel_list_get(s):
    return list_normalize(s.appnetdata['vchannels'])
