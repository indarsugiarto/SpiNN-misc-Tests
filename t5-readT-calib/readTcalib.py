#!/usr/bin/env python
"""
SYNOPSIS:
    Simple framework to do experiment with chip temperature calibration. 
    Created on 2 Nov 2016
    @author: indar.sugiarto@manchester.ac.uk
"""

from PyQt4 import Qt
from QtForms import MainWindow
import sys

if __name__=='__main__':
    myApp = Qt.QApplication(sys.argv)
    myMainWindow = MainWindow()
    myMainWindow.showMaximized()
    sys.exit(myApp.exec_())

