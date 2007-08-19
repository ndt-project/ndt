#!/usr/bin/python

import wx
import time

class Popup(wx.PopupTransientWindow):
    def __init__(self, parent, style, ctrl, item):
        wx.PopupTransientWindow.__init__(self, parent, style)
        itemData = ctrl.tree.GetPyData(item)
        if itemData is not None:
            panel = itemData.getPanel(self)
            self.SetSize(panel.GetSize())
        else:
            self.Destroy()

    def ProcessLeftDown(self, evt):
        print "ProcessLeftDown\n"
        return False

class MyLog(wx.PyLog):
    def __init__(self, textCtrl, logTime=0):
        wx.PyLog.__init__(self)
        self.tc = textCtrl
        self.logTime = logTime

    def DoLogString(self, message, timeStamp):
        if self.logTime:
            message = time.strftime("%X", time.localtime(timeStamp)) + \
                      ": " + message
        if self.tc:
            self.tc.AppendText(message + '\n')
