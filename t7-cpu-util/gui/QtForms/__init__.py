"""
OVERVIEW:
    This will handle both the main window that provides:
    - console-like view for raw data
    - plotter for the data
    - save data to a file

SYNOPSIS:
    This main window has two sub window: SpiNNaker widget (sw) and plotter.
    sw receives data from Arduino in SpiNN-4 board and emit it to MainWindow.
    MainWindow then processes the data to the plotter (tw).
    MainWindow also saves the data in a log file.
"""

from PyQt4 import QtGui, QtNetwork
from PyQt4.QtCore import *
import QtMainWindow
from arduSpiNN import ifaceArduSpiNN
from myPlotterP import Pwidget
from myPlotterS import Swidget
from tgvis import visWidget
import constDef as DEF # the calibration constants should be defined here
#from rig import machine_control
from rig.machine_control import MachineController
import time
import os
import struct
import math

class config():
    def __init__(self):
        self.xPos = 0
        self.xPosKey = "mainGUI/xPos"
        self.yPos = 0
        self.yPosKey = "mainGUI/yPos"
        self.width = 1000
        self.widthKey = "mainGUI/width"
        self.height = 800
        self.heightKey = "mainGUI/height"

"""===============================================================================================
                                             MainGUI
-----------------------------------------------------------------------------------------------"""
class MainWindow(QtGui.QMainWindow, QtMainWindow.Ui_QtMainWindow):
    """ It inherits QMainWindow and uses pyuic4-generated python files from the .ui """
    pltDataRdy = pyqtSignal(list)  # for streaming data to the plotter/calibrator

    def __init__(self, mIP=None, logFName=None, parent=None):
        super(MainWindow, self).__init__(parent)
        self.setupUi(self)
        self.setCentralWidget(self.mdiArea);
        self.statusTxt = QtGui.QLabel("")
        self.statusBar().addWidget(self.statusTxt)

        # load setting
        self.loadSettings()

        # if machine IP is provided, check if it is booted and/or if the profiler has been loaded
        if mIP is not None:
            self.mIP = mIP
            self.checkSpiNN4()
        else:
            self.mIP = None

        self.connect(self.actionShow_Chips, SIGNAL("triggered()"), SLOT("showChips()"))

        """
        Scenario: 
        - sw and pw send data to MainWindow
        - MainWindow do the averaging on sw data, then send avg_sw and pw to plotter
          At the same time, MainWindow store the data
        """

        #-------------------- GUI setup ---------------------               
        # Dump raw data in arduConsole widget (cw).
        self.cw = ifaceArduSpiNN(logFName,self)
        self.subWinConsole = QtGui.QMdiSubWindow(self)
        self.subWinConsole.setWidget(self.cw)
        self.mdiArea.addSubWindow(self.subWinConsole)
        self.subWinConsole.setGeometry(0,0,self.cw.width(),500)
        self.cw.show()

        # Power plot widget (pw).
        self.pw = Pwidget(self)
        self.subWinPW = QtGui.QMdiSubWindow(self)
        self.subWinPW.setWidget(self.pw)
        self.mdiArea.addSubWindow(self.subWinPW)
        self.subWinPW.setGeometry(self.cw.width()+10,0,760,500)
        self.pw.show()

        # SpiNNaker profiler plot widget (sw).
        self.sw = Swidget(self)
        self.subWinSW = QtGui.QMdiSubWindow(self)
        self.subWinSW.setWidget(self.sw)
        self.mdiArea.addSubWindow(self.subWinSW)
        self.subWinSW.setGeometry(self.subWinPW.x(), self.subWinPW.y()+self.subWinPW.height(),760,500)
        self.sw.show()

        # initially, we don't show chip visualizer
        self.vis = None

        # SIGNAL-SLOT connection
        self.cw.spinUpdate.connect(self.sw.readPltData)
        self.cw.arduUpdate.connect(self.pw.readPltData)
        # just for debugging:
        # self.cw.arduUpdate.connect(self.readArduData)
        # self.cw.spinUpdate.connect(self.readSpinData)

    def loadSettings(self):
        """
        Load configuration setting from file.
        Let's use INI format and UserScope
        :return:
        """
        #init value
        self.conf = config()

        #if used before, then load from previous one; otherwise, it use the initial value above
        self.settings = QSettings(QSettings.IniFormat, QSettings.UserScope, "APT", "SpiNN-4 Power Profiler")
        self.conf.xPos,ok = self.settings.value(self.conf.xPosKey, self.conf.xPos).toInt()
        self.conf.yPos,ok = self.settings.value(self.conf.yPosKey, self.conf.yPos).toInt()
        self.conf.width,ok = self.settings.value(self.conf.widthKey, self.conf.width).toInt()
        self.conf.height,ok = self.settings.value(self.conf.heightKey, self.conf.height).toInt()

        #print "isWritable?", self.settings.isWritable()
        #print "conf filename", self.settings.fileName()

        #then apply to self
        self.setGeometry(self.conf.xPos, self.conf.yPos, self.conf.width, self.conf.height)

    def checkSpiNN4(self, alwaysAsk=True):

        #return #go out immediately for experiment with Basab

        """
        if the MainWindow class is called with an IP, then check if the machine is ready
        """
        loadProfiler = False

        # first, check if the machine is booted
        print "Check the machine: ",
        self.mc = MachineController(self.mIP)
        bootByMe = self.mc.boot()
        # if bootByMe is true, then the machine is booted by readSpin4Pwr.py, otherwise it is aleady booted
        # if rig cannot boot the machine (eq. the machine is not connected or down), the you'll see error
        # message in the arduConsole
        if bootByMe is False:
            print "it is booted already!"
        else:
            print "it is now booted!"
            loadProfiler = True
            alwaysAsk = False       # because the machine is just booted, then force it load profiler

        # second, ask if profilers need to be loaded
        cpustat = self.mc.get_processor_status(1,0,0) #core-1 in chip <0,0>
        profName = cpustat.app_name
        profState = int(cpustat.cpu_state)
        profVersion = cpustat.user_vars[0] # read from sark.vcpu->user0
        #print "Profiler name: ", profName
        #print "Profiler state: ", profState
        #print "Profiler version: ", profVersion
        if profName.upper()==DEF.REQ_PROF_NAME.upper() and profState==7 and profVersion==DEF.REQ_PROF_VER:
            print "Required profiler.aplx is running!"
            loadProfiler = False
        else:
            if alwaysAsk:
                askQ = raw_input('Load the correct profiler.aplx? [Y/n]')
                if len(askQ) == 0 or askQ.upper() == 'Y':
                    loadProfiler = True
                else:
                    loadProfiler = False
                    print "[WARNING] profiler.aplx is not ready or invalid version"

            # third, load the profilers
            if loadProfiler:
                # then load the correct profiler version
                print "Loading profiler from", DEF.REQ_PROF_APLX
                # populate all chips in the board
                chips = self.mc.get_system_info().keys()
                dest = dict()
                for c in chips:
                    dest[c] = [1]   # the profiler.aplx goes to core-1
                self.mc.load_application(DEF.REQ_PROF_APLX, dest, app_id=DEF.REQ_PROF_APLX_ID)
                print "Profilers are sent to SpiNN4!"
            else:
                print "No valid profiler! This program might not work correctly!"

        # Last but important: MAKE SURE IPTAG 3 IS SET
        iptag = self.mc.iptag_get(DEF.REPORT_IPTAG,0,0)
        if iptag.addr is '0.0.0.0' or iptag.port is not DEF.REPORT_IPTAG:
            #iptag DEF.REPORT_IPTAG is not defined yet
            self.mc.iptag_set(DEF.REPORT_IPTAG, DEF.HOST_PC, DEF.RECV_PORT, 0, 0)

    @pyqtSlot()
    def reactiveShopCipMenu(self):
        self.actionShow_Chips.setEnabled(True)
        del self.subWinVis

    @pyqtSlot()
    def showChips(self):
        """
        Show chip layout
        :return: 
        """
        if self.mIP is None:
            """
            i.e., the IP is not defined when this menu is clicked
            """
            # first, ask the IP of the machine
            mIP, ok = QtGui.QInputDialog.getText(self, "SpiNN4 Location", "SpiNN4 IP address",
                                                 QtGui.QLineEdit.Normal, DEF.MACHINE)
            if ok is True:
                self.mIP = mIP
                self.checkSpiNN4(False)     # after this, self.mc is available

        # then open the widget
        mInfo = self.mc.get_system_info()
        w = mInfo.width
        h = mInfo.height
        chipList = mInfo.keys()
        #self.vis = visWidget(w, h, chipList) # Segmentation fault
        self.vis = visWidget(w, h, chipList, self)

        self.subWinVis = QtGui.QMdiSubWindow(self)
        self.subWinVis.setWidget(self.vis)
        self.mdiArea.addSubWindow(self.subWinVis)
        #self.subWinVis.setGeometry(self.subWinPW.x(), self.subWinPW.y()+self.subWinPW.height(),760,500)

        # connect to the source of temperature data
        # masih salah: self.cw.spinUpdate.connect(self.vis.readSpinData)

        self.vis.show()

        # and disable the menu item
        self.actionShow_Chips.setEnabled(False) # the menu will be reset to enable if the widget is closed
        self.vis.visWidgetTerminated.connect(self.reactiveShopCipMenu)

    """
    ######################### GUI callback ########################### 
    """

    # readSpinData and readArduData are just for debugging
    @pyqtSlot(list)
    def readSpinData(self, prfData):
        """
        Read profiler data and prepare for saving.
        prfData is a list of 48 "Integer" item that should be decoded as:
        freq, nActive, temp
        Hence, fmt = "<2BH"
        """
        #print len(prfData)
        #print type(prfData[0])
        #return

        fmt = "<2BH"
        #fmt = "<H2B"
        print "{",
        for i in range(len(prfData)):
            cpu = struct.pack("<I",prfData[i])
            #print "0x%x " % cpu,

            #f,nA,T = struct.unpack(fmt, prfData[i])
            f,nA,T = struct.unpack(fmt, cpu)
            #print "[{},{},{}]".format(f,nA,T),
            if i < (len(prfData)-1):
                print "[{},{},{}],".format(f, nA, T),
            else:
                print "[{},{},{}]".format(f, nA, T),

        print "}"

    @pyqtSlot(list)
    def readArduData(self, pwrData):
        """
        Read power data from Arduino and prepare for saving.
        """

    def closeEvent(self, event):

        #print "x, y, w, h =",self.x(),self.y(),self.width(),self.height()
        # save the current configuration
        self.settings = QSettings(QSettings.IniFormat, QSettings.UserScope, "APT", "SpiNN-4 Power Profiler")
        self.settings.setValue(self.conf.xPosKey, self.x())
        self.settings.setValue(self.conf.yPosKey, self.y())
        self.settings.setValue(self.conf.widthKey, self.width())
        self.settings.setValue(self.conf.heightKey, self.height())
        self.settings.sync()
        #print "Conf saved!"

        # Notify ifaceArduSpiNN to stop the thread:
        self.cw.stop() # to avoid QThread being destroyed while thread is still running
        event.accept()


