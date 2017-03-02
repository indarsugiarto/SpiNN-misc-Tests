#! /usr/bin/python

"""
This simple script is used for sending SCP command to alter a chip's frequency.
The SCP uses cmd_rc HOST_SET_FREQ_VALUE and put the frequency value in its seq part.
"""

import socket
import struct
import sys, getopt	# for reading command line parameter

PROG_NAME = 'setSpiNNFreq.py'
DEF_HOST = '192.168.240.253'
DEF_SEND_PORT = 17893 #tidak bisa diganti dengan yang lain
DEF_WITH_REPLY = 0x87
DEF_NO_REPLY = 0x07
DEF_SDP_IPTAG = 0

#----- Definition from constDef.py --------
DEF_SDP_PORT = 7
DEF_PROFILER_CORE = 17
HOST_REQ_PLL_INFO = 1
HOST_SET_CHANGE_PLL = 2		# host request special PLL configuration
HOST_REQ_REVERT_PLL = 3
HOST_SET_FREQ_VALUE = 4		# Note: HOST_SET_FREQ_VALUE assumes that CPUs use PLL1,
#------------------------------------------

def readCLI(argv):
   chipX = 0
   chipY = 0
   freq  = 0
   try:
      opts, args = getopt.getopt(argv,"hx:y:f:",["chipX=","chipY=","freq="])
   except getopt.GetoptError:
      print '{} -x <chipX> -y <chipY> -f <freq>'.format(PROG_NAME)
      print 'if x and y are not given, then chip<0,0> will be targeted'
      print 'Note: This is only for Spin3 board!'
      sys.exit(2)
   for opt, arg in opts:
      if opt == '-h':
         print '{} -x <chipX> -y <chipY> -f <freq>'.format(PROG_NAME)
         print 'if x and y are not given, then chip<0,0> will be targeted'
         print 'Note: This is only for Spin3 board!'
         sys.exit()
      elif opt in ("-x", "--chipX"):
         chipX = int(arg)
      elif opt in ("-y", "--chipY"):
         chipY = int(arg)
      elif opt in ("-f", "--freq"):
         freq = int(arg)
   
   return chipX, chipY, freq

def sendSDP(sock, flags, tag, dp, dc, dax, day, cmd, seq, arg1, arg2, arg3, bArray):
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
    HOST, PORT = DEF_HOST, DEF_SEND_PORT
    sock.sendto(sdp,(HOST,PORT))
    return sdp

def sendCmd(sock, destChipX, destChipY, freqChip):
    flags = DEF_NO_REPLY
    tag = DEF_SDP_IPTAG
    dp = DEF_SDP_PORT
    dc = DEF_PROFILER_CORE
    dax = destChipX
    day = destChipY
    cmd = HOST_SET_FREQ_VALUE
    seq = freqChip
    arg1 = 0
    arg2 = 0
    arg3 = 0
    bArray = None
    sendSDP(sock, flags, tag, dp, dc, dax, day, cmd, seq, arg1, arg2, arg3, bArray)

def main():
    cX, cY, fq = readCLI(sys.argv[1:])
    if fq==0:
        print "Invalid frequency spec!"
    else:
        print "Setting F={}-MHz to chip <{},{}>".format(fq,cX,cY)
        udpSocketOut = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sendCmd(udpSocketOut, cX, cY, fq)
        udpSocketOut.close()
    
if __name__=='__main__':
    main()
    
