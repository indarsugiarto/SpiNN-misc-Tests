"""
OVERVIEW:
    This module will interact with SpiNNaker and display raw data.
    Note: This is a GUI module with console/text-based output.

SYNOPSIS:
    This module provides the class wSpiNN. It has responsibility for managing connection with
    Spin3 board through port RECV_PORT (see constDef.py). It reads UDP socket from that port,
    and decode the message. 
    The profiler program in SpiNNaker uses the following SDP format for sending data:
    fmt = "<H4BH2B2H3I18I"
    pad, flags, tag, dp, sp, da, say, sax, cmd, freq, temp1, temp2, temp3, \
    cpu0, cpu1, cpu2, cpu3, cpu4, cpu5, cpu6, cpu7, cpu8, cpu9, cpu10, \
    cpu11, cpu12, cpu13, cpu14, cpu15, cpu16, cpu17 = struct.unpack(fmt, datagram)

    Where temp1-->sensor1, temp2-->sensor2, temp3-->sensor3.
    Hence, this module should distinguish, upon data reception, which chip sends the data.

    Note: This module will not save the data.
          Data will be saved automatically in the MainWindow section (using signal emission).
"""
import struct
from PyQt4 import Qt, QtGui, QtCore, QtNetwork
import constDef as DEF

"""
wSpiNN inherits QWidget
"""
class wSpiNN(QtGui.QWidget):
    # The following signals MUST defined here, NOT in the init()
    # spinUpdate = QtCore.pyqtSignal('QByteArray')  # for streaming data to the plotter/calibrator
    spinUpdate = QtCore.pyqtSignal(list)
    spinChipID = QtCore.pyqtSignal(list)
    isReady = False

    okToClose = False

    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)

        self.console = QtGui.QPlainTextEdit(self)
        """
        Note on QPlainTextEdit:
        When using appendPlainText, endl will automatically added; whereas in insertPlainText,
        endl will not be added!
        """
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
        h = 600
        self.setGeometry(x, y, w, h)

        # create and initialize the UDP socket for this module
        self.SpiNNSock = QtNetwork.QUdpSocket(self)
        self.initSpiNNSock(DEF.RECV_PORT)
        
    def paintEvent(self, e):
        w = self.width()
        h = self.height()
        self.console.setMaximumSize(w, h)
        self.console.setMinimumSize(w, h)        

    def initSpiNNSock(self, port):
        str = "Try opening port-{} for receiving SpiNNaker Data...".format(port)
        print str,
        self.console.appendPlainText(str)
        #result = self.sock.bind(QtNetwork.QHostAddress.LocalHost, DEF.RECV_PORT) 
        #         --> problematik dgn QHostAddress.LocalHost !!!
        result = self.SpiNNSock.bind(port)
        if result is False:
            str = 'failed! Cannot open UDP port-{}'.format(port)
            print str
            self.console.appendPlainText(str)
            self.isReady = False
        else:
            str = "done!"
            print str
            self.console.appendPlainText(str)
            self.isReady = True
            self.SpiNNSock.readyRead.connect(self.readSpiNN)
    
    @QtCore.pyqtSlot()   
    def readSpiNN(self):
        """
            The following format is used by the SpiNNaker profiler 111
            fmt = "<H4BH2B2H3I18I"
            pad, flags, tag, dp, sp, da, say, sax, cmd, freq, temp1, temp2, temp3, \
            cpu0, cpu1, cpu2, cpu3, cpu4, cpu5, cpu6, cpu7, cpu8, cpu9, cpu10, \
            cpu11, cpu12, cpu13, cpu14, cpu15, cpu16, cpu17 = struct.unpack(fmt, datagram)

            For the profiler 222, the simplified version is used:
            fmt = "<H4BH2B2H3I"
            pad, flags, tag, dp, sp, da, say, sax, \
            cmd, freq, temp1, temp2, temp3 = struct.unpack(fmt, datagram)
        """

        while self.SpiNNSock.hasPendingDatagrams():
            szData = self.SpiNNSock.pendingDatagramSize()
            datagram, host, port = self.SpiNNSock.readDatagram(szData)
        
            fmt = "<H4BH2B2H3I"
            pad, flags, tag, dp, sp, da, say, sax, \
            cmd, freq, temp1, temp2, temp3 = struct.unpack(fmt, datagram)

            # Dump the raw data on its display widget
            str = "[%d,%d] %d,%d,%d @%dMHz" % (sax,say,temp1,temp2,temp3,freq)
            self.console.appendPlainText(str)

            chipIdx = self.getChipID(sax, say)
            # emit chipID configuration: useful for debugging
            chipIdxInfo = [chipIdx, sax, say]
            self.spinChipID.emit(chipIdxInfo)

            tempVal = [chipIdx, temp1, temp2, temp3]
            self.spinUpdate.emit(tempVal)
            # self.spinUpdate.emit(something) -> old version uses QByteArray!!!

    def closeEvent(self, event):
        if self.okToClose:
            event.accept()
        else:
            event.ignore()

    def getChipID(self,x,y):
        """
        Given chip coordinat (x, y), determine the chip number as follows:
        <0,0> = 0
        <1,0> = 1
        <0,1> = 2
        <1,1> = 3
        """
        return y*2+x

