#!/usr/bin/python

from socket import * 

port = 8000
host = "localhost"
sockobj = socket(AF_INET, SOCK_DGRAM)
sockobj.bind(("", 0))
sockobj.sendto("We have Ingition!", (host, port))