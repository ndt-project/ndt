#!/usr/bin/python

def sendMsg(conn, msg):
    msglen = str(len(msg))
    conn.sendall(msglen + '|'+msg)

def recvMsg(conn):
    msglentxt = ''
    data = conn.recv(1)
    while data != '|':
        msglentxt += data
        data = conn.recv(1)
        if len(data) != 1:
            return ''
    msglen = int(msglentxt)
    read = 0
    toReturn = ''
    while read != msglen:
        data = conn.recv(msglen-read)
        if len(data) == 0:
            return ''
        read += len(data)
        toReturn += data
    return toReturn
