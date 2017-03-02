'''
Created on 2 Nov 2015

@author: indi
'''
import random
import struct
from PyQt4 import Qt, QtGui, QtCore, QtNetwork
import PyQt4.Qwt5 as Qwt
from PyQt4.Qwt5.anynumpy import *
import constDef as DEF

class Twidget(QtGui.QWidget):
    """
    Temperature plot. Each instance of this widget corresponds to only one chip in Spin3.
    """
    def __init__(self, chipID, parent=None):
        """
        Layout: upper row for raw data, lower row for the callibrated + pi Data
        """
        QtGui.QWidget.__init__(self, parent)
                
        # Get the chipID number used for extracting data signalled by main GUI
        self.chipID = chipID
        self.chipIDInfo = [[0, 0] for i in range(4)]

        # Create qwt plotters for each sensors. They plot raw (integer) and calibrated (in degree) data
        self.qwtTraw0 = TRawPlot(chipID,0); self.qwtTdeg0 = TDegPlot(chipID,0) # For sensor-0
        self.qwtTraw1 = TRawPlot(chipID,1); self.qwtTdeg1 = TDegPlot(chipID,1) # For sensor-1
        self.qwtTraw2 = TRawPlot(chipID,2); self.qwtTdeg2 = TDegPlot(chipID,2) # For sensor-2
        
        hLayout0 = QtGui.QHBoxLayout()
        hLayout0.addWidget(self.qwtTraw0)
        hLayout0.addWidget(self.qwtTraw1)
        hLayout0.addWidget(self.qwtTraw2)

        hLayout1 = QtGui.QHBoxLayout()
        hLayout1.addWidget(self.qwtTdeg0)
        hLayout1.addWidget(self.qwtTdeg1)
        hLayout1.addWidget(self.qwtTdeg2)

        vLayout = QtGui.QVBoxLayout()
        vLayout.addLayout(hLayout0)
        vLayout.addLayout(hLayout1)
        self.setLayout(vLayout)
        ttl = "Temperature Report for Chip-%d" % chipID
        self.setWindowTitle(ttl)

        self.pltData = None # the extracted data sent by receiver program in main GUI
                
    @QtCore.pyqtSlot(list)
    def readPltData(self, Tdata):
        """
        This is a slot that will be called by the mainGUI. Tdata is a 4-rows-7-cols list with the following 
        format:
        Pi-0 | Raw-00 | Raw-01 | Raw-02 | Deg-00 | Deg-01 | Deg-02
        Pi-1 | Raw-10 | Raw-11 | Raw-12 | Deg-10 | Deg-11 | Deg-12
        Pi-2 | Raw-20 | Raw-21 | Raw-22 | Deg-20 | Deg-21 | Deg-22
        Pi-3 | Raw-30 | Raw-31 | Raw-32 | Deg-30 | Deg-31 | Deg-32
        """

        # Pick only the corresponding chip and send them to qwt!
        self.pltData = Tdata[self.chipID]
        self.qwtTraw0.getData(self.pltData)
        self.qwtTraw1.getData(self.pltData)
        self.qwtTraw2.getData(self.pltData)
        self.qwtTdeg0.getData(self.pltData)
        self.qwtTdeg1.getData(self.pltData)
        self.qwtTdeg2.getData(self.pltData)

    @QtCore.pyqtSlot(list)
    def setChipIdInfo(self, chipIDxInfo):
        """
        Format chipIDxInfo: [chipID, sax, say]
        """
        #print "chipIDxInfo: ", chipIDxInfo
        cid = chipIDxInfo[0]; sax = chipIDxInfo[1]; say = chipIDxInfo[2]
        self.chipIDInfo[cid][0] = sax
        self.chipIDInfo[cid][1] = say
        if cid==self.chipID:
            ttl = "Temperature Report for Chip-%d <%d,%d>" % (self.chipID, sax, say)
            self.setWindowTitle(ttl)
        


#----------------------------------------------------------------------------------------
#-------------------------------------- Class TRawPlot ----------------------------------
    
class TRawPlot(Qwt.QwtPlot):
    def __init__(self, chipID, sensorID, *args):
        """
        What is chipID used for? Actually chipID is mainly used by the parent class Twidget.
        Here it is just for labeling.
        """
        Qwt.QwtPlot.__init__(self, *args)
       
        self.sID = sensorID
        self.chipID = chipID

        self.setCanvasBackground(Qt.Qt.white)
        self.alignScales()

        # Initialize data
        self.x = arange(0.0, 100.1, 0.5)
        self.y = zeros(len(self.x), Float)
        self.z = zeros(len(self.x), Float)
        self.maxScaleY = DEF.MAX_T_SCALE_Y_INT 
        self.minScaleY = DEF.MIN_T_SCALE_Y_INT
        self.currentMaxYVal = 0
        self.currentMinYVal = DEF.MAX_T_SCALE_Y_INT
               
        self.t = zeros(len(self.x), Float) # contains the real sensor value
        sname = "Chip-%d" % (chipID)
        self.c = Qwt.QwtPlotCurve(sname)

        
        self.c.setPen(Qt.QPen(DEF.clr[sensorID],DEF.PEN_WIDTH))
        self.c.attach(self)
        
        sname = "Raw data from sensor-%d" % (sensorID)
        self.setTitle(sname)
        
        # self.insertLegend(Qwt.QwtLegend(), Qwt.QwtPlot.BottomLegend);

        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)        
        self.setAxisTitle(Qwt.QwtPlot.xBottom, "Time (seconds)")
        self.setAxisTitle(Qwt.QwtPlot.yLeft, "Values")
        #self.setAxisAutoScale(Qwt.QwtPlot.yLeft)
        
    def getAlignedMaxValue(self):
        mVal = int(self.currentMaxYVal / 1000) + 1
        return mVal * 1000

    def getAlignedMinValue(self):
        mVal = (self.currentMinYVal / 1000) * 1000
        return mVal
    
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

    
    def getData(self, Tdata):   
        """
        Pi-x | Raw-x0 | Raw-x1 | Raw-x2 | Deg-x0 | Deg-x1 | Deg-x2

        where x is the chip-id
        """
        # This class is interested only in one specific Raw-xy
        temp = Tdata[self.sID+1] # skip the Pi-x part

        # Find out the proper scale for the plot
        if temp > self.currentMaxYVal:
            self.currentMaxYVal = temp
            self.maxScaleY = self.getAlignedMaxValue()
            #self.minScaleY = DEF.MIN_T_SCALE_Y_INT

        if temp < DEF.MIN_T_SCALE_Y_INT:
            self.currentMinYVal = temp
            self.minScaleY = self.getAlignedMinValue()
        else:
            self.minScaleY = DEF.MIN_T_SCALE_Y_INT

        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)
        if self.sID==0:
            print "Scale: ", self.minScaleY, self.maxScaleY

        #self.t = concatenate((self.t[:1], self.t[:-1]), 1)
        self.t = concatenate(([temp],self.t[:-1]))
            
        self.c.setData(self.x, self.t)

        #self.setAxisAutoScale(Qwt.QwtPlot.yLeft)
        self.replot()                


