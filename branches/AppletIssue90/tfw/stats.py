#!/usr/bin/python

import time

class SpeedStats:
    def __init__(self):
        self._currentBin = 0
        self._lastUpdate = 0
        self._bins = [0, 0, 0]

    def getSpeed(self):
        now = int(time.time())
        if now - self._lastUpdate == 0:
            pass
        elif now - self._lastUpdate == 1:
            self._currentBin = (self._currentBin + 1) % 3
            self._bins[self._currentBin] = 0
        elif now - self._lastUpdate == 2:
            self._currentBin = (self._currentBin + 1) % 3
            self._bins[self._currentBin] = 0
            self._currentBin = (self._currentBin + 1) % 3
            self._bins[self._currentBin] = 0
        else:
            self._bins = [0, 0, 0]
        self._lastUpdate = now
        return (self._bins[0] + self._bins[1] + self._bins[2]) / 384.0

    def addData(self, dataLen):
        now = int(time.time())
        if now - self._lastUpdate == 0:
            self._bins[self._currentBin] += dataLen
        elif now - self._lastUpdate == 1:
            self._currentBin = (self._currentBin + 1) % 3
            self._bins[self._currentBin] = dataLen
        elif now - self._lastUpdate == 2:
            self._currentBin = (self._currentBin + 1) % 3
            self._bins[self._currentBin] = 0
            self._currentBin = (self._currentBin + 1) % 3
            self._bins[self._currentBin] = dataLen
        else:
            self._bins = [0, 0, 0]
            self._bins[self._currentBin] = dataLen
        self._lastUpdate = now

