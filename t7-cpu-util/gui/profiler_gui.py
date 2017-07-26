#!/usr/bin/env python
"""
SYNOPSIS:
    Read and plot SpiNNaker Profile.
    Created on 25 July 2017
    @author: indar.sugiarto@manchester.ac.uk

TODO:
    if SpiNN-4 is used, then power profiler might be displayed as well
"""

from PyQt4 import Qt
from QtForms import MainWindow
import sys
import argparse


if __name__=='__main__':
    # Parse command line argument:
    parser = argparse.ArgumentParser(description='Read and plot SpiNNaker profile')
    parser.add_argument("-ip", type=str, help="ip address of SpiNN-4 machine")
    parser.add_argument("-n","--logName",type=str, help="log filename")
    args = parser.parse_args()
    if args.ip is not None:
        print "SpiNNaker machine is at", args.ip
    else:
        print "No SpiNNaker is specified"
        sys.exit(-1)

    myApp = Qt.QApplication(sys.argv)
    myMainWindow = MainWindow(args.ip,args.logName)
    myMainWindow.show()
    #myMainWindow.showMaximized()
    sys.exit(myApp.exec_())

