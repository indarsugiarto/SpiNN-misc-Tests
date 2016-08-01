#!/usr/bin/python

import socket
import os

SDP_TAG_REPLY            = 1
DEF_PORT = 20001

def main():
    #prepare the socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
      sock.bind( ('', DEF_PORT) )
    except OSError as msg:
      print "%s" % msg
      sock.close()
      sock = None

    if sock is None:
      print "Cannot open the socket. Terminated!"
      return
    else:
      print "Make sure iptag-{} is set! Ready for the stream ...".format(SDP_TAG_REPLY)

      cntr = list()
      total = 0
      not_272 = 0
      Running = True
      while Running:
          try:
             data = sock.recv(1024)
             l = len(data)-10	# ignore sdp header
             if l<10:
                 Running = False
             elif l<20:
                 not_272 += 1
             else:
                 total += l
                 cntr.append(l)
          except KeyboardInterrupt:
             break
      sock.close()
      print "Total packet = {}, of {} stream, and with {} non-272 packet".format(total, len(cntr), not_272)
      #txt = "Press enter to see {} packets".format(len(cntr))
      #_ = raw_input(txt)
      #print "\n"
      #for c in cntr:
      #    print "{}".format(c)

if __name__=='__main__':
    main()

