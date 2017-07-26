"""
OVERVIEW:
    This module will interact with arduino and SpiNNaker.
    Note: This is a GUI module with console/text-based output.

    When using PyQt4, serial port module is not included (Serial port IS officially included in Qt5, but not
    int Qt4). Hence, we use pySerial library along with PyQt4. The idea is to create a qthread that reads/encapsulates
    serial port communication.

    The most essential data produced by this module is pwrData (7 items, floating point)
    and prfData (48 items, 32-bit integer). These will be emitted as arduUpdate and spinUpdate signals.

    Also, make sure that the program has right access to the emulated serial-port on the USB port.
    For example, in Fedora, it might get /dev/ttyACM0. Read access permission must be granted for this program
    on that file.

SYNOPSIS:
    This module provides the class wSpiNN. It has responsibility for managing connection with
    the Arduino as well as with Spin4 board through port RECV_PORT (see constDef.py). 
    It reads UDP socket from that port, and decode the message. 

    The profiler program in SpiNNaker uses the following SDP format for sending data:
    pad, flags, tag, dp, sp, da, say, sax, cmd, seq, arg1, arg2, arg2, \
    [chip-00], [chip-01], ..., [chip-47] 

    where the [chip-xx] is a data structure with the following format:
    [chip-xx] == [chip_freq(byte), num_active_cores(byte), temperature(short)]
    For temperature, only sensor-3 is reported (since sensor-1 and sensor-2 are less reliable
    than sensor-3).

    Hence, this module should distinguish, upon data reception, which chip sends the data.

    Note: This module will not save the data.
          Data will be saved automatically in the MainWindow section (using signal emission).

    The processing version uses the following mechanism for measuring the power:
    // Bank A
    meas_i = dac[0]*2.5/(ADC_Bits*0.005*50); 	//I
    meas_v = dac[1]*2.5/ADC_Bits; 				//V
    meas[0] = meas_i * meas_v;
      
    // Bank B
    meas_i = dac[2]*2.5/(ADC_Bits*0.005*50); 	//I
    meas_v = dac[3]*2.5/ADC_Bits; 				//V
    meas[1] = meas_i * meas_v; 
      
    // Bank C
    meas_i = dac[4]*2.5/(ADC_Bits*0.005*50); 	//I
    meas_v = dac[5]*2.5/ADC_Bits; 				//V
    meas[2] = meas_i * meas_v;
      
    //FPGA
    meas_i = dac[6]*2.5/(ADC_Bits*0.010*50); 	//I
    meas_v = dac[7]*2.5/ADC_Bits; 				//V
    meas[3] = meas_i * meas_v;
      
    //SDRAM
    meas_i = dac[8]*2.5/(ADC_Bits*0.005*50); 	//I
    meas_v = dac[9]*2.5/ADC_Bits; 				//V
    meas[4] = meas_i * meas_v;
      
    // 3.3V
    meas_i = dac[10]*2.5/(ADC_Bits*0.010*50); 	//I
    meas_v = dac[13]*2.5/ADC_Bits*3/2; 			//V
    meas[5] = meas_i * meas_v;
      
    // 12V
    meas_i = dac[11]*2.5/(ADC_Bits*0.005*50); 	//I
    meas_v = dac[12]*2.5/ADC_Bits*6; 			//V
    meas[6] = meas_i * meas_v;
"""
import struct
from PyQt4 import Qt, QtGui, QtCore, QtNetwork
import constDef as DEF
import serial # Since PyQt4 doesn't include serial port natively
import numpy
import time
import os


