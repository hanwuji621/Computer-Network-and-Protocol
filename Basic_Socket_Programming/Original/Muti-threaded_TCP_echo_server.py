#!/usr/bin/python

'''
    This is basically the same as the previous TCP echo server, but using threads
    to handle multiple client connections
'''
 
import sys
from socket import *  
from thread import *
 
myHost = ''     # Symbolic name meaning all available interfaces
myPort = 8000   # Arbitrary non-privileged port
 
sockobj = socket(AF_INET, SOCK_STREAM)
print 'Socket created'
 
#Bind socket to local host and port
try:
    sockobj.bind((myHost, myPort))
except socket.error as msg:
    print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
    sys.exit()
     
print 'Socket bind complete'
 
#Start listening on socket
sockobj.listen(10)
print 'Socket now listening'
 
# Function for multi-threaded handling of connections
def clientthread(conn):
         
    # Keep reading from client connection
    while True:
         
        # Read message from client
        data = conn.recv(1024)
        reply = 'Echoing...' + data
        if not data: 
            break
     
        conn.sendall(reply)
     
    conn.close()
 
# Keep accepting new connections
while True:
    conn, addr = sockobj.accept()
    print 'Connected with ' + addr[0] + ':' + str(addr[1])
     
    # Spawn new thread and pass it the new socket object 
    start_new_thread(clientthread ,(conn,))
 
sockobj.close()