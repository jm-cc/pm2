import socket
import sys

import leo_loaders
import leo_comm

def hostname_receive(client):
    hostname = leo_comm.receive_string(client)
    return leo_comm.hostname_normalize(hostname)

def session_spawn(s):
    """Spawn the session loader by loader."""

    s.process_list	= []
    temp_node_dict	= {}

    for loader in s.loader_dict.keys():
        loader_stub = eval('leo_loaders.'+loader+'_loader')
        loader_host_list = s.loader_dict[loader]
        loader_stub(s, loader_host_list)

        l = len(loader_host_list)

        while l > 0:
            l = l-1
            client   = leo_comm.client_accept(s.leo)
            hostname = hostname_receive(client)

            node_process_dict = s.node_dict[hostname]

            if not temp_node_dict.has_key(hostname):
                keys = node_process_dict.keys()
                keys.sort()
                temp_node_dict[hostname] = keys

            key	= temp_node_dict[hostname].pop(0)

            ps	= node_process_dict[key]
            ps.client	= client

            leo_comm.send_int(client, ps.global_rank)
            s.process_list.append(ps)
