import select
import socket

def pack_int(i):
    """Pack a NTBX int."""
    buf = 'iiii%*d-'%(11, i)
    return buf.ljust(32)

def pack_long(l):
    """Pack a NTBX long."""
    buf = 'llll%*ld-'%(21, l)
    return buf.ljust(32)

def pack_uint(i):
    """Pack a NTBX unsigned int."""
    buf = 'IIII%*u-'%(10, i)
    return buf.ljust(32)

def pack_ulong(l):
    """Pack a NTBX unsigned long."""
    buf = 'LLLL%*lu-'%(20, l)
    return buf.ljust(32)


def unpack_int(b):
    """Unpack a NTBX int."""
    return int(b[4:15])

def unpack_long(b):
    """Unpack a NTBX long."""
    return long(b[4:25])

def unpack_uint(b):
    """Unpack a NTBX unsigned int."""
    return int(b[4:14])

def unpack_ulong(b):
    """Unpack a NTBX unsigned long."""
    return long(b[4:24])


def send(client, b):
    """Sent a string over a client socket."""
    (cs, addr) = client
    #print "sending:", b
    o = 0
    s = len(b)
    while s > 0:
        nb = cs.send(b[o:])
        s = s - nb
        o = o + nb

def receive(client, s):
    """Receive a string over a client socket."""
    (cs, addr) = client
    b = ''
    while s > 0:
        r = cs.recv(s)
        s = s - len(r)
        b = b+r
        
    return b


def send_type(client, pack_func, data):
    """Generically pack some data into a NTBX compatible format and send it."""
    b = pack_func(data)
    send(client, b)

def send_int(client, i):
    """Send a NTBX int."""
    send_type(client, pack_int, i)

def send_long(client, l):
    """Send a NTBX long."""
    send_type(client, pack_long, l)

def send_uint(client, ui):
    """Send a NTBX unsigned int."""
    send_type(client, pack_uint, ui)

def send_ulong(client, ul):
    """Send a NTBX unsigned long."""
    send_type(client, pack_ulong, ul)

def send_string(client, s):
    """Send a NTBX string."""
    s = s + '\0'
    send_int(client, len(s))
    send(client, s)


def receive_type(client, unpack_func):
    """Receive a NTBX type."""
    b = receive(client, 32)
    return unpack_func(b)

def receive_int(client):
    """Receive a NTBX int."""
    return receive_type(client, unpack_int)

def receive_long(client):
    """Receive a NTBX long."""
    return receive_type(client, unpack_long)

def receive_uint(client):
    """Receive a NTBX unsigned int."""
    return receive_type(client, unpack_uint)

def receive_ulong(client):
    """Receive a NTBX unsigned long."""
    return receive_type(client, unpack_ulong)

def receive_string(client):
    """Receive a NTBX string."""
    l = receive_int(client)
    s = receive(client, l)
    return s[:-1]

class Server:
    """Store the Leony TCP server attributes."""
    pass

def server_init(leo):
    """Initialize the Leony TCP server."""
    server		= Server()
    server.socket	= socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.hostname	= socket.gethostname()
    server.socket.bind((server.hostname, 0))
    server.socket.listen(5)
    (ip, port)		= server.socket.getsockname()
    server.port		= port
    server.ip		= ip
    leo.server		= server

def client_accept(leo):
    return leo.server.socket.accept()

def hostname_normalize(hostname):
    """Normalize the hostname."""
    if hostname == 'localhost':
        hostname = socket.gethostname()

    (hostname, aliaslist, ipaddrlist) = socket.gethostbyname_ex(hostname) 
    return hostname

def client_select(client_list):
    idx_dict = {}
    fd_list  = []

    for num, (fd, addr) in enumerate(client_list):
        idx_dict[fd] = num
        fd_list.append(fd)

    (ilist, olist, elist) = select.select(fd_list, [], [], 0.9)

    result_list = [ client_list[idx_dict[x]] for x in ilist ]

    return result_list
