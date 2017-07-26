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

class Swidget(QtGui.QWidget):
    """
    This is for plotting SpiNNaker profiler data.

    This plotter has the following layout:
    Top row: checkbox (for selecting which chip will be included) and number of active cores
    Second row: temperature plot
    Third row: frequency plot
    """
    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)

        # Row 1: checkbox and line edit
        CHIPLabel = QtGui.QLabel("Select a Chip", self)
        CORELabel = QtGui.QLabel("#Active Cores", self)
        self.cbChip = QtGui.QComboBox(self); self.populateSelectChip()
        self.cbChip.currentIndexChanged.connect(self.changeChip)
        self.leCore = QtGui.QLineEdit(self); self.leCore.setText("18")
        hLayout00 = QtGui.QHBoxLayout()
        hLayout00.addWidget(CHIPLabel)
        hLayout00.addWidget(self.cbChip)
        hLayout01 = QtGui.QHBoxLayout()
        hLayout01.addWidget(CORELabel)
        hLayout01.addWidget(self.leCore)
        vLayout0 = QtGui.QVBoxLayout()
        vLayout0.addLayout(hLayout00)
        vLayout0.addLayout(hLayout01)

        # Row 2: temperature plot
        self.qwtTemp = TDegPlot()
        hLayout1 = QtGui.QHBoxLayout()
        hLayout1.addWidget(self.qwtTemp)

        # Row 3: frequency plot
        self.qwtFreq = FreqPlot()
        hLayout2 = QtGui.QHBoxLayout()
        hLayout2.addWidget(self.qwtFreq)

        vLayout = QtGui.QVBoxLayout()
        vLayout.addLayout(vLayout0)
        vLayout.addLayout(hLayout1)
        vLayout.addLayout(hLayout2)
        self.setLayout(vLayout)

        self.pltData = None # the extracted data sent by receiver program in main GUI
        self.currentChip = 0
        self.changeChip(self.currentChip)


    def populateSelectChip(self):
        for c in range(48):
            str = "<%d,%d>" % (DEF.CHIP_LIST_48[c][0], DEF.CHIP_LIST_48[c][1])
            self.cbChip.addItem(str)

    @QtCore.pyqtSlot(list)
    def changeChip(self, idx):
        self.currentChip = idx
        ttl = "SpiNNaker Profile Report for chip-{}".format(DEF.getChipName(idx))
        self.setWindowTitle(ttl)
        self.qwtTemp.setCurrentChip(idx, None)
        self.qwtFreq.setCurrentChip(idx, None)

    @QtCore.pyqtSlot(list)
    def readPltData(self, Sdata):
        """
        Here how we usually process the profile data from ifaceArduSpiNN module:
        fmt = "<2BH"
        print "{",
        for i in range(len(prfData)):
            cpu = struct.pack("<I",prfData[i])
            f,nA,T = struct.unpack(fmt, cpu)
            if i < (len(prfData)-1):
                print "[{},{},{}],".format(f, nA, T),
            else:
                print "[{},{},{}]".format(f, nA, T),
        print "}"

        """
        # Pick only the corresponding chip and send them to qwt!
        #print self.currentChip
        if self.currentChip >= len(Sdata):
            return
        self.pltData = Sdata[self.currentChip]
        fmt = "<2BH"
        cpu = struct.pack("<I", self.pltData)
        f,nA,T = struct.unpack(fmt, cpu)
        Tdeg = DEF.convert2deg(T)
        nA_str = "{}".format(nA)
        self.leCore.setText(nA_str)
        self.qwtTemp.getData(Tdeg)
        self.qwtFreq.getData(f)

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
        


# ----------------------------------------------------------------------------------------
# -------------------------------------- Class TDegPlot ----------------------------------