#----------------------------------------------------------------------------------------
#-------------------------------------- Class TDegPlot ----------------------------------
    
class TDegPlot(Qwt.QwtPlot):
    def __init__(self, chipID, sensorID, *args):
        Qwt.QwtPlot.__init__(self, *args)
       
        self.sID = sensorID

        self.setCanvasBackground(Qt.Qt.white)
        self.alignScales()

        # Initialize data
        self.x = arange(0.0, 100.1, 0.5)
        self.y = zeros(len(self.x), Float)
        self.z = zeros(len(self.x), Float)
        self.maxScaleY = DEF.MAX_T_SCALE_Y_DEG
        self.minScaleY = DEF.MIN_T_SCALE_Y_DEG
        self.currentMaxYVal = 0
        
        self.p = zeros(len(self.x), Float) # contains raspberry value (in degree)
        self.t = zeros(len(self.x), Float) # contains the SpiNNaker sensor value (in degree)
        #sname = "Raspi data for chip-%d" % (chipID)
        sname = "Raspi data"
        self.cp = Qwt.QwtPlotCurve(sname)  # curve for the raspberry pi
        #sname = "SpiNN data for chip-%d" % (chipID)
        sname = "SpiNN data"
        self.ct = Qwt.QwtPlotCurve(sname)  # curve for the spinnaker

        ## Color contants
        clr = [Qt.Qt.red, Qt.Qt.green, Qt.Qt.blue, Qt.Qt.cyan, Qt.Qt.magenta, Qt.Qt.yellow, Qt.Qt.black]
        
        # Attach the curve container
        self.cp.setPen(Qt.QPen(DEF.clr[3],DEF.PEN_WIDTH))
        self.cp.attach(self)
        self.ct.setPen(Qt.QPen(DEF.clr[sensorID],DEF.PEN_WIDTH))
        self.ct.attach(self)
        
        sname = "Sensor-%d" % (sensorID)
        self.setTitle(sname)
        
        self.insertLegend(Qwt.QwtLegend(), Qwt.QwtPlot.BottomLegend);

        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)        
        self.setAxisTitle(Qwt.QwtPlot.xBottom, "Time (seconds)")
        self.setAxisTitle(Qwt.QwtPlot.yLeft, "Values")
            

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

    def getData(self, Tdata):       
        """
        Pi-x | Raw-x0 | Raw-x1 | Raw-x2 | Deg-x0 | Deg-x1 | Deg-x2

        where x is the chip-id
        """
        # This class is interested only in Raspi data and one specific SpiNN Raw-xy

        # For Raspi data:
        temp = Tdata[0] # skip the Pi-x and Raw-xy parts
        #self.p = concatenate((self.p[:1], self.p[:-1]), 1)
        self.p = concatenate((self.p[:1], self.p[:-1]), 0)
        # Find out the proper scale for the plot
        if temp > self.currentMaxYVal:
            self.currentMaxYVal = temp
            self.minScaleY = DEF.MIN_T_SCALE_Y_DEG
            self.maxScaleY = int(temp) + 1
            self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)
        self.p[0] = temp
        self.cp.setData(self.x, self.p)


        # For SpiNN data:
        temp = Tdata[self.sID+4] # skip the Pi-x and Raw-xy parts
        #self.t = concatenate((self.t[:1], self.t[:-1]), 1)
        self.t = concatenate((self.t[:1], self.t[:-1]), 0)
        # Find out the proper scale for the plot
        if temp > self.currentMaxYVal:
            self.currentMaxYVal = temp
            self.minScaleY = DEF.MIN_T_SCALE_Y_DEG
            self.maxScaleY = int(temp) + 1
            self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)
        self.t[0] = temp
        self.ct.setData(self.x, self.t)

        #self.setAxisAutoScale(Qwt.QwtPlot.yLeft)
        self.replot()

    def wheelEvent(self, e):
        numDegrees = e.delta() / 8
        numSteps = numDegrees / 15  # will produce either +1 or -1
        maxY = self.maxScaleY - (numSteps*5)
        if maxY > DEF.MAX_T_SCALE_Y_DEG:
            maxY = DEF.MAX_T_SCALE_Y_DEG
        if maxY < DEF.MIN_T_SCALE_Y_DEG+10:
            maxY = DEF.MIN_T_SCALE_Y_DEG+10
        self.maxScaleY = maxY
        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)


