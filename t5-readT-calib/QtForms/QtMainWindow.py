# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'QtMainWindow.ui'
#
# Created: Fri Feb  3 12:29:15 2017
#      by: PyQt4 UI code generator 4.10.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_QtMainWindow(object):
    def setupUi(self, QtMainWindow):
        QtMainWindow.setObjectName(_fromUtf8("QtMainWindow"))
        QtMainWindow.resize(1024, 800)
        self.centralwidget = QtGui.QWidget(QtMainWindow)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.mdiArea = QtGui.QMdiArea(self.centralwidget)
        self.mdiArea.setGeometry(QtCore.QRect(0, 0, 1031, 751))
        self.mdiArea.setObjectName(_fromUtf8("mdiArea"))
        QtMainWindow.setCentralWidget(self.centralwidget)
        self.menubar = QtGui.QMenuBar(QtMainWindow)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 1024, 27))
        self.menubar.setObjectName(_fromUtf8("menubar"))
        QtMainWindow.setMenuBar(self.menubar)
        self.statusbar = QtGui.QStatusBar(QtMainWindow)
        self.statusbar.setObjectName(_fromUtf8("statusbar"))
        QtMainWindow.setStatusBar(self.statusbar)
        self.action_Tubotron = QtGui.QAction(QtMainWindow)
        self.action_Tubotron.setCheckable(True)
        self.action_Tubotron.setObjectName(_fromUtf8("action_Tubotron"))
        self.action_Temperature = QtGui.QAction(QtMainWindow)
        self.action_Temperature.setCheckable(True)
        self.action_Temperature.setObjectName(_fromUtf8("action_Temperature"))
        self.action_Utilization = QtGui.QAction(QtMainWindow)
        self.action_Utilization.setCheckable(True)
        self.action_Utilization.setObjectName(_fromUtf8("action_Utilization"))
        self.action_Frequency = QtGui.QAction(QtMainWindow)
        self.action_Frequency.setCheckable(True)
        self.action_Frequency.setObjectName(_fromUtf8("action_Frequency"))
        self.action_Quit = QtGui.QAction(QtMainWindow)
        self.action_Quit.setObjectName(_fromUtf8("action_Quit"))
        self.action_SaveData = QtGui.QAction(QtMainWindow)
        self.action_SaveData.setCheckable(True)
        self.action_SaveData.setObjectName(_fromUtf8("action_SaveData"))
        self.action_SelectBoard = QtGui.QAction(QtMainWindow)
        self.action_SelectBoard.setObjectName(_fromUtf8("action_SelectBoard"))
        self.action_CoreSwitcher = QtGui.QAction(QtMainWindow)
        self.action_CoreSwitcher.setCheckable(True)
        self.action_CoreSwitcher.setObjectName(_fromUtf8("action_CoreSwitcher"))
        self.actionRecord_Data = QtGui.QAction(QtMainWindow)
        self.actionRecord_Data.setObjectName(_fromUtf8("actionRecord_Data"))

        self.retranslateUi(QtMainWindow)
        QtCore.QMetaObject.connectSlotsByName(QtMainWindow)

    def retranslateUi(self, QtMainWindow):
        QtMainWindow.setWindowTitle(_translate("QtMainWindow", "SpiNNaker Temperature Profiler", None))
        self.action_Tubotron.setText(_translate("QtMainWindow", "&Tubotron", None))
        self.action_Tubotron.setShortcut(_translate("QtMainWindow", "F1", None))
        self.action_Temperature.setText(_translate("QtMainWindow", "Tem&perature", None))
        self.action_Temperature.setShortcut(_translate("QtMainWindow", "F2", None))
        self.action_Utilization.setText(_translate("QtMainWindow", "&Utilization", None))
        self.action_Utilization.setShortcut(_translate("QtMainWindow", "F3", None))
        self.action_Frequency.setText(_translate("QtMainWindow", "&Frequency", None))
        self.action_Frequency.setShortcut(_translate("QtMainWindow", "F4", None))
        self.action_Quit.setText(_translate("QtMainWindow", "&Quit", None))
        self.action_SaveData.setText(_translate("QtMainWindow", "&Save Data", None))
        self.action_SelectBoard.setText(_translate("QtMainWindow", "Select &Board", None))
        self.action_SelectBoard.setShortcut(_translate("QtMainWindow", "F6", None))
        self.action_CoreSwitcher.setText(_translate("QtMainWindow", "&Core Switcher", None))
        self.action_CoreSwitcher.setShortcut(_translate("QtMainWindow", "F5", None))
        self.actionRecord_Data.setText(_translate("QtMainWindow", "Record Data", None))

