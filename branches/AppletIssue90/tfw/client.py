#!/usr/bin/python

import network
import threading
import sys
import time
from communication import *
from stats import SpeedStats


class TFWclient:
    def __init__(self, host, port, paddr):
        self._host = host
        self._port = port
        self._paddr = paddr
        self._working = 1
        self._threads = []

    def connect(self):
        print "Connecting to", self._host, self._port
        self._sock = network.connect(self._host, self._port)
        self._sock.setblocking(True)
        self._sock.settimeout(None)
        print "   >>> Done"

    def logIn(self):
        print "Logging in..."
        self._sock.sendall("C")
        print "   >>> Done"

    def operate(self):
        while 1:
            msg = recvMsg(self._sock)
            if len(msg) > 0:
                print '>>>', msg
                if msg == 'OPEN':
                    print 'opening...'
                    s = network.listen('::', None)
                    sendMsg(self._sock, str('OPENED:'+self._paddr+'/'+str(s.getsockname()[1])))
                    print 'accepting... [' + self._paddr + ':' + str(s.getsockname()[1]) + ']'
                    conn, addr = s.accept()
                    print 'starting...'
                    # Start garbage thread
                    self._working = 1
                    thread = threading.Thread(target=self.garbageListener, args=(conn, addr))
                    self._threads.append(thread)
                    thread.start()
                elif msg.startswith('TRAFFIC/'):
                    list = msg[8:].split('/')
                    thost = list[0]
                    tport = list[1]
                    tspeed = list[2]
                    print 'host:', thost, 'port:', tport, 'speed:', tspeed
                    # Start worker thread
                    self._working = 1
                    thread = threading.Thread(target=self.worker, args=(thost, tport, tspeed))
                    self._threads.append(thread)
                    thread.start()
                elif msg == 'STOP':
                    print 'stopping...'
                    self._working = 0
                    for thread in self._threads:
                        thread.join()
                        self._threads.remove(thread)
                    print '...done!'
                else:
                    print "Unrecognized command"
                    self._working = 0
                    sys.exit()
            else:
                print "Connection has been closed"
                self._working = 0
                sys.exit()

    def speedPrinter(self, addr1, addr2, stats):
        while self._working:
            time.sleep(1)
            print '%s %s: speed=%.2f kb/s' % (addr1, addr2, stats.getSpeed())

    def garbageListener(self, conn, addr):
        try:
            print '%s %s: reading...' % (addr[0], addr[1])
            stats = SpeedStats()
            thread = threading.Thread(target=self.speedPrinter, args=(addr[0], addr[1], stats))
            thread.start()
            while self._working:
                data = conn.recv(65536)
                if len(data) == 0:
                    break
                stats.addData(len(data))
        except Exception, p:
            print p
        print 'closing connection...'
        try:
            conn.close()
        except:
            pass

    def worker(self, host, port, speed):
        socket = network.connect(host, port)
    
        rspeed = float(speed)
        
        data = '1234567890-=qwertyuiopsdfghjkl;zxcvbnm,QWERTYUIOPASDFGHJLZXCVBNM'
        for i in range(10):
            data += data
        delay = 0.75 / (rspeed / 512.0)
        
        print 'delay:', delay, 'chunk:', len(data)
        
        while self._working:
            back = time.time()
            socket.sendall(data)
            back = time.time() - back
            if (delay - back) > 0.0:
                time.sleep(delay - back)

        print 'stop writing...'
        try:
            socket.close()
        except:
            pass


if __name__ == '__main__':
    if len(sys.argv) != 4:
        print "Usage:", sys.argv[0], "<host> <port> <publicaddr>"
        sys.exit()
    client = TFWclient(sys.argv[1], sys.argv[2], sys.argv[3])
    client.connect()
    client.logIn()
    client.operate()
