'''
Created on 2 Nov 2015

@author: indi
'''

import random
import struct
from PyQt4 import Qt, QtGui, QtCore, QtNetwork
import PyQt4.Qwt5 as Qwt
#from PyQt4.Qwt5.anynumpy import *
from numpy import *
import constDef as DEF

class Pwidget(QtGui.QWidget):
    """
    This is for plotting Arduino power data. It displays the following data
    "Input Power 12V (W)", "BMP 3.3V (W)", "SDRAM and each SpiNN-link 1.8V (W)", "FPGA (W)",
    "Bank C 1.2V (W)", "Bank B 1.2V (W)", "Bank A 1.2V (W)"
    """
    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)

        # Use auto-scale or fix-scale?
        self.scaleBtn = QtGui.QPushButton()
        self.scaleBtn.setCheckable(True)
        self.scaleBtn.setChecked(False)
        self.scaleBtn.setFlat(False)
        self.scaleBtn.setText("Use Auto Scale")
        self.scaleBtn.clicked.connect(self.scaleBtnClicked)

        # Create qwt plotters for each DC-DC converter. 
        self.qwtBankA = pwrPlot(0, 'Bank-A', DEF.MAX_PWR_SCALE_MED)
        self.qwtBankB = pwrPlot(1, 'Bank-B', DEF.MAX_PWR_SCALE_MED)
        self.qwtBankC = pwrPlot(2, 'Bank-C', DEF.MAX_PWR_SCALE_MED)
        self.qwtFPGA = pwrPlot(3,'FPGA', DEF.MAX_PWR_SCALE_UNDER)
        self.qwtSDRAM = pwrPlot(4,'SDRAM', DEF.MAX_PWR_SCALE_MED)
        self.qwtBMP = pwrPlot(5,'BMP', DEF.MAX_PWR_SCALE_MED)
        self.qwt12V = pwrPlot(6, '12V', DEF.MAX_PWR_SCALE_HIGH)
        # additionally, we want to see all cores (from BankA to BankC)
        self.qwtCores = pwrPlot(7,'All Cores', DEF.MAX_PWR_SCALE_HIGH) # just a combination of Bank-A, Bank-B, and Bank-C
        
        hLayout0 = QtGui.QHBoxLayout()
        hLayout0.addWidget(self.qwtBankA)
        hLayout0.addWidget(self.qwtBankB)
        hLayout0.addWidget(self.qwtBankC)

        hLayout1 = QtGui.QHBoxLayout()
        hLayout1.addWidget(self.qwtFPGA)
        hLayout1.addWidget(self.qwtSDRAM)
        hLayout1.addWidget(self.qwtBMP)

        hLayout2 = QtGui.QHBoxLayout()
        hLayout2.addWidget(self.qwt12V)
        hLayout2.addWidget(self.qwtCores)
        hLayout2.addWidget(self.scaleBtn)

        vLayout = QtGui.QVBoxLayout()
        vLayout.addLayout(hLayout0)
        vLayout.addLayout(hLayout1)
        vLayout.addLayout(hLayout2)

        self.setLayout(vLayout)
        ttl = "Power Consumption of SpiNN-4"
        self.setWindowTitle(ttl)

        self.pltData = [0.0 for _ in range(8)] # the extracted data sent by receiver program in main GUI

    @QtCore.pyqtSlot()
    def scaleBtnClicked(self):
        if self.scaleBtn.isChecked() is True:
            #print "Checked"
            self.scaleBtn.setText("Use Fix Scale")
            self.qwtBankA.useAutoScale = True
            self.qwtBankB.useAutoScale = True
            self.qwtBankC.useAutoScale = True
            self.qwtFPGA.useAutoScale = True
            self.qwtSDRAM.useAutoScale = True
            self.qwtBMP.useAutoScale = True
            self.qwt12V.useAutoScale = True
            self.qwtCores.useAutoScale = True
        else:
            #print "Unchecked"
            self.scaleBtn.setText("Use Auto Scale")
            self.qwtBankA.useAutoScale = False
            self.qwtBankB.useAutoScale = False
            self.qwtBankC.useAutoScale = False
            self.qwtFPGA.useAutoScale = False
            self.qwtSDRAM.useAutoScale = False
            self.qwtBMP.useAutoScale = False
            self.qwt12V.useAutoScale = False
            self.qwtCores.useAutoScale = False

    @QtCore.pyqtSlot(list)
    def readPltData(self, Pdata):
        """
        This is a slot that receives Arduino power data. It has the following format:
        [BankA, BankB, BankC, FPGA, SDRAM, BMP, All]
        """
        self.pltData[:7] = Pdata
        # then compute additional "all-cores" data
        self.pltData[7] = Pdata[0] + Pdata[1] + Pdata[2]
        #print self.pltData

        # give to the plotter:
        self.qwtBankA.getData(self.pltData[0])
        self.qwtBankB.getData(self.pltData[1])
        self.qwtBankC.getData(self.pltData[2])
        self.qwtFPGA.getData(self.pltData[3])
        self.qwtSDRAM.getData(self.pltData[4])
        self.qwtBMP.getData(self.pltData[5])
        self.qwt12V.getData(self.pltData[6])
        self.qwtCores.getData(self.pltData[7])