"""
ifaceArduSpiNN inherits QWidget
It provides interface to Arduino and SpiNNaker:
- With SpiNNaker, it uses UDP protocol
- With Arduino, it uses TTY
"""
class ifaceArduSpiNN(QtGui.QWidget):
    # The following signals MUST defined here, NOT in the init()
    # spinUpdate = QtCore.pyqtSignal('QByteArray')  # for streaming data to the plotter/calibrator
    spinUpdate = QtCore.pyqtSignal(list) # data from SpiNNaker profiler program
    arduUpdate = QtCore.pyqtSignal(list) # data from Arduino
    isReady = False
    okToClose = True
    pwrData = [0.0 for _ in range(7)]   # power data from Arduino
    prfData = list()   # profiler data from SpiNNaker

    def __init__(self, logFName=None, parent=None):
        QtGui.QWidget.__init__(self, parent)
        self.logFName = logFName
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
        palette.setBrush(QtGui.QPalette.Active, QtGui.QPalette.Base, brush1)
        palette.setBrush(QtGui.QPalette.Inactive, QtGui.QPalette.Text, brush)
        palette.setBrush(QtGui.QPalette.Inactive, QtGui.QPalette.Base, brush1)
        brush2 = QtGui.QBrush(QtGui.QColor(128,128,128,255))
        brush2.setStyle(Qt.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Disabled, QtGui.QPalette.Text, brush2)
        brush3 = QtGui.QBrush(QtGui.QColor(247,247,247,255))
        brush3.setStyle(Qt.Qt.SolidPattern)
        palette.setBrush(QtGui.QPalette.Disabled, QtGui.QPalette.Base, brush3)

        # create the arduConsole-like widget to display Arduino data
        self.arduConsole = QtGui.QPlainTextEdit(self)
        self.arduConsole.setPalette(palette)

        self.spinConsole = QtGui.QPlainTextEdit(self)
        self.spinConsole.setPalette(palette)

        x = self.x()
        y = self.y()
        w = 400
        h = 1000

        vBoxLayout = QtGui.QVBoxLayout(self)
        vBoxLayout.addWidget(self.arduConsole)
        vBoxLayout.addWidget(self.spinConsole)
        self.setLayout(vBoxLayout)

        self.setGeometry(x, y, w, h)

        # create and initialize the UDP socket for this module
        self.SpiNNSock = QtNetwork.QUdpSocket(self)
        self.initSpiNNSock(DEF.RECV_PORT)

        # create serial module
        """ How it usually works:
        connect(&workerThread, SIGNAL(finished()), worker, SLOT(deleteLater()));
        connect(this, SIGNAL(operate(QString)), worker, SLOT(doWork(QString)));
        connect(worker, SIGNAL(resultReady(QString)), this, SLOT(handleResults(QString)));
        workerThread.start();

        // Additionally, before calling the start, we need:
        connect(thread, SIGNAL(started()), worker, SLOT(process()));
        connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
        connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

        """
        self.ttyThread = QtCore.QThread()
        self.ttyObj = SerialObject(DEF.Arduino_port, 115200)         # create and initialize the serial object
        self.ttyObj.moveToThread(self.ttyThread)  # move to an independent thread
        self.ttyThread.started.connect(self.ttyObj.run)
        self.ttyThread.finished.connect(self.ttyObj.deleteLater)
        self.ttyThread.finished.connect(self.ttyThread.deleteLater)
        self.ttyObj.finished.connect(self.ttyThread.quit)
        self.ttyObj.finished.connect(self.ttyObj.deleteLater)
        self.ttyObj.finished.connect(self.ttyThread.deleteLater)
        self.ttyObj.dataReady.connect(self.arduinoDataReady)
        self.ttyThread.start()               # call the serial object event-loop

        # Preparing experimental folder:
        print "Preparing {}".format(DEF.LOG_DIR)
        str = "mkdir -p {}".format(DEF.LOG_DIR)
        os.system(str)
        # Open logfile
        if self.logFName is None:
            fname = DEF.LOG_DIR + time.strftime("%b_%d_%Y-%H.%M.%S", time.gmtime()) + ".log"
        else:
            fname = DEF.LOG_DIR + self.logFName + ".log"
        self.logFile = open(fname, "w")
        print fname

        
    #---------------------- Class GUI/basic functionalities and helpers -----------------
    def paintEvent(self, e):
        w = self.width()
        h = self.height()
        #self.arduConsole.setMaximumSize(w, h)
        #self.arduConsole.setMinimumSize(w, h)

    def stop(self):
        self.ttyObj.stop()

    def closeEvent(self, event):
        if self.okToClose:
            event.accept()
            # Don't forget to close log file:
            self.logFile.close()
        else:
            event.ignore()

    def SaveData(self):
        """
        Operates on self.pwrdata and self.prfData
        self.pwrData is a floating point (7 items), and self.prfData is in integer 32-bit (48 items)

        Save data on disk.
        txt = ''
        for c in range(4):
            txt += str(self.Tpi[c])
            txt += ','
        for c in range(4):
            for s in range(3):
                txt += str(self.Tspin[c][s])
                if c*s is not 6:
                    txt += ','
        txt += '\n'
        # print txt
        self.logFile.write(txt)
        """

        # if Profiler is not ready, then we'll get "list index out of range"
        try:
            txt = ''
            for i in range(7):
                txt += str(self.pwrData[i])
                txt += ','
            for i in range(48):
                txt += str(self.prfData[i])
                txt += ','
                # because now we add time stamp, so we don't check 47
                #if i < 47:
                #    txt += ','
            t = time.time()
            s = "%10.6f"%t
            txt += s
            txt += '\n'
            self.logFile.write(txt)
        except IndexError as ie:
            """
            if Profiler is not ready, then we'll get "list index out of range"
            """
            #print ie, ": Profiler is not ready?"
            # in the case that profiler is not ready, then let's put 0 on pfrData
            txt = ''
            for i in range(7):
                txt += str(self.pwrData[i])
                txt += ','
            for i in range(48):
                txt += '0,'
                # because now we add time stamp, so we don't check 47
                #if i < 47:
                #    txt += ','
            t = time.time()
            s = "%10.6f"%t
            txt += s
            txt += '\n'
            self.logFile.write(txt)



    #---------------------- Related with Arduino serial interface ----------------------
    @QtCore.pyqtSlot(list)
    def arduinoDataReady(self, arduinoData):
        """
        Put the data in the 'arduConsole'
        """
        str = "[Ardui] : {}".format(arduinoData)
        self.arduConsole.appendPlainText(str)

        # then convert into list of numeric data
        if len(arduinoData) < 14:
            return

        try:
            rawData = map(int, arduinoData)
            # rawData has the following format:
            # I_BankA, V_BankA, I_BankB, V_BankB, I_BankC, V_BankC, I_FPGA, V_FPGA, I_SDRAM,
            # V_SDRAM, I_BMP, V_ALL, V_BMP, I_All -> then it needs to be modified!!!

            modData = numpy.concatenate((rawData[:11], rawData[13:], rawData[11:13]))
            for i in range(7):
                self.pwrData[i] = (modData[i*2]*DEF.V_ref/(DEF.ADC_Bits*DEF.R_shunt[i])) *\
                                  (modData[i*2+1]*DEF.V_ref*DEF.V_div[i]/(DEF.ADC_Bits))

            # also, trigger SpiNNaker profiler to collect and report again
            # this is how we synchronize Arduino and profiler data
            self.reqSpiNNData()

            # finally, inform main controller about the Arduino and SpiNNaker data:
            self.arduUpdate.emit(self.pwrData)
            self.spinUpdate.emit(self.prfData)

            # and store to a log file
            self.SaveData()
        except ValueError as ve:
            print "Value error on arduinoData:", ve


    #---------------------- Related with SpiNNaker UDP interface -----------------------
    def initSpiNNSock(self, port):
        str = "Try opening port-{} for receiving SpiNNaker Data...".format(port)
        print str,
        self.arduConsole.appendPlainText(str)
        #result = self.sock.bind(QtNetwork.QHostAddress.LocalHost, DEF.RECV_PORT) 
        #         --> problematik dgn QHostAddress.LocalHost !!!
        result = self.SpiNNSock.bind(port)
        if result is False:
            str = 'failed! Cannot open UDP port-{}'.format(port)
            print str
            self.arduConsole.appendPlainText(str)
            self.isReady = False
        else:
            str = "done!"
            print str
            self.arduConsole.appendPlainText(str)
            self.isReady = True
            self.SpiNNSock.readyRead.connect(self.readSpiNN)
            # then trigger the profiler to collect and report data
            self.reqSpiNNData()
    
    @QtCore.pyqtSlot()   
    def readSpiNN(self):
        """
        Debugging on 1 June 2017, 12:27
        Program doesn't receive anything?
        Check iptag 3
        """

        """
            The following format is used by the SpiNNaker profiler 333
            fmt = "<HQ2H51I"
            udp_header, cmd_rc, seq, arg1, arg2, arg3, \
            cpu[0], cpu[1], cpu[2], cpu[3], cpu[4], cpu[5], cpu[6], cpu[7], cpu[8], cpu[9], cpu[10], \
            cpu[11], cpu[12], cpu[13], cpu[14], cpu[15], cpu[16], cpu[17], cpu[18], cpu[19], cpu[20], \
            cpu[21], cpu[22], cpu[23], cpu[24], cpu[25], cpu[26], cpu[27], cpu[28], cpu[29], cpu[30], \
            cpu[31], cpu[32], cpu[33], cpu[34], cpu[35], cpu[36], cpu[37], cpu[38], cpu[39], cpu[40], \
            cpu[41], cpu[42], cpu[43], cpu[44], cpu[45], cpu[46], cpu[47] = struct.unpack(fmt, datagram)
        """

        while self.SpiNNSock.hasPendingDatagrams():
            szData = self.SpiNNSock.pendingDatagramSize()
            datagram, host, port = self.SpiNNSock.readDatagram(szData)

            #print "Receiving {} items".format(len(datagram))
            #return


            fmt = "<HQ2H51I"
            if len(datagram) < struct.calcsize(fmt):
                return


            cpu = [None for _ in range(48)]        
            pad, udp_header, cmd_rc, seq, arg1, arg2, arg3, \
            cpu[0], cpu[1], cpu[2], cpu[3], cpu[4], cpu[5], cpu[6], cpu[7], cpu[8], cpu[9], cpu[10], \
            cpu[11], cpu[12], cpu[13], cpu[14], cpu[15], cpu[16], cpu[17], cpu[18], cpu[19], cpu[20], \
            cpu[21], cpu[22], cpu[23], cpu[24], cpu[25], cpu[26], cpu[27], cpu[28], cpu[29], cpu[30], \
            cpu[31], cpu[32], cpu[33], cpu[34], cpu[35], cpu[36], cpu[37], cpu[38], cpu[39], cpu[40], \
            cpu[41], cpu[42], cpu[43], cpu[44], cpu[45], cpu[46], cpu[47] = struct.unpack(fmt, datagram)

            # Use cmd_rc and seq for debugging
            # print "[SpiNN] : cmd_rc = 0x%x, seq = %d" % (cmd_rc, seq)
            str = "[SpiNN] : {}".format(cpu)
            self.spinConsole.appendPlainText(str)

            self.prfData = cpu

    def reqSpiNNData(self):
        """
        Trigger profiler in SpiNNaker to collect and report data.
        Use simplified parameters: flags = 0x07, tag = 0, dp = 7, dc = 17, dax = 0, day = 0, 
        cmd_rc = HOST_REQ_PROFILER_INFO, seq = 0, arg1 = 0, arg2 = 0, arg3 = 0, bArray = None
        """
        tag = 0x07 # no reply
        dax = 0; day = 0; da = (dax << 8) + day # send to chip<0,0>
        dp = DEF.PROFILER_SDP_PORT; dc = DEF.PROFILER_CORE; dpc = (dp << 5) + dc # to core-17 on port-7
        sa = 0; spc = 255; # sent from host-PC
        cmd_rc = DEF.HOST_REQ_PROFILER_INFO; seq = 0; arg1=0; arg2=0; arg3=0;
        bArray = None;

        pad = struct.pack('2B',0,0)
        hdr = struct.pack('4B2H',0x07,tag,dpc,spc,da,sa)
        scp = struct.pack('2H3I',cmd_rc,seq,arg1,arg2,arg3)
        
        if bArray is not None:
            sdp = pad + hdr + scp + bArray
        else:
            sdp = pad + hdr + scp

        CmdSock = QtNetwork.QUdpSocket()
        CmdSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF.MACHINE), DEF.SEND_PORT)
        return sdp

    def getChipCoord(self,chipID):
        """
        Use CHIP_LIST_48 for mapping
        """
        x = DEF.CHIP_LIST_48[chipID][0]
        y = DEF.CHIP_LIST_48[chipID][1]
        return (x,y)


