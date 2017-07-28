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

DEF_HOST = '192.168.240.253'
DEF_SEND_PORT = 17893

PLL_CPU = 0
PLL_AHB = 1
PLL_RTR = 2


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
        print "Usage ./test_maxFreq.py machine_ip chipX chipY"
    else:
        """
        the aplx uses the following encoding:
			PLL_PART comp = (PLL_PART)(msg->seq >> 8);
            uint f = msg->seq & 0xFF;
            changeFreq(comp, f);
        """
        dax = int(sys.argv[2])
        day = int(sys.argv[3])
        ip = sys.argv[1]
        cmd_rc = HOST_SET_FREQ_VALUE
        dp = DEF_HOST_SDP_PORT
        dc = DEF_PROFILER_CORE
        # first, set cpu
        print "Requesting maximum frequency (200) for cpu..."
        seq = 200
        sendSDP(ip,7, 0, dp, dc, dax, day, cmd_rc, seq, 0, 0, 0, None)
        time.sleep(1)
        # second, the AHB
        print "Requesting maximum frequency (130) for ahb..."
        seq = (PLL_AHB << 8) + 130
        sendSDP(ip,7, 0, dp, dc, dax, day, cmd_rc, seq, 0, 0, 0, None)
        time.sleep(1)
        # last, the RTR
        print "Requesting maximum frequency (130) for rtr..."
        seq = (PLL_RTR << 8) + 130
        sendSDP(ip,7, 0, dp, dc, dax, day, cmd_rc, seq, 0, 0, 0, None)
        print "Done!"