#----------------------------------------------------------------------------------------
#-------------------------------------- Class pwrPlot ----------------------------------
    
class pwrPlot(Qwt.QwtPlot):
    def __init__(self, id, label, mx_scale, *args):
        Qwt.QwtPlot.__init__(self, *args)
       
        self.setCanvasBackground(Qt.Qt.white)
        self.alignScales()

        self.Label = label
        self.pltID = id

        self.useAutoScale = False

        # Initialize data
        self.x = arange(0.0, 100.1, 0.5)
        #self.y = zeros(len(self.x), Float)
        #self.z = zeros(len(self.x), Float)
        self.y = zeros(len(self.x), float)
        self.z = zeros(len(self.x), float)
        self.maxScaleY = mx_scale
        self.minScaleY = DEF.MIN_PWR_SCALE

        #self.t = zeros(len(self.x), Float) # contains the real sensor value
        self.t = zeros(len(self.x), float)
        self.c = Qwt.QwtPlotCurve(label)

        
        self.c.setPen(Qt.QPen(DEF.clr[id],DEF.PEN_WIDTH))
        self.c.attach(self)
        
        sname = "%s (Watt)" % (label)
        self.setTitle(sname)
        
        # self.insertLegend(Qwt.QwtLegend(), Qwt.QwtPlot.BottomLegend);

        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)        
        self.setAxisTitle(Qwt.QwtPlot.xBottom, "Time (seconds)")
        self.setAxisTitle(Qwt.QwtPlot.yLeft, "Values")

        # by default use fix scale
        self.useAutoScale = False
        self.setScale()


    def setScale(self):
        if self.useAutoScale is True:
            self.setAxisAutoScale(Qwt.QwtPlot.yLeft)
        else:
            self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)

    def alignScales(self):
        self.canvas().setFrameStyle(Qt.QFrame.Box | Qt.QFrame.Plain)
        self.canvas().setLineWidth(1)
        for i in range(Qwt.QwtPlot.axisCnt):
            scaleWidget = self.axisWidget(i)
            
            if scaleWidget:
                scaleWidget.setMargin(0)
            scaleDraw = self.axisScaleDraw(i)
            if scaleDraw:
                scaleDraw.enableComponent(
                    Qwt.QwtAbstractScaleDraw.Backbone, False)

    
    def getData(self, Pdata):
        # This class is interested only in one specific Raw-xy
        temp = Pdata

        #self.t = concatenate((self.t[:1], self.t[:-1]), 1)
        self.t = concatenate(([temp],self.t[:-1]))
            
        self.c.setData(self.x, self.t)

        self.setScale()

        #self.setAxisAutoScale(Qwt.QwtPlot.yLeft)
        self.replot()                

    def wheelEvent(self, e):
        return
        numDegrees = e.delta() / 8
        numSteps = numDegrees / 15  # will produce either +1 or -1
        maxY = self.maxScaleY - (numSteps*5)
        if maxY > self.maxScaleY:
            maxY = self.maxScaleY
        if maxY < 0:
            maxY = 0
        self.maxScaleY = maxY
        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)


