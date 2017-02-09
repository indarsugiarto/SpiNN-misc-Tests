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
    Temperature plot
    """
    okToClose = False
    pause = False
    def __init__(self, nChip, parent=None):
        """
        Layout: top : Dropbox Sensor and Mode
                bottom: QwtPlot 
        """
        QtGui.QWidget.__init__(self, parent)
        Slabel = QtGui.QLabel("Sensor", self)
        Mlabel = QtGui.QLabel("Mode", self)
        
        # Sensor selector
        self.cbSensor = QtGui.QComboBox(self)
        sensorStr = ["1","2","3","Combined"]
        self.cbSensor.addItems(sensorStr)
        self.cbSensor.setCurrentIndex(0)
        self.cbSensor.currentIndexChanged.connect(self.changeSensor)

        # Temperature mode: raw(integer) or degree(celcius)
        self.cbMode = QtGui.QComboBox(self)
        self.cbMode.addItem("Integer")
        self.cbMode.addItem("Celcius")
        self.cbMode.setCurrentIndex(0)
        self.cbMode.currentIndexChanged.connect(self.changeMode)

        self.pbPause = QtGui.QPushButton("Pause")
        self.pbPause.clicked.connect(self.pbPauseClicked)
        
        self.qwtT = TPlot(nChip)
        #self.qwtT.sensorChanged(self.cbSensor.currentIndex())
        #self.qwtT.modeChanged(self.cbMode.currentIndex())

        """
        Somehow, the following signal-slot mechanisms don't work: 
        #self.cbSensor.currentIndexChanged.connect(self.qwtT.sensorChanged)
        #self.cbMode.currentIndexChanged.connect(self.qwtT.modeChanged)
        """        
        
        hLayout = QtGui.QHBoxLayout()
        hLayout.addWidget(Slabel)
        hLayout.addWidget(self.cbSensor)
        hLayout.addSpacing(100)
        hLayout.addWidget(Mlabel)
        hLayout.addWidget(self.cbMode)
        hLayout.addSpacing(100)
        hLayout.addWidget(self.pbPause)
        hLayout.addStretch()
        vLayout = QtGui.QVBoxLayout()
        vLayout.addLayout(hLayout)
        vLayout.addWidget(self.qwtT)
        self.setLayout(vLayout)
        self.setWindowTitle("Chip Temperature Report")
        
    def pbPauseClicked(self):
        if not self.pause:
            self.pbPause.setText("Run")
            self.qwtT.pause = True
        else:
            self.pbPause.setText("Pause")
            self.qwtT.pause = False
        self.pause = not self.pause
        
    def changeSensor(self, sID):
        self.qwtT.sensorChanged(sID)
        
    def changeMode(self, m):
        self.qwtT.modeChanged(m)
        
    @QtCore.pyqtSlot(list)
    def newData(self, datagram):
        """
        This is a slot that will be called by the mainGUI
        """
        fmt = "<HQ2H3I18I"
        pad, hdr, cmd, seq, temp1, temp2, temp3, cpu0, cpu1, cpu2, cpu3, cpu4, cpu5, cpu6, \
        cpu7, cpu8, cpu9, cpu10, cpu11, cpu12, cpu13, cpu14, cpu15, cpu16, cpu17 = struct.unpack(fmt, datagram)
        self.qwtT.readSDP(datagram)

    def saveToFileTriggered(self, state, dirName):
        self.qwtT.saveToFileTriggered(state, dirName)

    def updateNChips(self, nc):
        self.nChips = nc

    def closeEvent(self, event):
        if self.okToClose:
            event.accept()
        else:
            event.ignore()
    
class TPlot(Qwt.QwtPlot):
    tambah = 0 # Remove this later
    pause = False
    def __init__(self, nChip, *args):
        Qwt.QwtPlot.__init__(self, *args)
       
        self.setCanvasBackground(Qt.Qt.white)
        self.alignScales()

        # Initialize data
        self.x = arange(0.0, 100.1, 0.5)
        self.y = zeros(len(self.x), Float)
        self.z = zeros(len(self.x), Float)
        self.modeID = 0
        self.sensorID = 0
        self.nChips = nChip
        self.maxScaleY = DEF.MAX_T_SCALE_Y_INT 
        self.minScaleY = DEF.MIN_T_SCALE_Y_INT
        self.currentMaxYVal_S1 = 0
        self.currentMaxYVal_S2 = 0
        self.currentMaxYVal_S3 = 0
        
        # Initialize working parameters
        self.saveToFile = False
        self.Tfiles = list()
        for i in range(nChip):
            self.Tfiles.append(None)
        
        self.t = list() # contains the real sensor value
        self.d = list() # contains the temperature in degree
        self.c = list() # contains the curve value for self.t
        self.v = list() # contains the curve value for self.c
                   
        for i in range(nChip):
            y = zeros(len(self.x), Float)
            self.t.append(y)
            self.d.append(y)
            sname = "Chip-%d" % (i+1)
            self.c.append(Qwt.QwtPlotCurve(sname))
            self.v.append(Qwt.QwtPlotCurve(sname))

        ## Color contants
        clr = [Qt.Qt.red, Qt.Qt.green, Qt.Qt.blue, Qt.Qt.cyan, Qt.Qt.magenta, Qt.Qt.yellow, Qt.Qt.black]
        
        for i in range(nChip):
            self.c[i].setPen(Qt.QPen(clr[i],DEF.PEN_WIDTH))
            self.v[i].setPen(Qt.QPen(clr[i],DEF.PEN_WIDTH))
            if self.modeID == 0:
                self.c[i].attach(self)
            else:
                self.v[i].attach(self)
            
        
        self.setTitle("Chip Temperature Report")
        
        self.insertLegend(Qwt.QwtLegend(), Qwt.QwtPlot.BottomLegend);

        """
        self.curveR = Qwt.QwtPlotCurve("Data Moving Right")
        self.curveR.attach(self)
        self.curveL = Qwt.QwtPlotCurve("Data Moving Left")
        self.curveL.attach(self)

        self.curveL.setSymbol(Qwt.QwtSymbol(Qwt.QwtSymbol.Ellipse,
                                        Qt.QBrush(),
                                        Qt.QPen(Qt.Qt.yellow),
                                        Qt.QSize(7, 7)))
        self.curveR.setPen(Qt.QPen(Qt.Qt.red))
        self.curveL.setPen(Qt.QPen(Qt.Qt.blue))

        mY = Qwt.QwtPlotMarker()
        mY.setLabelAlignment(Qt.Qt.AlignRight | Qt.Qt.AlignTop)
        mY.setLineStyle(Qwt.QwtPlotMarker.HLine)
        mY.setYValue(0.0)
        mY.attach(self)
        """

        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)        
        self.setAxisTitle(Qwt.QwtPlot.xBottom, "Time (seconds)")
        self.setAxisTitle(Qwt.QwtPlot.yLeft, "Values")
    
        
        self.startTimer(50)
        self.phase = 0.0
        


    def getAlignedMaxValue(self):
        mVal = 0
        if self.sensorID == 0:
            mVal = self.currentMaxYVal_S1
        elif self.sensorID == 1:
            mVal = self.currentMaxYVal_S2
        elif self.sensorID == 2:
            mVal = self.currentMaxYVal_S3
        else:
            mVal = int((self.currentMaxYVal_S1 + self.currentMaxYVal_S2) / 2)
        mVal = int(mVal / 1000)
        mVal += 1
        return mVal * 1000
        
    
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

    # alignScales()
    
    @QtCore.pyqtSlot()
    def readSDP(self, datagram):       
        """
        fmt = "<HQ2H3I18I"
        pad, hdr, cmd, seq, temp1, temp2, temp3, cpu0, cpu1, cpu2, cpu3, cpu4, cpu5, cpu6, cpu7, cpu8, cpu9, cpu10, cpu11, cpu12, cpu13, cpu14, cpu15, cpu16, cpu17 = struct.unpack(fmt, datagram)
        sax = seq >> 8
        say = seq & 0xFF
        """
        
        fmt = "<H4BH2B2H3I18I"
        pad, flags, tag, dp, sp, da, say, sax, cmd, freq, temp1, temp2, temp3, \
        cpu0, cpu1, cpu2, cpu3, cpu4, cpu5, cpu6, cpu7, cpu8, cpu9, cpu10, \
        cpu11, cpu12, cpu13, cpu14, cpu15, cpu16, cpu17 = struct.unpack(fmt, datagram)

        """## debugging: who's the sender?
        fmt = "<H4BH2B2H3I18I"
        pad, flags, tag, dp, sp, da, sx, sy, cmd, seq, temp1, temp2, temp3, cpu0, cpu1, cpu2, cpu3, cpu4, cpu5, cpu6, cpu7, cpu8, cpu9, cpu10, cpu11, cpu12, cpu13, cpu14, cpu15, cpu16, cpu17 = struct.unpack(fmt, datagram)
        core = sp & 0x1f
        port = sp >> 5
        print "Data from [%d,%d,%d:%d]" % (sx,sy,core,port)
        ##end debugging: who's the sender?
        """
        
        # TODO: chipID will be very different with 48 boards
        #chipID = sax*2+say
        if self.nChips==4:
            cmap = DEF.CHIP_LIST_4
        else:
            cmap = DEF.CHIP_LIST_48
        chipID = self.getP2Vidx(cmap, sax, say)
        self.t[chipID] = concatenate((self.t[chipID][:1], self.t[chipID][:-1]), 1)
        self.d[chipID] = concatenate((self.d[chipID][:1], self.d[chipID][:-1]), 1)

        # Find out the proper scale for the plot
        maxValChange = False
        if temp1 > self.currentMaxYVal_S1:
            self.currentMaxYVal_S1 = temp1
            maxValChange = True           
        if temp2 > self.currentMaxYVal_S2:
            self.currentMaxYVal_S2 = temp2
            maxValChange = True
        if temp3 > self.currentMaxYVal_S3:
            self.currentMaxYVal_S3 = temp3
            maxValChange = True

        if maxValChange is True and self.modeID==0:
            self.minScaleY = DEF.MIN_T_SCALE_Y_INT
            self.maxScaleY = self.getAlignedMaxValue()
            self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)

            
        if self.sensorID == 0:
            self.t[chipID][0] = temp1
            self.d[chipID][0] = (temp1 - DEF.TEMP1_OFFSET[chipID]) / DEF.TEMP1_GRAD[chipID]
        elif self.sensorID == 1:
            self.t[chipID][0] = temp2
            self.d[chipID][0] = (temp2 - DEF.TEMP2_OFFSET[chipID]) / DEF.TEMP2_GRAD[chipID]
        elif self.sensorID == 2:
            self.t[chipID][0] = temp3
            self.d[chipID][0] = temp3   # don't want to play with exponential 
        else:
            # TODO: betulin yang ini
            val1 = (temp1 - DEF.TEMP1_OFFSET[chipID]) / DEF.TEMP1_GRAD[chipID]
            val2 = (temp2 - DEF.TEMP2_OFFSET[chipID]) / DEF.TEMP2_GRAD[chipID]
            self.t[chipID][0] = (temp1 + temp2) / 2
            self.d[chipID][0] = (val1 + val2) / 2
            
        self.c[chipID].setData(self.x, self.t[chipID])
        self.v[chipID].setData(self.x, self.d[chipID])
        #self.setAxisAutoScale(Qwt.QwtPlot.yLeft)
        if not self.pause:
            self.replot()

        if self.saveToFile is True:
            seq = sax * 2 + say           
            tVal = "{},{},{},{}\n".format(seq, temp1, temp2, temp3)
            self.Tfiles[seq].write(tVal)
    
    def timerEvent(self, e):
        return
        ##-------------- Gunakan cara berikut untuk mendisable curve --------------------
        if self.tambah == 100:
            self.c[0].detach()
        else:
            self.tambah += 1
        #--------------------------------------------------------------------------------
            
        if self.phase > pi - 0.0001:
            self.phase = 0.0
        
        # y moves from left to right:
        # shift y array right and assign new value y[0]
        self.y = concatenate((self.y[:1], self.y[:-1]), 1)
        self.y[0] = sin(self.phase) * (-1.0 + 2.0*random.random())
        
        # z moves from right to left:
        # Shift z array left and assign new value to z[n-1].
        self.z = concatenate((self.z[1:], self.z[:1]), 1)
        self.z[-1] = 0.8 - (2.0 * self.phase/pi) + 0.4*random.random()

        self.curveR.setData(self.x, self.y)
        self.curveL.setData(self.x, self.z)

        self.replot()
        self.phase += pi*0.02

    # timerEvent()

    @QtCore.pyqtSlot()
    def sensorChanged(self, newID):        
        self.sensorID = newID
        if self.modeID == 1:
            self.minScaleY = DEF.MIN_T_SCALE_Y_DEG
            self.maxScaleY = DEF.MAX_T_SCALE_Y_DEG
        else:
            self.minScaleY = DEF.MIN_T_SCALE_Y_INT
            self.maxScaleY = self.getAlignedMaxValue()
            self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)
            
        
    @QtCore.pyqtSlot()
    def modeChanged(self, newMode):
        if self.modeID == 0:    # if previous mode is integer value
            for i in range(self.nChips):
                self.c[i].detach()
                self.v[i].attach(self)
            self.minScaleY = DEF.MIN_T_SCALE_Y_DEG
            self.maxScaleY = DEF.MAX_T_SCALE_Y_DEG
            self.setAxisTitle(Qwt.QwtPlot.yLeft, "Celcius")
        else:                   # if previous mode is degree value
            for i in range(self.nChips):
                self.v[i].detach()
                self.c[i].attach(self)
            self.minScaleY = DEF.MIN_T_SCALE_Y_INT
            self.maxScaleY = self.getAlignedMaxValue()
            self.setAxisTitle(Qwt.QwtPlot.yLeft, "Values")
        self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)
        self.modeID = newMode
                

    def wheelEvent(self, e):
        if self.modeID==1:      # only for degree values
            numDegrees = e.delta() / 8
            numSteps = numDegrees / 15  # will produce either +1 or -1
            maxY = self.maxScaleY - (numSteps*5)
            if maxY > DEF.MAX_T_SCALE_Y_DEG:
                maxY = DEF.MAX_T_SCALE_Y_DEG
            if maxY < DEF.MIN_T_SCALE_Y_DEG+10:
                maxY = DEF.MIN_T_SCALE_Y_DEG+10
            self.maxScaleY = maxY
            self.setAxisScale(Qwt.QwtPlot.yLeft, self.minScaleY, self.maxScaleY)

    def saveToFileTriggered(self, state, dirName):
        if state is True:
            for i in range(self.nChips):
                fName = dirName + '/temp.' + str(i)
                print "Creating file {}".format(fName)
                self.Tfiles[i] = open(fName, 'w')
            self.saveToFile = True

        else:
            self.saveToFile = False
            for i in range(self.nChips):
                if self.Tfiles[i] is not None:
                    self.Tfiles[i].close()
                    self.Tfiles[i] = None
                
    """
    ############################# Helper Functions ##################################
    """
    def getP2Vidx(self, cmap, x, y):
        for i in range(self.nChips):
            if cmap[i][0]==x and cmap[i][1]==y:
                return i
# class DataPlot
