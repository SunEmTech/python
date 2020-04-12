import sys
import socket
import select

from ctypes import *

class NetStruct(BigEndianStructure):
    _pack_ = 1

    def raw(self):
        return bytes(memoryview(self))

    def __new__(self, sb=None):
        if sb:
            return self.from_buffer_copy(sb)
        else:
            return BigEndianStructure.__new__(self)

    def __init__(self, sb=None):
        pass
    
class ExampleNetworkPacket(NetStruct):
    _fields_ = [('version', c_ushort),
                ('reserved', c_ushort),
                ('sanity', c_uint),
                ('datalen', c_uint)]
    _data = (c_ubyte * 0)()

    @property
    def data(self):
        return memoryview(self._data)

    @data.setter
    def data(self, indata):
        self.datalen = len(indata)
        self._data = (self._data._type_ * len(indata))()
        memmove(self._data, indata, len(indata))    

    def raw(self):
        return super(self.__class__, self).raw() + self.data
    

def socket_listen(port):
    # get the hostname
    host = socket.gethostname()
    host = "localhost"
    #port = 5000  # initiate port no above 1024

    listen_socket = socket.socket()  # get instance
    # look closely. The bind() function takes tuple as argument
    listen_socket.bind((host, port))  # bind host address and port together

    # configure how many client the server can listen simultaneously
    listen_socket.listen(12)
    print("listening on host {}, port {}".format(host, port))
    return listen_socket

def socket_accept(sd):
    conn, address = sd.accept()  # accept new connection
    print("Connection from: " + str(address))
    return conn
    
def connect_server(ip, port):
    #host = socket.gethostname()  # as both code is running on same pc
    #port = 5000  # socket server port number

    client_socket = socket.socket()  # instantiate
    client_socket.connect((ip, port))
    print("socket connect ip:{}, port:{}".format(ip, port))
#    try:
#        client_socket.connect((ip, port))  # connect to the server
#    except:
#        print("Error: Connect")
#        client_socket = None
#        sys.exit(1)
#    
    return client_socket


def main():
    argv = len(sys.argv)
    if argv != 4:
        print("Usage: cli.py <listener port> <server ip> <server port>")
        sys.exit(1)
    
    listener_port = int(sys.argv[1])
    #server_ip = socket.gethostbyname(sys.argv[2])
    server_ip = sys.argv[2]
    server_port = int(sys.argv[3])
    
    print("listener_port: {}, server_ip: {}, server_port: {}".format(listener_port, server_ip, server_port) )
    server_socket = connect_server(server_ip, server_port)
    listen_socket = socket_listen(listener_port)
    
    inputs = [server_socket, listen_socket]
    app_fd = None
    
    while inputs:
        readable, writable, exceptional = select.select(inputs, [], inputs)

        for s in readable:

            if s is listen_socket:
                # A "readable" server socket is ready to accept a connection
                connection  = socket_accept(s)
                #print('new connection from: {}'.format(client_address))
                connection.setblocking(0)
                inputs.append(connection)
                app_fd = connection
                #app_fds.append(connection)
                # Give the connection a queue for data we want to send
                #message_queues[connection] = Queue.Queue()
            elif s is server_socket:
                if app_fd is None:
                    app_fd = connect_server("localhost", 22)
                    inputs.append(app_fd)
                                        
                data = s.recv(1024)
                if data:
                    # A readable client socket has data
                    print('received from {}'.format(s.getpeername()))
                    if app_fd is not None:
                        app_fd.send(data)
                    #message_queues[s].put(data)
                    # Add output channel for response
                    #if s not in outputs:
                    #    outputs.append(s)
                else:
                # Interpret empty result as closed connection
                    print("closing, client_address, after reading no data")
                # Stop listening for input on the connection
                    #if s in outputs:
                    #    outputs.remove(s)
                    inputs.remove(s)
                    s.close()
                    # Remove message queue
                    #del message_queues[s]
                    # Handle "exceptional conditions"
            elif s is app_fd:
                data = s.recv(1024)
                if data:
                    # A readable client socket has data
                    print('received from {}'.format(s.getpeername()))
                    server_socket.send(data)
                    #message_queues[s].put(data)
                    # Add output channel for response
                    #if s not in outputs:
                    #    outputs.append(s)
                else:
                # Interpret empty result as closed connection
                    print("closing, client_address, after reading no data")
                # Stop listening for input on the connection
                    #if s in outputs:
                    #    outputs.remove(s)
                    inputs.remove(s)
                    s.close()
                    app_fd = None
                    # Remove message queue
                    #del message_queues[s]
                    # Handle "exceptional conditions"
        for s in exceptional:
            print('handling exceptional condition for'.format(s.getpeername()))
                # Stop listening for input on the connection
            inputs.remove(s)
            #if s in outputs:
                 #   outputs.remove(s)
            s.close()

            # Remove message queue
            #del message_queues[s]

if __name__ == '__main__':
    main()


