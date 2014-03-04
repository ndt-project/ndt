#!/usr/bin/python

import wx

class Traffic:
    def __init__(self, ctrl, name):
        self._ctrl = ctrl
        self._name = name
        self._client = None
        self._throughput = 100

    def setName(self, name):
        self._name = name

    def getName(self):
        return self._name

    def removeMe(self, event):
        self._ctrl.removeNode()
        self._parent.Dismiss()

    def getPanel(self, parent):
        panel = wx.Panel(parent, -1)
        a = wx.Button(panel, -1, "Remove me", (1, 1))
        self._parent = parent
        wx.EVT_BUTTON(panel, a.GetId(), self.removeMe)
        sz = a.GetBestSize()
        panel.SetSize( (sz.width, sz.height) )
        return panel

    def assign(self, event):
        self._client = event.GetClientData()

    def getInfoPanel(self, parent):
        wx.StaticText(parent, -1, "Traffic ", wx.Point(5, 5))
        wx.StaticText(parent, -1, "   Name: " + self.getName(), wx.Point(5, 20))
        wx.StaticText(parent, -1, "   Throughput: ", wx.Point(5, 35))
        wx.StaticText(parent, -1, "   Target: ", wx.Point(5, 64))
        tID = wx.NewId()
        choice = wx.Choice(parent, tID, wx.Point(100, 60), size = wx.Size(105, 22), choices = [])
        wx.EVT_CHOICE(parent, tID, self.assign)
        id = choice.Append(" <None> ")
        choice.SetClientData(id, None)
        choice.SetSelection(id)
        for item in self._ctrl.getClients():
            hostAssigned = self._ctrl.tree.GetPyData(self._ctrl.tree.GetItemParent(self._ctrl.findTreeNode(self))).getAssignedClient()
            if hostAssigned == item[0]:
                continue
            id = choice.Append(str(item[1][1]))
            choice.SetClientData(id, item[0])
            if self._client == item[0]:
                choice.SetSelection(id)
        tID = wx.NewId()
        text = wx.TextCtrl(parent, tID, str(self._throughput), wx.Point(100, 31), (105, -1))
        wx.EVT_TEXT(parent, text.GetId(), self.EvtText)
        wx.StaticText(parent, -1, "kb/s", wx.Point(210, 35))

    def getAssignedClient(self):
        return self._client

    def isValidClientAssigned(self):
        for item in self._ctrl.getClients():
            if self._client == item[0]:
                return True
        return False

    def EvtText(self, event):
        self._throughput = int(event.GetString())

    def getThroughput(self):
        return self._throughput