class TDegPlot(Qwt.QwtPlot):
    def __init__(self, *args):
        Qwt.QwtPlot.__init__(self, *args)

        self.cID = 0 # current chip index

        self.setCanvasBackground(Qt.Qt.white)
        self.alignScales()

        # Initialize data
        self.x = arange(0.0, 100.1, 0.5)
        self.y = zeros(len(self.x), float)
        self.z = zeros(len(self.x), float)
        self.maxScaleY = DEF.MAX_T_SCALE_Y_DEG
        self.minScaleY = DEF.MIN_T_SCALE_Y_DEG
        self.currentMaxYVal = 0
        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)

        self.t = zeros(len(self.x), float)  # contains the SpiNNaker sensor value (in degree)

        # Attach to the curve container
        #sname = "SpiNN data"
        #self.ct = Qwt.QwtPlotCurve(sname)  # curve for the spinnaker
        self.ct = Qwt.QwtPlotCurve()  # curve for the spinnaker
        self.ct.setPen(Qt.QPen(DEF.clr[3], DEF.PEN_WIDTH))
        self.ct.attach(self)

        #self.insertLegend(Qwt.QwtLegend(), Qwt.QwtPlot.BottomLegend);

        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)
        self.setAxisTitle(Qwt.QwtPlot.xBottom, "Time (seconds)")
        self.setAxisTitle(Qwt.QwtPlot.yLeft, "Values")

        self.setCurrentChip(self.cID, 0)

    def setCurrentChip(self, newID, newTemp):
        if newID is not None:
            self.cID = newID
        if newTemp:
            temp = "%2.1f" % newTemp
        else:
            temp = '0'
        sname = "Temperature of chip-{}: {}-deg".format(DEF.getChipName(self.cID), temp)
        self.setTitle(sname)


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
        # For SpiNN data:
        # self.t = concatenate((self.t[:1], self.t[:-1]), 1)
        self.t = concatenate((self.t[:1], self.t[:-1]), 0)
        # Find out the proper scale for the plot
        self.t[0] = Tdata
        self.ct.setData(self.x, self.t)

        # Adjust the scale (or autoscale?)
        # self.setAxisAutoScale(Qwt.QwtPlot.yLeft)
        if self.currentMaxYVal < Tdata:
            self.currentMaxYVal = Tdata
            self.maxScaleY = ((int(Tdata) / 10) + 1) * 10
            self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)

        self.replot()
        self.setCurrentChip(None, Tdata)

    def wheelEvent(self, e):
        numDegrees = e.delta() / 8
        numSteps = numDegrees / 15  # will produce either +1 or -1
        maxY = self.maxScaleY - (numSteps * 5)
        if maxY > DEF.MAX_T_SCALE_Y_DEG:
            maxY = DEF.MAX_T_SCALE_Y_DEG
        if maxY < DEF.MIN_T_SCALE_Y_DEG + 10:
            maxY = DEF.MIN_T_SCALE_Y_DEG + 10
        self.maxScaleY = maxY
        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)


#----------------------------------------------------------------------------------------
#-------------------------------------- Class TRawPlot ----------------------------------
    
class FreqPlot(Qwt.QwtPlot):
    def __init__(self, *args):
        """
        For displaying SpiNNaker core frequency.
        """
        Qwt.QwtPlot.__init__(self, *args)
       
        self.cID = 0 # initially, assume chip<0,0> is selected

        self.setCanvasBackground(Qt.Qt.white)
        self.alignScales()

        # Initialize data
        self.x = arange(0.0, 100.1, 0.5)
        self.y = zeros(len(self.x), float)
        self.z = zeros(len(self.x), float)
        self.maxScaleY = DEF.MAX_F_SCALE
        self.minScaleY = DEF.MIN_F_SCALE
        #self.currentMaxYVal = DEF.MAX_F_SCALE
        #self.currentMinYVal = DEF.MIN_F_SCALE
        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)
               
        self.f = zeros(len(self.x), float) # contains the real sensor value
        #sname = "Chip-%s" % (self.getChipName(self.chipID))
        #self.c = Qwt.QwtPlotCurve(sname)
        self.c = Qwt.QwtPlotCurve()
        self.c.setPen(Qt.QPen(DEF.clr[2],DEF.PEN_WIDTH))
        self.c.attach(self)
        
        # self.insertLegend(Qwt.QwtLegend(), Qwt.QwtPlot.BottomLegend);

        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)        
        self.setAxisTitle(Qwt.QwtPlot.xBottom, "Time (seconds)")
        self.setAxisTitle(Qwt.QwtPlot.yLeft, "Values")
        self.setAxisAutoScale(Qwt.QwtPlot.yLeft)

        self.setCurrentChip(self.cID, 200)

    def setCurrentChip(self, newID, newF):
        if newID is not None:
            self.cID = newID
        if newF:
            freq = newF
        else:
            freq = 200
        sname = "Cores frequency of chip-{}: {}-MHz".format(DEF.getChipName(self.cID), freq)
        self.setTitle(sname)


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

    
    def getData(self, Fdata):
        #self.t = concatenate((self.t[:1], self.t[:-1]), 1)
        self.f = concatenate(([Fdata],self.f[:-1]))
        self.f[0] = Fdata
        self.c.setData(self.x, self.f)

        self.replot()
        self.setCurrentChip(None, Fdata)


