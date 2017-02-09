"""
OVERVIEW:
    This module will interact with Raspberry-pi and display raw data.
    Note: This is a GUI module with console/text-based output.

SYNOPSIS:
    This module provides the class wRaspi. It has responsibility for managing connection with
    Raspberry-pi (Pi3) board through port RASPI_PORT (see constDef.py). It reads TCP socket from 
    that port, and decode the message. 

    When initialized, first it tries to establish the connection with P3 (P3 acts as a TCP server).
    Once connected, pi3 will stream the data to this module and this module will decode 
    the data.

    The temperature reader program in pi3 uses the following comma-separated 
    floating-point values:
    "

    Note: This module will not save the data.
          Data will be saved automatically in the MainWindow section (using signal emission).


    TODO: validasi sensor-to-chip mapping (in ctask.h)
"""
import struct
from PyQt4 import Qt, QtGui, QtCore, QtNetwork
import constDef as DEF

class wRaspi(QtGui.QWidget):
    # The following signals MUST defined here, NOT in the init()
    piUpdate = QtCore.pyqtSignal(list)  # for streaming data to the plotter/calibrator
    isReady = False;

    okToClose = False
    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)
        
        self.console = QtGui.QPlainTextEdit(self)

        palette = QtGui.QPalette()
        brush = QtGui.QBrush(QtGui.QColor(255,255,255,255))
        brush.setStyle(Qt.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Active, QtGui.QPalette.Text, brush)
        brush1 = QtGui.QBrush(QtGui.QColor(0,0,0,255))
        palette.setBrush(QtGui.QPalette.Active, QtGui.QPalette.Base, brush1);
        palette.setBrush(QtGui.QPalette.Inactive, QtGui.QPalette.Text, brush);
        palette.setBrush(QtGui.QPalette.Inactive, QtGui.QPalette.Base, brush1);
        brush2 = QtGui.QBrush(QtGui.QColor(128,128,128,255))
        brush2.setStyle(Qt.Qt.SolidPattern);
        palette.setBrush(QtGui.QPalette.Disabled, QtGui.QPalette.Text, brush2);
        brush3 = QtGui.QBrush(QtGui.QColor(247,247,247,255))
        brush3.setStyle(Qt.Qt.SolidPattern);
        palette.setBrush(QtGui.QPalette.Disabled, QtGui.QPalette.Base, brush3);
        self.console.setPalette(palette)
        x = self.x()
        y = self.y()
        w = 400
        h = 1000
        self.setGeometry(x, y, w, h)

        # Create a socket and initiate a connection to Pi3 server
        self.blockSize = 0
        self.raspiSock = QtNetwork.QTcpSocket(self)
        self.raspiSock.connected.connect(self.connected)
        self.raspiSock.disconnected.connect(self.disconnected)
        self.raspiSock.error.connect(self.displayError)
        self.raspiSock.readyRead.connect(self.readRaspi)
        self.initRaspiSock(DEF.RASPI_ADDR, DEF.RASPI_PORT)

    def paintEvent(self, e):
        w = self.width()
        h = self.height()
        self.console.setMaximumSize(w, h)
        self.console.setMinimumSize(w, h)        

    def connected(self):
        print "connected!"
        isReady = True

    def disconnected(self):
        print "disconnected! Restart the system!!!"
        isReady = False;

    def displayError(self, socketError):
        if socketError == QtNetwork.QAbstractSocket.RemoteHostClosedError:
            str = ""
            pass
        elif socketError == QtNetwork.QAbstractSocket.HostNotFoundError:
            str = "The host was not found. Please check the host name and port settings."
        elif socketError == QtNetwork.QAbstractSocket.ConnectionRefusedError:
            str = "The connection was refused by the peer. Make sure the "\
                    "Pi3 server is running, and check that the host name "\
                    "and port settings are correct."
        else:
            str = "The following error occurred: %s." % self.raspiSock.errorString()
  
        self.console.insertPlainText(str)

    def initRaspiSock(self, addr, port):
        print "Try to connect to Pi3 server at {}:{} ...".format(addr, port),
        self.raspiSock.connectToHost(addr, port)
   
    #@QtCore.pyqtSlot()   
    def readRaspi(self):
        # print "Retrieve something" # --> OK!
        """
        Read data from raspberry pi
        """
        instr = QtCore.QTextStream(self.raspiSock)
        """
        instr = QtCore.QDataStream(self.raspiSock)
        instr.setVersion(QtCore.QDataStream.Qt_4_0)
  
        if self.blockSize == 0:
            if self.raspiSock.bytesAvailable() < 2:
                return
  
            self.blockSize = instr.readUInt16()
  
        if self.raspiSock.bytesAvailable() < self.blockSize:
            return
        """
        Tdata = instr.readLine()
        self.console.insertPlainText(Tdata + "\n")

        splt = Tdata.split(",")
        Tval = list()
        Tval.append(float(splt[0]))
        Tval.append(float(splt[1]))
        Tval.append(float(splt[2]))
        Tval.append(float(splt[3]))

        self.piUpdate.emit(Tval)

    def closeEvent(self, event):
        if self.okToClose:
            if isReady is True:
                self.raspiSock.disconnectFromHost()
            event.accept()
        else:
            event.ignore()

