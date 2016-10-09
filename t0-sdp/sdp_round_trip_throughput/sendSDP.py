#!/usr/bin/python

import struct
from PyQt4 import QtNetwork
import time
import socket


SPINN3_HOST             = '192.168.240.253'
SPINN5_HOST             = '192.168.240.1'

DEF_SEND_PORT           = 17893 #tidak bisa diganti dengan yang lain
DEF_HOST                = SPINN3_HOST
SDP_UDP_REPLY_PORT      = 20001
TOTAL_PACKETS           = 350000

"""
We want to see the different between handshaking mechanism vs non-handshaking mechanism.
In handshaking, the next SDP packet will be sent if spiNNaker has ackowledge the previous
one. In non handshaking, the SDP packets will be streamed with certain amount of delay.
"""
HANDSHAKING = 1
NON_HANDSHAKING = 0

Mode = NON_HANDSHAKING	    # for non-handshaking mode
#Mode = HANDSHAKING	        # handshaking mode

def sendSDP(flags, tag, dp, dc, dax, day, cmd, seq, arg1, arg2, arg3, bArray):
    da = (dax << 8) + day
    dpc = (dp << 5) + dc
    sa = 0
    spc = 255
    pad = struct.pack('<2B',0,0)
    hdr = struct.pack('<4B2H',flags,tag,dpc,spc,da,sa)
    scp = struct.pack('<2H3I',cmd,seq,arg1,arg2,arg3)
    if bArray is not None:
        sdp = pad + hdr + scp + bArray
    else:
        sdp = pad + hdr + scp

    #CmdSock = QtNetwork.QUdpSocket()
    #CmdSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF_HOST), DEF_SEND_PORT)
    #CmdSock.flush()
    return sdp

def giveDelay():
    val = 1
    #for i in range(1150):	# This gives 6.5MBps
    #for i in range(800):	# This gives 9.4MBps -> equal to 32fps for grey color vga resolution
    for i in range(720):	# This gives 10MBps -> equal to 33fps for grey color vga resolution  
        val += i
    return val

def main():
    str = "Will bursting {} packets!".format(TOTAL_PACKETS)
    ask = raw_input(str)
    lst = list()
    for i in range(256):
        if (i % 2)==0:
            lst.append(0xA0)
        else:
            lst.append(0xAF)
    ba = bytearray(lst)

    sdp = sendSDP(7, 0, 1, 1, 0, 0, 1, 2, 3, 4, 5, ba)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
      sock.bind( ('', SDP_UDP_REPLY_PORT) )
    except OSError as msg:
      print "%s" % msg
      sock.close()
      sock = None

    if sock is None:
         return

    sdpSock = QtNetwork.QUdpSocket()
    for j in range(TOTAL_PACKETS):
         sdpSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF_HOST), DEF_SEND_PORT)
         #sdpSock.flush() # It seems this flushing doesn't make any difference
         if Mode==HANDSHAKING:
             reply = sock.recv(1024)
         else:
             #time.sleep(0.000001) # 1ms resolution: with this, we can achieve 5.0MBps
             angka = giveDelay()


    print "done with angka = {} !".format(angka)
    print "Now notify SpiNNaker...",
    sdp = sendSDP(7, 0, 1, 1, 0, 0, 1, 2, 3, 4, 5, None)
    # repeat several times to make sure SpiNNaker receive "stop" signal
    for k in range(5):
         sdpSock.writeDatagram(sdp, QtNetwork.QHostAddress(DEF_HOST), DEF_SEND_PORT)
    print "done!"

if __name__=='__main__':
    main()
