"""
OVERVIEW:
    This will handle both the main window that provides:
    - console-like view for SpiNNaker temperature data
    - console-like view for Raspberry-pi temperature data
    - plotter for the resulting calibration
    - save data (from both SpiNN and Raspi) to a file

SYNOPSIS:
    This main window has two sub window: SpiNNaker widget (sw) and Raspberry widget (pw).
    sw receives data from SpiNNaker and emit it to MainWindow.
    pw receives data from Raspberry-pi and emit it to MainWindow.
    MainWindow then processes the data (such as regression for calibration), and
    then forward the resulting data to the plotter (tw).
    MainWindow also saves the data in a log file.
"""

from PyQt4 import QtGui, QtNetwork
from PyQt4.QtCore import *
import QtMainWindow
from myPlotterT import Twidget
from mySpiNN import *  # it has UDP protocol involved
from myRaspi import *  # it has TCP protocol involved
import constDef as DEF # the calibration constants should be defined here
"""===============================================================================================
                                             MainGUI
-----------------------------------------------------------------------------------------------"""
class MainWindow(QtGui.QMainWindow, QtMainWindow.Ui_QtMainWindow):
    """ It inherits QMainWindow and uses pyuic4-generated python files from the .ui """

    def __init__(self, parent=None):
        super(MainWindow, self).__init__(parent)
        self.setupUi(self)
        self.setCentralWidget(self.mdiArea);
        self.statusTxt = QtGui.QLabel("")
        self.statusBar().addWidget(self.statusTxt)

        """
        Scenario: 
        - sw and pw send data to MainWindow
        - MainWindow do the averaging on sw data, then send avg_sw and pw to plotter
          At the same time, MainWindow store the data
        """
               
        self.sw = wSpiNN(self)
        self.subWinSpin = QtGui.QMdiSubWindow(self)
        self.subWinSpin.setWidget(self.sw)
        self.mdiArea.addSubWindow(self.subWinSpin)
        self.subWinSpin.setGeometry(0,0,self.sw.width(),self.sw.height())
        self.sw.show()
        if self.sw.isReady is True:
            initialMsg = "SpiNNaker widget is ready!"
        else:
            initialMsg = "SpiNNaker widget is NOT ready!!!"
        self.sw.console.insertPlainText(initialMsg)
        self.sw.spinUpdate.connect(self.readSpiNN)

        self.pw = wRaspi(self)
        self.subWinRaspi = QtGui.QMdiSubWindow(self)
        self.subWinRaspi.setWidget(self.pw)
        self.mdiArea.addSubWindow(self.subWinRaspi)
        self.subWinRaspi.setGeometry(self.sw.width(),0,self.pw.width(),self.pw.height())
        self.pw.show()
        self.pw.piUpdate.connect(self.readRaspi)


        #self.tw = Twidget(self);

        #self.connect(self.sw, SIGNAL("spinUpdate()"), SLOT("readSpiNN()"))
        #self.connect(self.pw, SIGNAL("piUpdate()"), SLOT("readRaspi()"))

    """
    ######################### GUI callback ########################### 
    """

    @pyqtSlot(list)
    def readSpiNN(self, Tdata):
        """
        SpiNNaker widget will send data as a list. 
        TODO: accumulate
        """
        # just for debugging:
        print Tdata

    @pyqtSlot(list)
    def readRaspi(self, Tdata):
        """
        Raspi widget will send data as a list
        TODO: averaging spin data, send to plotter, and save to a file
        """
        print "Receiving {} data".format(len(Tdata))
        print Tdata


    @pyqtSlot()
    def SaveData(self):
        if self.action_SaveData.isChecked():
            print "SaveData is now checked"
        else:
            print "SaveData is now not checked"

    def closeEvent(self, event):
        """
        Don't forget to close log file
        """

