import os
import re
import socket
import sys
from threading import Thread
import time

PORT = 3333


def tcp_client(address, payload):
    for res in socket.getaddrinfo(address, PORT, socket.AF_UNSPEC,
                                  socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
        family_addr, socktype, proto, canonname, addr = res
    try:
        sock = socket.socket(family_addr, socket.SOCK_STREAM)
        sock.settimeout(60.0)
    except socket.error as msg:
        print('Could not create socket: ' + str(msg[0]) + ': ' + msg[1])
        raise
    try:
        sock.connect(addr)
    except socket.error as msg:
        print('Could not open socket: ', msg)
        sock.close()
        raise
    for i in range(100):
        payload += str(i)
        sock.sendall(payload.encode())
        data = sock.recv(1024)
        if not data:
            return
        print('Reply : ' + data.decode())
        time.sleep(0.01)
    return sock
    sock.close()
    return data.decode()

if __name__ == '__main__':
    # Usage: example_test.py <server_address> <message_to_send_to_server>
    # tcp_client(sys.argv[1], sys.argv[2])
    threads = []
    socks = []
    for n in range(10):
        # t = Thread(target=tcp_client, args=(sys.argv[1],100*sys.argv[2]))
        s = tcp_client(sys.argv[1], sys.argv[2])
        socks.append(s)
        # t.start()
        # threads.append(t)
    # for t in threads:
        # t.join()
    for s in socks:
        s.close()
        time.sleep(0.01)
