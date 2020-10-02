#!/usr/bin/python

"""
Basic TCP echo client. Sends a default message string to data to the server, 
and print server's echo. User can also specify an IP and message string to
sent to the server. 

NOTE: Specify commnadline arguments as follows:

./echo-client.py [host]
or
./echo-client.py [host] [message]

"""

import sys
from socket import *              
serverHost = 'localhost'        # Default IP to connect to
serverPort = 8000               # Default port number

message = [b'Hello TCP World']  # Default text (ASCII) message
                                # requires bytes: b'' to convert to byte literal
if len(sys.argv) == 2:       
    serverHost = sys.argv[1]    # User has provided a server IP at cmd line arg 1
    if len(sys.argv) > 2:       # User-defined message from cmd line args 2
        message = (x.encode() for x in sys.argv[2:])  

sockobj = socket(AF_INET, SOCK_STREAM)      # Create a TCP socket object
sockobj.connect((serverHost, serverPort))   # connect to server IP + port

for line in message:
    sockobj.send(line)                      # send user message 
    data = sockobj.recv(1024)               # read server response
    print('Received From Server:', data)    

sockobj.close()