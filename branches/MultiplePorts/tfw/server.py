#!/usr/bin/python

import network
import threading
import re
import time
import wx
import sys
import os

from scenarios import Scenario
from hosts import Host
from widgets import *
from traffics import Traffic
from communication import *

def serve1cli(ctrl, conn, addr):
    ctrl.write('accepted: ' + str(addr))
    id = ctrl.addClient(conn, addr)
    data = conn.recv(1)
    try:
        if data == "C":
            ctrl.enableClient(id)
            while 1:
                msg = recvMsg(conn)
                if len(msg) > 0:
                    print msg
                    if msg.startswith('OPENED:'):
                        list = msg[7:].split('/')
                        ctrl.startTraffic(id, list[0], list[1])
                else:
                    break
    except:
        print "Unexpected error:", sys.exc_info()[0]
        raise
    ctrl.removeClient(id)
    ctrl.write('closed: ' + str(addr))
    try:
        conn.close()
    except:
        pass

def listen4cli(ctrl):
    s = network.listen(None, "50111")
    ctrl.write('Listening on ' + str(s.getsockname()))
    
    while 1:
        ctrl.write('Accepting...');
        conn, addr = s.accept()
        # Start working thread
        thread = threading.Thread(target=serve1cli, args=(ctrl, conn, addr))
        thread.start()

class TFW(wx.Frame):
    def __init__(self, parent, id, title):
        wx.Frame.__init__(self, parent, -1, title, size = (800, 600))

        # Private variables
        self._scenarioInProgress = False
        self._currId = 0
        self._clients = {}
        self._statuses = ['idle', 'running']
        self._status = 0

        # Screen & status bar
        self.CenterOnScreen()

        self.CreateStatusBar()
        self.statusBar(0)

        # Menu
        menuBar = wx.MenuBar()
        menu1 = wx.Menu()
        menu1.Append(104, "&Exit", "Exit the TFW")
        menuBar.Append(menu1, "&File")

        self.SetMenuBar(menuBar)

        wx.EVT_MENU(self, 104, self.CloseWindow)

        # Notebook
        self.nb = wx.Notebook(self,-1)

        # Scenario page
        splitter = wx.SplitterWindow(self.nb, -1)

        panel = wx.Panel(splitter, -1, style=wx.CLIP_CHILDREN)
        tID = wx.NewId()
        self.tree = wx.TreeCtrl(panel, tID, wx.DefaultPosition, wx.DefaultSize,
                wx.TR_HAS_BUTTONS | wx.TR_EDIT_LABELS)

        isz = (16,16)
        il = wx.ImageList(isz[0], isz[1])
        self.fldridx     = il.Add(wx.ArtProvider_GetBitmap(wx.ART_FOLDER,      wx.ART_OTHER, isz))
        self.fldropenidx = il.Add(wx.ArtProvider_GetBitmap(wx.ART_FILE_OPEN,   wx.ART_OTHER, isz))
        self.fileidx     = il.Add(wx.ArtProvider_GetBitmap(wx.ART_REPORT_VIEW, wx.ART_OTHER, isz))

        self.tree.SetImageList(il)
        self.il = il
        self.createNewScenario()

        self.tree.Expand(self.root)

        def EmptyHandler(evt): pass

        def OnSelChanged(event):
            self.selectionPanel()
            event.Skip()

        def OnTreeOvrSize(evt, ovr=self.tree):
            ovr.SetSize(evt.GetSize())

        def OnRightClick(event):
            pt = event.GetPosition();
            item, flags = self.tree.HitTest(pt)
            self.tree.SelectItem(item)
            win = Popup(self.tree, wx.SIMPLE_BORDER, self, item)
            btn = self.tree.GetBoundingRect(item, False)
            if btn is not None:
                pos = self.tree.ClientToScreen(btn.GetPosition())
                sz =  btn.GetSize()
                win.Position(pos, (0, sz.height))
                win.Popup()

        def OnLeftDClick(event):
            pt = event.GetPosition();
            item, flags = self.tree.HitTest(pt)
            self.tree.EditLabel(item)

        def OnEndEdit(event):
            selItem = self.tree.GetSelection()
            if selItem is not None:
                self.tree.GetPyData(selItem).setName(self.tree.GetItemText(selItem))
            self.selectionPanel()

        wx.EVT_SIZE(panel, OnTreeOvrSize)
        wx.EVT_TREE_SEL_CHANGED(self, tID, OnSelChanged)
        wx.EVT_TREE_END_LABEL_EDIT (self, tID, OnEndEdit)
        wx.EVT_LEFT_DCLICK(self.tree, OnLeftDClick)
        wx.EVT_RIGHT_DOWN(self.tree, OnRightClick)

        self._panel2 = wx.Panel(splitter, -1, style=wx.CLIP_CHILDREN)
        self.selectionPanel()

        splitter.SetMinimumPaneSize(150)
        splitter.SplitVertically(panel, self._panel2, 150)

        self.nb.AddPage(splitter, "Scenario")

        # Clients page
        panel = wx.Panel(self.nb, -1, style=wx.CLIP_CHILDREN)
        self.txt = wx.StaticText(panel, -1, "Connected clients:", wx.Point(5, 5), wx.Size(200, -1))
        tID = wx.NewId()
        self.list = wx.ListCtrl(panel, tID, pos=wx.Point(5, 22), style=wx.LC_REPORT|wx.SUNKEN_BORDER)

        def OnOvrSize(evt, ovr=self.list):
            ovr.SetSize(evt.GetSize())

        wx.EVT_SIZE(panel, OnOvrSize)
        wx.EVT_ERASE_BACKGROUND(panel, EmptyHandler)

        self.populateList();

        self.nb.AddPage(panel, "Clients")

        # Logs page
        panel = wx.Panel(self.nb, -1, style=wx.CLIP_CHILDREN)
        self.log = wx.TextCtrl(panel, -1, style = wx.TE_MULTILINE|wx.TE_READONLY|wx.HSCROLL)

        def OnLogOvrSize(evt, ovr=self.log):
            ovr.SetSize(evt.GetSize())

        wx.EVT_SIZE(panel, OnLogOvrSize)
        wx.EVT_ERASE_BACKGROUND(panel, EmptyHandler)

        self.nb.AddPage(panel, "Logs")
        wx.Log_SetActiveTarget(MyLog(self.log, True))

        wx.EVT_CLOSE(self, self.OnCloseWindow)

        # Start listening thread
        thread = threading.Thread(target=listen4cli, args=([self]))
        thread.start()

    def statusBar(self, status):
        self.SetStatusText("Status: " + self._statuses[status] + " Clients: " + str(len(self._clients)))

    def updateStatus(self):
        self.statusBar(self._status)

    def selectionPanel(self):
        self._panel2.DestroyChildren()
        selItem = self.tree.GetSelection()
        if selItem is not None:
            if self.tree.GetPyData(selItem) is not None:
                self.tree.GetPyData(selItem).getInfoPanel(self._panel2)

    def CloseWindow(self, event):
        self.Close()

    def OnCloseWindow(self, event):
        self.Destroy()
        os._exit(0)

    def WriteText(self, text):
        if text[-1:] == '\n':
            text = text[:-1]
        wx.LogMessage(text)

    def write(self, txt):
        self.WriteText(txt)

    def addClient(self, conn, addr):
        self._clients[self._currId] = (conn, addr, 'logging...')
        self.updateList()
        self._currId += 1
        self.updateStatus()
        return self._currId - 1

    def getClient(self, id):
        return self._clients[id]

    def getClients(self):
        return self._clients.items()

    def enableClient(self, id):
        self._clients[id] = (self._clients.get(id)[0], self._clients.get(id)[1], 'OK')
        self.updateList()
