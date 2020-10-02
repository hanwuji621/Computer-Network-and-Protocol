#!/usr/bin/python

from socket import * 

port = 8000
sockobj = socket(AF_INET, SOCK_DGRAM)
sockobj.bind(("", port))
print "waiting on port:", port
while 1:
    data, addr = sockobj.recvfrom(1024)
    print "Received: ", data, "From: ", addr