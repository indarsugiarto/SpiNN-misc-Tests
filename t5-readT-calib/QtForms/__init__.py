"""
OVERVIEW:
    This will handle both the main window that provides:
    - console-like view for SpiNNaker temperature data
    - console-like view for Raspberry-pi temperature data
    - plotter for the resulting calibration
    - save data (from both SpiNN and Raspi) to a file

SYNOPSIS:
    This main window has two sub window: SpiNNaker widget (sw) and Raspberry widget (pw).
    sw receives data from SpiNNaker and emit it to MainWindow.
    pw receives data from Raspberry-pi and emit it to MainWindow.
    MainWindow then processes the data (such as regression for calibration), and
    then forward the resulting data to the plotter (tw).
    MainWindow also saves the data in a log file.

TODO:
    Kalman Filter for temperature fusion/tracking?
"""

from PyQt4 import QtGui, QtNetwork
from PyQt4.QtCore import *
import QtMainWindow
from myPlotterT import Twidget
from mySpiNN import *  # it has UDP protocol involved
from myRaspi import *  # it has TCP protocol involved
import constDef as DEF # the calibration constants should be defined here
import time
import os
import math
from PyQt4.Qwt5.anynumpy import * # need concatenate

"""===============================================================================================
                                             MainGUI
-----------------------------------------------------------------------------------------------"""
class MainWindow(QtGui.QMainWindow, QtMainWindow.Ui_QtMainWindow):
    """ It inherits QMainWindow and uses pyuic4-generated python files from the .ui """
    pltDataRdy = QtCore.pyqtSignal(list)  # for streaming data to the plotter/calibrator

    def __init__(self, parent=None):
        super(MainWindow, self).__init__(parent)
        self.setupUi(self)
        self.setCentralWidget(self.mdiArea);
        self.statusTxt = QtGui.QLabel("")
        self.statusBar().addWidget(self.statusTxt)


        """
        Scenario: 
        - sw and pw send data to MainWindow
        - MainWindow do the averaging on sw data, then send avg_sw and pw to plotter
          At the same time, MainWindow store the data
        """

        #-------------------- GUI setup ---------------------               
        self.sw = wSpiNN(self)
        self.subWinSpin = QtGui.QMdiSubWindow(self)
        self.subWinSpin.setWidget(self.sw)
        self.mdiArea.addSubWindow(self.subWinSpin)
        self.subWinSpin.setGeometry(0,0,self.sw.width(),500)
        self.sw.show()
        self.sw.spinUpdate.connect(self.readSpiNN)
        self.sw.spinChipID.connect(self.readSpinChipID)

        self.pw = wRaspi(self)
        self.subWinRaspi = QtGui.QMdiSubWindow(self)
        self.subWinRaspi.setWidget(self.pw)
        self.mdiArea.addSubWindow(self.subWinRaspi)
        self.subWinRaspi.setGeometry(0,self.sw.height()+30,self.pw.width(),500)
        self.pw.show()
        self.pw.piUpdate.connect(self.readRaspi)

        """
        Temperature plot. Each instance of this widget corresponds to only one chip in Spin3.
        """
        self.tw0 = Twidget(0, self);
        self.pltDataRdy.connect(self.tw0.readPltData) 
        self.subWinTW0 = QtGui.QMdiSubWindow(self)
        self.subWinTW0.setWidget(self.tw0)
        self.mdiArea.addSubWindow(self.subWinTW0)
        self.subWinTW0.setGeometry(self.sw.width()+10,0,760,500)
        self.tw0.show()

        self.tw1 = Twidget(1, self); 
        self.pltDataRdy.connect(self.tw1.readPltData)
        self.subWinTW1 = QtGui.QMdiSubWindow(self)
        self.subWinTW1.setWidget(self.tw1)
        self.mdiArea.addSubWindow(self.subWinTW1)
        self.subWinTW1.setGeometry(self.subWinTW0.x()+self.subWinTW0.width(),0,760,500)
        self.tw1.show()

        self.tw2 = Twidget(2, self); 
        self.pltDataRdy.connect(self.tw2.readPltData)
        self.subWinTW2 = QtGui.QMdiSubWindow(self)
        self.subWinTW2.setWidget(self.tw2)
        self.mdiArea.addSubWindow(self.subWinTW2)
        self.subWinTW2.setGeometry(self.subWinTW0.x(),self.subWinTW0.y()+self.subWinTW0.height(),760,500)
        self.tw2.show()

        self.tw3 = Twidget(3, self); 
        self.pltDataRdy.connect(self.tw3.readPltData)
        self.subWinTW3 = QtGui.QMdiSubWindow(self)
        self.subWinTW3.setWidget(self.tw3)
        self.mdiArea.addSubWindow(self.subWinTW3)
        self.subWinTW3.setGeometry(self.subWinTW1.x(),self.subWinTW2.y(),760,500)
        self.tw3.show()


        # For each chip, prepare dummy containers for streamed data
        self.C0raw = list() 
        self.C1raw = list() 
        self.C2raw = list() 
        self.C3raw = list() 
        self.collected_SpiNN_raw = 0

        # Prepare total Tdata container from SpiNNaker
        self.Tspin = [[0 for sensor in range(3)] for chip in range(4)] # creates 4 rows 3 cols array
        # Hence, Tspin[1][2] corresponds to chip-1 sensor-2 (Note: index starts from 0)

        # ADDITIONAL (for debugging): the callibrated (in degree/centigrade) version of Tspin
        self.TspinDeg = [[0 for sensor in range(3)] for chip in range(4)] # creates 4 rows 3 cols array

        # and the container for Tdata from Raspi
        self.Tpi = list()

        # Finally, send the data to the plotter using the following format:
        """
        TPlotData is a combination of Tpi, Tspin and TspinDeg.
        Hence, TPlotData will be a 4-rows-7-cols list with the following format:
        Pi-0 | Raw-00 | Raw-01 | Raw-02 | Deg-00 | Deg-01 | Deg-02
        Pi-1 | Raw-10 | Raw-11 | Raw-12 | Deg-10 | Deg-11 | Deg-12
        Pi-2 | Raw-20 | Raw-21 | Raw-22 | Deg-20 | Deg-21 | Deg-22
        Pi-3 | Raw-30 | Raw-31 | Raw-32 | Deg-30 | Deg-31 | Deg-32
        """
        self.TPlotData = [[0 for _ in range(7)] for chip in range(4)] # creates 4 rows 7 cols array


        # Preparing experimental folder:
        print "Preparing {}".format(DEF.LOG_DIR)
        str = "mkdir -p {}".format(DEF.LOG_DIR)
        os.system(str)
        # Open logfile
        fname = DEF.LOG_DIR + time.strftime("%b_%d_%Y-%H.%M.%S", time.gmtime()) + ".log"
        self.logFile = open(fname, "w")
        print fname

        """
        # Some debug info:
        if DEF.regCoef==DEF.regCoef1:
            print "Using Cooling data from ptDemo"
        else:
            print "Using Cooling data from worseC"
        """
        self.rmse = [0 for _ in range(3)] # for comparison between data from ptDemo and worseC; for 3 sensors

    """
    ######################### GUI callback ########################### 
    """

    @pyqtSlot(list)
    def readSpinChipID(self, chipIDxInfo):
        """
        SpiNNaker widget will send chip ID configuration: [chipID, sax, say]
        TODO: tell plotter
        """
        self.tw0.setChipIdInfo(chipIDxInfo)
        self.tw1.setChipIdInfo(chipIDxInfo)
        self.tw2.setChipIdInfo(chipIDxInfo)
        self.tw3.setChipIdInfo(chipIDxInfo)

    @pyqtSlot(list)
    def readSpiNN(self, Tdata):
        """
        SpiNNaker widget will send data as a list. Tdata contains [chipID, S0, S1, S2]
        TODO: accumulate
        """
        # just for debugging:
        # print Tdata[0], ":", Tdata[1:4]

        # get sensor data:
        Sdata = Tdata[1:4]

        if Tdata[0] == 0:
            self.C0raw.append(Sdata)
            self.collected_SpiNN_raw += 1
        elif Tdata[0] == 1:
            self.C1raw.append(Sdata)
            self.collected_SpiNN_raw += 1
        elif Tdata[0] == 2:
            self.C2raw.append(Sdata)
            self.collected_SpiNN_raw += 1
        else:
            self.C3raw.append(Sdata)
            self.collected_SpiNN_raw += 1

        # if Raspberry pi is not connected, then send the raw SpiNNaker data to plotter
        # to see the effect directly
        # print self.sw.isReady
        if self.collected_SpiNN_raw >= 4:
            if self.sw.isReady is False:
                # if all chips have reported, then send fill TPlotData and send it
                self.TPlotData[0] = concatenate(([0],self.C0raw[0],[0,0,0]))
                self.TPlotData[1] = concatenate(([0],self.C1raw[0],[0,0,0]))
                self.TPlotData[2] = concatenate(([0],self.C2raw[0],[0,0,0]))
                self.TPlotData[3] = concatenate(([0],self.C3raw[0],[0,0,0]))
    
                self.pltDataRdy.emit(self.TPlotData)
                # Then reset dummy container
                self.collected_SpiNN_raw = 0
                self.C0raw = list()
                self.C1raw = list()
                self.C2raw = list()
                self.C3raw = list()

    @pyqtSlot(list)
    def readRaspi(self, Tdata):
        """
        Raspi widget will send data as a list
        TODO: averaging spin data, send to plotter, and save to a file
        """
        # print "Receiving {} data".format(len(Tdata))
        # print Tdata
        self.Tpi = Tdata

        # Step-1: average Tspin
        sum = [ [0 for _ in range(3)] for _ in range(4)]

        # For chip-0
        for i in range(len(self.C0raw)):
            for s in range(3):
                sum[0][s] += self.C0raw[i][s]
        for s in range(3):
            sum[0][s] /= len(self.C0raw)
        print '0: ', sum[0]

        # For chip-1
        for i in range(len(self.C1raw)):
            for s in range(3):
                sum[1][s] += self.C1raw[i][s]
        for s in range(3):
            sum[1][s] /= len(self.C1raw)
        print '1: ', sum[1]

        # For chip-2
        for i in range(len(self.C2raw)):
            for s in range(3):
                sum[2][s] += self.C2raw[i][s]
        for s in range(3):
            sum[2][s] /= len(self.C2raw)
        print '2: ', sum[2]

        # For chip-3
        for i in range(len(self.C3raw)):
            for s in range(3):
                sum[3][s] += self.C3raw[i][s]
        for s in range(3):
            sum[3][s] /= len(self.C3raw)
        print '3: ', sum[3]

        # Step-2: put into Tspin
        for c in range(4):
            self.Tspin[c] = sum[c]
            print '%d: [%d, %d, %d]' % (c, self.Tspin[c][0], self.Tspin[c][1], self.Tspin[c][2])
        # print self.Tspin

        # Step-3: process the calibration and send to the plotters
        self.processData()

        # Step-4: reset dummy container
        self.C0raw = list()
        self.C1raw = list()
        self.C2raw = list()
        self.C3raw = list()

        # Step-5: save data
        self.SaveData()

        # Step-6: send to plotters
        

    def SaveData(self):
        """
        Works on Tpi and Tspin. Save as follows:
        Tpi0,Tpi1,Tpi2,Tpi3,Tspin00,Tspin01,Tspin02,Tspin10,Tspin11,Tspin12,...
        """
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

    def processData(self):
        """
        Takes Tspin, produces TspinDeg and combine with Tpi to produce TPlotData.
        Then send to the plotters.
        Note: the format of TPlotData is:
              Pi-0 | Raw-00 | Raw-01 | Raw-02 | Deg-00 | Deg-01 | Deg-02
              Pi-1 | Raw-10 | Raw-11 | Raw-12 | Deg-10 | Deg-11 | Deg-12
              Pi-2 | Raw-20 | Raw-21 | Raw-22 | Deg-20 | Deg-21 | Deg-22
              Pi-3 | Raw-30 | Raw-31 | Raw-32 | Deg-30 | Deg-31 | Deg-32

        """
        for c in range(4): # for all chips in Spin3

            # Compute regression:
            """
            S = self.Tspin[c]
            S[0] = DEF.regCoef[0][0]*S[0] + DEF.regCoef[0][1]
            S[1] = DEF.regCoef[1][0]*S[1] + DEF.regCoef[1][1]
            S[2] = DEF.regCoef[2][0]*(S[2]**3) + DEF.regCoef[2][1]*(S[2]**2) + DEF.regCoef[2][2]*S[2] + DEF.regCoef[2][3]
            """
            chipData = c  # Use each original chip data
            # chipData = 1	# Use only data from chip-1
            S = [None for _ in range(3)]
            # Using matlab refined regression, where z = (x-mu)/sigma
            z = (self.Tspin[chipData][0] - DEF.regCoef[chipData][0][2]) / DEF.regCoef[chipData][0][3]
            S[0] = DEF.regCoef[chipData][0][0]*z + DEF.regCoef[chipData][0][1]
            z = (self.Tspin[chipData][1] - DEF.regCoef[chipData][1][2]) / DEF.regCoef[chipData][1][3]
            S[1] = DEF.regCoef[chipData][1][0]*z + DEF.regCoef[chipData][1][1]
            z = (self.Tspin[chipData][2] - DEF.regCoef[chipData][2][4]) / DEF.regCoef[chipData][2][5]
            S[2] = DEF.regCoef[chipData][2][0]*(z ** 3) + \
                   DEF.regCoef[chipData][2][1]*(z ** 2) + \
                   DEF.regCoef[chipData][2][2]*(z) + DEF.regCoef[chipData][2][3]

            """
            # Using unrefined matlab regression functions:
            S[0] = DEF.regCoef[c][0][0]*self.Tspin[c][0] + DEF.regCoef[c][0][1]
            S[1] = DEF.regCoef[c][1][0]*self.Tspin[c][1] + DEF.regCoef[c][1][1]
            S[2] = DEF.regCoef[c][2][0]*(self.Tspin[c][2] ** 3) + \
                   DEF.regCoef[c][2][1]*(self.Tspin[c][2] ** 2) + \
                   DEF.regCoef[c][2][2]*(self.Tspin[c][2]) + DEF.regCoef[c][2][3]
            """
            
            self.TPlotData[c][0] = self.Tpi[c]
            for s in range(3):
                self.TPlotData[c][s+1] = self.Tspin[c][s]
                self.TPlotData[c][s+4] = S[s]


        self.pltDataRdy.emit(self.TPlotData)

        #------------- Additional debugging ---------------
        # See the result on console for debugging:
        # print self.Tspin
        for c in range(4):
            print "Chip-%d --> [Pi: %f] [int: %d, %d, %d] [deg: %f, %f, %f]" % (c, self.TPlotData[c][0], 
                  self.TPlotData[c][1], self.TPlotData[c][2], self.TPlotData[c][3],
                  self.TPlotData[c][4], self.TPlotData[c][5], self.TPlotData[c][6])

        """
        Debugging using SSE to see which data is better, from ptDemo or worseC        
        Example results:
        - Using ptDemo: SSE:  [1.6582199958358708, 1.4263293459969282, 1.7199070902035638]
        - Using worseC: SSE:  [1.6319509872915254, 1.317886926475081, 1.2023682174342858]
        It seems that worseC data provides "better" coverage data than ptDemo!
        """
        for s in range(3):
            sse = 0 # sum squared error
            for c in range(4): # for each sensor-s in all chips
                e = self.TPlotData[c][0] - self.TPlotData[c][s+4] # error value
                se = e**2                                          # squared error
                sse += se
            self.rmse[s] = math.sqrt(sse)
        print "SSE: ", self.rmse

    def closeEvent(self, event):
        """
        Don't forget to close log file
        """
        self.logFile.close()
        event.accept()