#-------------------------------- Class for reading serial data from Arduino ----------------------------
class SerialObject(QtCore.QObject):
    finished = QtCore.pyqtSignal() # emit this signal to inform that this object has finished its job
    dataReady = QtCore.pyqtSignal(list) # emit this list when receives arduino data
    isReady = False
    def __init__(self, port, baud=115200, TTYconfig='default', parent=None):
        super(SerialObject, self).__init__(parent)
  
        self.tty = serial.Serial()
        self.tty.port = port
        self.tty.baudrate = baud
        if TTYconfig is not 'default':
            self.tty.bytesize = TTYconfig.bytesize
            self.tty.parity = TTYconfig.parity
            self.tty.stopbits = TTYconfig.stopbits
        else:
            self.tty.bytesize = 8
            self.tty.parity = 'N'
            self.tty.stopbits = 1

        self.tty.open()

        if self.tty.is_open is True:
            self.isReady = True
            self.arduinoData = list()
            # create a scheduler for an event-loop
            # use QTimer as the scheduler
            self.scheduler = QtCore.QTimer()
            self.scheduler.setInterval(0)     # fire as soon as possible
            self.scheduler.timeout.connect(self.readSerial)

    def stop(self):
        """
        Call this function to stop the event-loop
        """
        self.scheduler.stop()
        self.finished.emit()

    def run(self):
        """
        The main class event-loop. Call this function to start the event-loop.
        """
        if self.isReady is False:
            return
        self.scheduler.start()

    def readSerial(self):
        # read serial data from Arduino
        line = self.tty.readline()
        strList = line.split(',')
        numList = list(strList)
        self.arduinoData = numList[:14] # ignore time-stamp (the last element)
        if len(self.arduinoData)==14:
            self.dataReady.emit(self.arduinoData)