#        self.selectionPanel()

    def removeClient(self, id):
        self._clients.pop(id)
        self.updateList()
        self.updateStatus()
#        self.selectionPanel()

    def updateList(self):
        self.list.ClearAll()
        wx.CallAfter(self.populateList)

    def populateList(self):
        self.list.InsertColumn(0, "IP")
        self.list.InsertColumn(1, "Port")
        self.list.InsertColumn(2, "Status")

        ind = 0
        for x in self._clients.keys():
            conn, addr, stat = self._clients[x]
            self.list.InsertStringItem(ind, addr[0])
            self.list.SetStringItem(ind, 1, str(addr[1]))
            self.list.SetStringItem(ind, 2, stat)
            self.list.SetItemData(ind, x)
            ind += 1

        if ind > 0:
            self.list.SetColumnWidth(0, wx.LIST_AUTOSIZE)
            self.list.SetColumnWidth(1, wx.LIST_AUTOSIZE)
            self.list.SetColumnWidth(2, wx.LIST_AUTOSIZE)

    def createNewScenario(self):
        scenario = Scenario(self, "New Scenario")
        self.root = self.tree.AddRoot(scenario.getName())
        self.tree.SetPyData(self.root, scenario)
        self.tree.SetItemImage(self.root, self.fldridx, wx.TreeItemIcon_Normal)
        self.tree.SetItemImage(self.root, self.fldropenidx, wx.TreeItemIcon_Expanded)

    def addHost(self):
        host = Host(self, "New Host")
        newItem = self.tree.AppendItem(self.root, host.getName())
        self.tree.SetPyData(newItem, host)
        self.tree.SetItemImage(newItem, self.fldridx, wx.TreeItemIcon_Normal)
        self.tree.SetItemImage(newItem, self.fldropenidx, wx.TreeItemIcon_Expanded)
        self.selectionPanel()

    def addTraffic(self):
        selItem = self.tree.GetSelection()
        traffic = Traffic(self, "Traffic")
        newItem = self.tree.AppendItem(selItem, traffic.getName())
        self.tree.SetPyData(newItem, traffic)
        self.tree.SetItemImage(newItem, self.fileidx, wx.TreeItemIcon_Normal)
        self.selectionPanel()

    def removeNode(self):
        selItem = self.tree.GetSelection()
        self.tree.Delete(selItem)
        self.selectionPanel()

    def findTreeNode(self, data, node = None):
        if node is None:
            node = self.root
        child, cookie = self.tree.GetFirstChild(node)
        while child.IsOk():
            if self.tree.GetPyData(child) == data:
                return child
            found = self.findTreeNode(data, child)
            if found is not None:
                return found
            child, cookie = self.tree.GetNextChild(node, cookie)
        return None

    def applyOnAllNodes(self, func, acc, node = None):
        if node is None:
            node = self.root
        res = None
        child, cookie = self.tree.GetFirstChild(node)
        while child.IsOk():
            res = acc(res, func(self.tree.GetPyData(child)))
            res = acc(res, self.applyOnAllNodes(func, acc, child))
            child, cookie = self.tree.GetNextChild(node, cookie)
        return res

    def startScenario(self):
        print 'startScenario'
        host, cookie = self.tree.GetFirstChild(self.root)
        while host.IsOk():
            asHost = self.tree.GetPyData(host)
            if asHost.isValidClientAssigned():
                print 'Valid Client Assigned!'
                traffic, tcook = self.tree.GetFirstChild(host)
                while traffic.IsOk():
                    asTraffic = self.tree.GetPyData(traffic)
                    if asTraffic.isValidClientAssigned():
                        print '  Valid Client Assigned!'
                        sendMsg(self.getClient(asTraffic.getAssignedClient())[0], 'OPEN')
                    else:
                        print '  Warning: valid client not assigned to traffic: %s!' % (asTraffic.getName())
                    traffic, tcook = self.tree.GetNextChild(host, tcook)
            else:
                print 'Warning: valid client not assigned to host: %s!' % (asHost.getName())
            host, cookie = self.tree.GetNextChild(self.root, cookie)
        self.statusBar(1)

    def startTraffic(self, id, hostname, port):
        print 'startTraffic(' + str(id) + ', ' + hostname + ', ' + port + ')'
        host, cookie = self.tree.GetFirstChild(self.root)
        while host.IsOk():
            asHost = self.tree.GetPyData(host)
            if asHost.isValidClientAssigned():
                traffic, tcook = self.tree.GetFirstChild(host)
                while traffic.IsOk():
                    asTraffic = self.tree.GetPyData(traffic)
                    if asTraffic.isValidClientAssigned():
                        if asTraffic.getAssignedClient() == id:
                            sendMsg(self.getClient(asHost.getAssignedClient())[0],
                                    'TRAFFIC/'+hostname+'/'+port+'/'+str(asTraffic.getThroughput()))
                    traffic, tcook = self.tree.GetNextChild(host, tcook)
            else:
                print 'Warning: valid client not assigned to host: %s!' % (asHost.getName())
            host, cookie = self.tree.GetNextChild(self.root, cookie)

    def stopScenario(self):
        print 'stopScenario'
        host, cookie = self.tree.GetFirstChild(self.root)
        while host.IsOk():
            asHost = self.tree.GetPyData(host)
            if asHost.isValidClientAssigned():
                sendMsg(self.getClient(asHost.getAssignedClient())[0], 'STOP')
                traffic, tcook = self.tree.GetFirstChild(host)
                while traffic.IsOk():
                    asTraffic = self.tree.GetPyData(traffic)
                    if asTraffic.isValidClientAssigned():
                        sendMsg(self.getClient(asTraffic.getAssignedClient())[0], 'STOP')
                    traffic, tcook = self.tree.GetNextChild(host, tcook)
            else:
                print 'Warning: valid client not assigned to host: %s!' % (asHost.getName())
            host, cookie = self.tree.GetNextChild(self.root, cookie)
        self.statusBar(0)

    def getHostsAmount(self):
        return self.tree.GetChildrenCount(self.root, False)

    def getNotAssignedHostsAmount(self):
        child, cookie = self.tree.GetFirstChild(self.root)
        count = 0
        while child.IsOk():
            if not self.tree.GetPyData(child).isValidClientAssigned():
                count += 1
            child, cookie = self.tree.GetNextChild(self.root, cookie)
        return count


if __name__ == '__main__':
    app = wx.PySimpleApp()
    frame = TFW(None, -1, " NDT TFW")
    frame.Show()
    app.MainLoop()
