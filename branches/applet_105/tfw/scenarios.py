#!/usr/bin/python

import wx

class Scenario:
    def __init__(self, ctrl, name):
        self._ctrl = ctrl
        self._name = name

    def setName(self, name):
        self._name = name

    def getName(self):
        return self._name

    def addHost(self, event):
        self._ctrl.addHost()
        self._parent.Dismiss()

    def startScenario(self, event):
        self._ctrl.startScenario()

    def stopScenario(self, event):
        self._ctrl.stopScenario()

    def getPanel(self, parent):
        panel = wx.Panel(parent, -1)
        a = wx.Button(panel, -1, "Add host", (1, 1))
        self._parent = parent
        wx.EVT_BUTTON(panel, a.GetId(), self.addHost)
        sz = a.GetBestSize()
        panel.SetSize( (sz.width, sz.height) )
        return panel

    def getInfoPanel(self, parent):
        wx.StaticText(parent, -1, "Scenario", wx.Point(5,5))
        wx.StaticText(parent, -1, "   Name: " + self.getName(), wx.Point(5,20))
        if self._ctrl.getNotAssignedHostsAmount() > 0:
            wx.StaticText(parent, -1, "   Hosts: " + str(self._ctrl.getHostsAmount()) +
                    " (" + str(self._ctrl.getNotAssignedHostsAmount()) + " not assigned)", wx.Point(5,35))
        else:
            wx.StaticText(parent, -1, "   Hosts: " + str(self._ctrl.getHostsAmount()), wx.Point(5,35))
        a = wx.Button(parent, -1, "Start", (5, 100))
        wx.EVT_BUTTON(parent, a.GetId(), self.startScenario)
        b = wx.Button(parent, -1, "Stop", (130, 100))
        wx.EVT_BUTTON(parent, b.GetId(), self.stopScenario)
