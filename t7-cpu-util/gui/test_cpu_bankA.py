#!/usr/bin/python

from PyQt4 import Qt, QtCore, QtNetwork
import sys
import struct
import time

HOST_REQ_PLL_INFO = 1
HOST_REQ_INIT_PLL = 2		#// host request special PLL configuration
HOST_REQ_REVERT_PLL = 3
HOST_SET_FREQ_VALUE = 4		#// Note: HOST_SET_FREQ_VALUE assumes that CPUs use PLL1,
HOST_REQ_PROFILER_STREAM = 5		#// host send this to a particular profiler to start streaming

DEF_PROFILER_CORE = 1
DEF_HOST_SDP_PORT = 7		#// port-7 has a special purpose, usually related with ETH

DEF_HOST = '192.168.240.1'
DEF_SEND_PORT = 17893

PLL_CPU = 0
PLL_AHB = 1
PLL_RTR = 2


bankAx = [0, 1, 0, 1, 0, 1, 1, 2, 0, 2, 1, 3, 4, 2, 3, 4]
bankAy = [0, 0, 1, 1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 5, 6, 7]

# We can use sendSDP to control frequency, getting virtual core map, etc
def sendSDP(ip, flags, tag, dp, dc, dax, day, cmd, seq, arg1, arg2, arg3, bArray):
    """
    The detail experiment with sendSDP() see mySDPinger.py
    """
    da = (dax << 8) + day
    dpc = (dp << 5) + dc
    sa = 0
    spc = 255
    pad = struct.pack('2B',0,0)
    hdr = struct.pack('4B2H',flags,tag,dpc,spc,da,sa)
    scp = struct.pack('2H3I',cmd,seq,arg1,arg2,arg3)
    if bArray is not None:
        sdp = pad + hdr + scp + bArray
    else:
        sdp = pad + hdr + scp

    CmdSock = QtNetwork.QUdpSocket()
    CmdSock.writeDatagram(sdp, QtNetwork.QHostAddress(ip), DEF_SEND_PORT)
    return sdp

if __name__=="__main__":
    if len(sys.argv) != 4:
        print "Usage ./test_maxFreq.py fcpu fahb frtr"
    else:
        """
        the aplx uses the following encoding:
			PLL_PART comp = (PLL_PART)(msg->seq >> 8);
            uint f = msg->seq & 0xFF;
            changeFreq(comp, f);
        """
        fcpu = int(sys.argv[1])
        fahb = int(sys.argv[2])
        frtr = int(sys.argv[3])
        cmd_rc = HOST_SET_FREQ_VALUE
        dp = DEF_HOST_SDP_PORT
        dc = DEF_PROFILER_CORE

        # Then iterate for all bankA chips
        for i in range(len(bankAx)):
            dax = bankAx[i]
            day = bankAy[i]
            # first, set cpu
            seq = fcpu
            sendSDP(DEF_HOST,7, 0, dp, dc, dax, day, cmd_rc, seq, 0, 0, 0, None)
            time.sleep(0.001)
            # second, the AHB
            seq = (PLL_AHB << 8) + fahb
            sendSDP(DEF_HOST,7, 0, dp, dc, dax, day, cmd_rc, seq, 0, 0, 0, None)
            time.sleep(0.001)
            # last, the RTR
            seq = (PLL_RTR << 8) + frtr
            sendSDP(DEF_HOST,7, 0, dp, dc, dax, day, cmd_rc, seq, 0, 0, 0, None)
            time.sleep(0.001)

        print "done!"
