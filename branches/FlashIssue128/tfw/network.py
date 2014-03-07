#network functions

import socket
import sys

# listen
def listen(host, port):
	s = None
	for res in socket.getaddrinfo(host, port, socket.AF_UNSPEC, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
	    af, socktype, proto, canonname, sa = res
	    try:
		s = socket.socket(af, socktype, proto)
	    except socket.error, msg:
		s = None
		continue
	    try:
		s.bind(sa)
		s.listen(1)
	    except socket.error, msg:
		s.close()
		s = None
		continue
	    break
	if s is None:
	    print 'could not open socket'
	    sys.exit(1)
	return s

# connect
def connect(host, port):
	s = None
	for res in socket.getaddrinfo(host, port, socket.AF_UNSPEC, socket.SOCK_STREAM):
	    af, socktype, proto, canonname, sa = res
	    try:
		s = socket.socket(af, socktype, proto)
	    except socket.error, msg:
		s = None
		continue
	    try:
		s.connect(sa)
	    except socket.error, msg:
		s.close()
		s = None
		continue
	    break
	if s is None:
	    print 'could not open socket'
	    sys.exit(1)
	return s
