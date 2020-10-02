#!/usr/bin/python

"""
Basic TCP echo server. Listen for a client conenction, reads the message 
and then echoes the message back to the client. 

"""

from socket import *                        
myHost = ''                                 # '' to set the default IP to localhost
myPort = 8000                               # Default port number

sockobj = socket(AF_INET, SOCK_STREAM)      # Create a TCP socket object
sockobj.bind((myHost, myPort))              # bind it to server port number
sockobj.listen(5)                           

while True:                                 # listen until process killed
    connection, address = sockobj.accept()  
    print('Client Connection:', address)    # Print the connected client address
    while True:
        data = connection.recv(1024)        # read the client message
        if not data: break                  # Echo it back
        connection.send(b'Echo => ' + data)   # send data using "b" to format string as byte literal (ASCII)
    connection.close()