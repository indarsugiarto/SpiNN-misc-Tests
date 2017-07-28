#!/usr/bin/python

# Just logging the cpu load information sent by the aplx

from PyQt4 import Qt, QtCore, QtNetwork
import sys
import time
import os
import struct
import argparse

DEF_HOST_PC = '192.168.240.2'   # unfortunately we cannot use socket.getfqdn() to get this
DEF_MACHINE = '192.168.240.1'
DEF_SEND_PORT = 17893	# the default port for sending sdp to SpiNNaker
DEF_PROFILER_CORE = 1
DEF_REPORT_TAG = 3
DEF_REPORT_PORT = 40003
DEF_HOST_SDP_PORT = 7		#// port-7 has a special purpose, usually related with ETH
HOST_REQ_PLL_INFO = 1
HOST_REQ_INIT_PLL = 2		#// host request special PLL configuration
HOST_REQ_REVERT_PLL = 3
HOST_SET_FREQ_VALUE = 4		#// Note: HOST_SET_FREQ_VALUE assumes that CPUs use PLL1,
HOST_REQ_PROFILER_STREAM = 5		#// host send this to a particular profiler to start streaming

class cLogger(QtCore.QObject):
    def __init__(self, xpos, ypos, ip, port, logName=None, parent=None):
        super(cLogger, self).__init__(parent)
        self.ip = ip
        self.port = port
        self.logName = logName
        self.xpos = xpos
        self.ypos = ypos

        self.setupUDP()
        self.isStarted = False

    def setupUDP(self):
        self.SpiNNSock = QtNetwork.QUdpSocket(self)
        print "Try opening port-{} for receiving SpiNNaker Data...".format(self.port),
        result = self.SpiNNSock.bind(self.port)
        if result is False:
            print 'failed! Cannot open UDP port-{}!'.format(self.port)
            self.isReady = False
        else:
            print "done!"
            self.SpiNNSock.readyRead.connect(self.readSpiNN)
            self.SpiNNSock.error.connect(self.udpError)

            # prepare the scp command
            self.tag = DEF_REPORT_TAG
            self.cmd = HOST_REQ_PROFILER_STREAM
            self.dp = DEF_HOST_SDP_PORT
            self.dc = DEF_PROFILER_CORE

            self.isReady = True


    def start(self):

        # prepare data format, from profiler.h:
        """
        ushort v;           // profiler version, in cmd_rc
        ushort pID;         // which chip sends this?, in seq
        uchar cpu_freq;     // in arg1
        uchar rtr_freq;     // in arg1
        uchar ahb_freq;     // in arg1
        uchar nActive;      // in arg1
        ushort  temp1;         // from sensor-1, for arg2
        ushort temp3;         // from sensor-3, for arg2
        uint memfree;       // optional, sdram free (in bytes), in arg3
        uchar load[18];
        """
        sdp_hdr = 'HQ'  # should be ten bytes
        part_1 = 'I4B2HI'
        part_2 = '18B'
        self.fmt = '<'+sdp_hdr+part_1+part_2
        self.szData = struct.calcsize(self.fmt)
        print "Require {}-byte data for each row".format(self.szData)
        self.isStarted = True

        # prepare log file
        LOG_DIR = "./experiments/"
        cmd = "mkdir -p {}".format(LOG_DIR)
        os.system(cmd)
        if self.logName is None:
            self.fname = LOG_DIR + 'cpu_' + time.strftime("%b_%d_%Y-%H.%M.%S", time.gmtime()) + ".log"
        else:
            if self.logName.find(".log") == -1:
                self.logName += ".log"
            self.fname = LOG_DIR + self.logName
        print "[INFO] Preparing log-file: {}".format(self.fname)
        self.logFile = open(self.fname, "w")

        # then trigger the profiler in chip x,y to start streaming (reporting data)
        print "[INFO] Send start streaming command to chip <{},{}>".format(self.xpos, self.ypos)
        self.sendSDP(7, self.tag, self.dp, self.dc, self.xpos, self.ypos, self.cmd, 1, 0, 0, 0, None)

        while True:
            try:
                QtCore.QCoreApplication.processEvents()
            except KeyboardInterrupt:
                break

        # then tell the profiler to stop streaming
        print "[INFO] Send stop streaming command to chip <{},{}>".format(self.xpos, self.ypos)
        self.sendSDP(7, self.tag, self.dp, self.dc, self.xpos, self.ypos, self.cmd, 0, 0, 0, 0, None)

        # finally, stop this program
        self.logFile.close()
        print "[INFO] Recording is done!"
        sys.exit(0)


    def sendSDP(self, flags, tag, dp, dc, dax, day, cmd, seq, arg1, arg2, arg3, bArray):
        """
        The detail experiment with sendSDP() see mySDPinger.py
        """
        da = (dax << 8) + day
        dpc = (dp << 5) + dc
        sa = 0
        spc = 255
        pad = struct.pack('2B', 0, 0)
        hdr = struct.pack('4B2H', flags, tag, dpc, spc, da, sa)
        scp = struct.pack('2H3I', cmd, seq, arg1, arg2, arg3)
        if bArray is not None:
            sdp = pad + hdr + scp + bArray
        else:
            sdp = pad + hdr + scp

        CmdSock = QtNetwork.QUdpSocket()
        CmdSock.writeDatagram(sdp, QtNetwork.QHostAddress(self.ip), DEF_SEND_PORT)
        return sdp

    @QtCore.pyqtSlot()
    def udpError(self, socketError):
        if socketError == QtNetwork.QAbstractSocket.RemoteHostClosedError:
            pass
        elif socketError == QtNetwork.QAbstractSocket.HostNotFoundError:
            print "[ERROR] The host was not found. Please check the host name and port settings."
        elif socketError == QtNetwork.QAbstractSocket.ConnectionRefusedError:
            print "[ERROR] The connection was refused by the server. Make sure the server is running, and check that the host name and port settings are correct."
        else:
            print "[ERROR] The following error occurred: %s." % self.SpiNNSock.errorString()

    @QtCore.pyqtSlot()
    def readSpiNN(self):

        while self.SpiNNSock.pendingDatagramSize() >= self.szData:
            ba, host, port = self.SpiNNSock.readDatagram(self.szData)
            f = [0 for _ in range(3)]
            c = [0 for _ in range(18)]

            pad,hdr,dont_care,f[0],f[1],f[2],nA,temp1,temp3,sdram,\
            c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7],c[8],\
            c[9],c[10],c[11],c[12],c[13],c[14],c[15],c[16],c[17]\
                = struct.unpack(self.fmt, ba)
            s = ''
            for i in range(3):
                s += '%d,' % f[i]
            s += '%d,%d,%d,%d' % (nA,temp1,temp3,sdram)
            for i in range(18):
                s += '%d,' % c[i]
            s += '%lf\n' % time.time()
            self.logFile.write(s)


if __name__=="__main__":
    parser = argparse.ArgumentParser(description='CPU Load Logger')
    parser.add_argument('-i', '--ip', type=str, default=DEF_MACHINE, help='SpiNNaker ip address')
    parser.add_argument('-p', '--port', type=int, default=DEF_REPORT_PORT, help='UDP port to open')
    parser.add_argument('-l', '--logname', type=str, help='Log filename')
    parser.add_argument("xpos", type=int, help="Chip's x-coordinate")
    parser.add_argument("ypos", type=int, help="Chip's y-coordinate")

    args = parser.parse_args()

    app = Qt.QCoreApplication(sys.argv)
    logger = cLogger(args.xpos, args.ypos, args.ip, args.port, args.logname, app)
    if logger.isReady is True:
        logger.start()
    else:
        sys.exit(-1)
    sys.exit(app.exec_())
