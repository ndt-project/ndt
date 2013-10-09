#!/usr/bin/python

import wx

class Host:
    def __init__(self, ctrl, name):
        self._ctrl = ctrl
        self._name = name
        self._client = None

    def setName(self, name):
        self._name = name

    def getName(self):
        return self._name

    def removeMe(self, event):
        self._ctrl.removeNode()
        self._parent.Dismiss()

    def addTraffic(self, event):
        self._ctrl.addTraffic()
        self._parent.Dismiss()

    def getPanel(self, parent):
        panel = wx.Panel(parent, -1)
        a = wx.Button(panel, -1, "Add traffic", (1, 1))
        self._parent = parent
        wx.EVT_BUTTON(panel, a.GetId(), self.addTraffic)
        sz = a.GetBestSize()
        b = wx.Button(panel, -1, "Remove me", (1, sz.height))
        wx.EVT_BUTTON(panel, b.GetId(), self.removeMe)
        bsz = b.GetBestSize()
        panel.SetSize( (max(sz.width, bsz.width), sz.height + bsz.height) )
        return panel

    def assign(self, event):
        self._client = event.GetClientData()

    def getInfoPanel(self, parent):
        wx.StaticText(parent, -1, "Host ", wx.Point(5,5))
        wx.StaticText(parent, -1, "   Name: " + self.getName(), wx.Point(5,20))
        wx.StaticText(parent, -1, "   Assigned to: ", wx.Point(5,39))
        tID = wx.NewId()
        choice = wx.Choice(parent, tID, wx.Point(100, 35), size = wx.Size(105, 22), choices = [])
        wx.EVT_CHOICE(parent, tID, self.assign)
        id = choice.Append(" <None> ")
        choice.SetClientData(id, None)
        choice.SetSelection(id)
        for item in self._ctrl.getClients():
            id = choice.Append(str(item[1][1]))
            choice.SetClientData(id, item[0])
            if self._client == item[0]:
                choice.SetSelection(id)

    def getAssignedClient(self):
        return self._client

    def isValidClientAssigned(self):
        for item in self._ctrl.getClients():
            if self._client == item[0]:
                return True
        return False
