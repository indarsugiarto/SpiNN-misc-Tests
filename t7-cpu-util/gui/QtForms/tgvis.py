"""
This module displays SpiNNaker chips layout.
"""

from PyQt4 import Qt, QtGui, QtCore
from PyQt4 import QtSvg

SVG_IMG_WIDTH = 1000
SVG_IMG_HEIGHT = 1200
SVG_IMG_POS_X = 100
SVG_IMG_POS_Y = 0
#SVG_IMG_WIDTH = 900
#SVG_IMG_HEIGHT = 1000
#SVG_IMG_POS_X = 400
#SVG_IMG_POS_Y = -1200

# Drawing parameters
ChipWidth = 85
ChipHeight = 85
xOffset = 125
yOffset = -125
hSpace = xOffset-ChipWidth
DrawingCanvasBackColor = Qt.Qt.white
ChipPenWidth = 2
ChipBoxColorOn = Qt.Qt.black
ChipBoxColorOff = DrawingCanvasBackColor
ChipBrushColor = DrawingCanvasBackColor
#Node text
NodeTxtXOffset = 40
NodeTxtYOffset = -20
NodeTxtColorOn = ChipBoxColorOn
NodeTxtColorOff = ChipBoxColorOff
#Temperature text
TempTxtColorOn = Qt.Qt.red
TempTxtColorOff = DrawingCanvasBackColor
TempTxtXOffset = 25
TempTxtYOffset = 5

# copy from constDef:
DEF_DEAD_CHIP_COLOR = Qt.Qt.red
DEF_ACTIVE_CHIP_COLOR = Qt.Qt.blue
DEF_INACTIVE_CHIP_COLOR = Qt.Qt.white
DEF_OK_CHIP_TEXT = Qt.Qt.black
DEF_DEAD_CHIP_TEXT = Qt.Qt.red

"""--------------------------------------------------------
                Items for drawing "chip"
--------------------------------------------------------"""
class chipBox(QtGui.QGraphicsRectItem):
    def __init__(self, id, xy, parent=None):
        """
        
        :param id: general purpose id, usually just an automatically generated sequential number 
        :param xy: chip xy coordinate from spinnaker (from <X,Y>)
        :param parent: 
        """
        QtGui.QGraphicsRectItem.__init__(self, parent)
        self.id = id
        self.xy = xy
        x = xy[0]
        y = xy[1]
        w = ChipWidth
        h = ChipHeight
        self.setRect(x*xOffset,y*yOffset,w,h)
        self.setBrush(ChipBrushColor)
        self.turnOn()

    def turnOn(self):
        self.on = True
        pen = QtGui.QPen()
        pen.setWidth(ChipPenWidth)
        pen.setColor(ChipBoxColorOn)
        self.setPen(pen)

    def turnOff(self):
        self.on = False
        pen = QtGui.QPen()
        pen.setWidth(ChipPenWidth)
        pen.setColor(ChipBoxColorOff)
        self.setPen(pen)

    def mousePressEvent(self, event):
        print "chip ID-", self.id, "at",self.xy,"is clicked!"


class nodeTxt(QtGui.QGraphicsTextItem):
    """
    Displaying chip coordinate <x,y>
    """
    def __init__(self, id, xy, parent=None):
        QtGui.QGraphicsTextItem.__init__(self, parent)
        self.id = id
        self.xy = xy
        x = xy[0]
        y = xy[1]
        self.on = True
        nodeStr = "<%d,%d>"%(x,y)
        self.setPlainText(nodeStr)
        self.setDefaultTextColor(ChipBoxColorOn)
        self.setPos(x*xOffset+NodeTxtXOffset,y*yOffset+NodeTxtYOffset)
        fnt = self.font()
        fnt.setBold(True)
        self.setFont(fnt)

    def turnOn(self):
        self.on = True
        self.setDefaultTextColor(NodeTxtColorOn)

    def turnOff(self):
        self.on = False
        self.setDefaultTextColor(NodeTxtColorOff)


class tempTxt(QtGui.QGraphicsTextItem):
    """
    Temperature text will ALWAYS displayed as 3 digit (2 integer, 1 decimal)
    """
    def __init__(self, id, xy, parent=None):
        QtGui.QGraphicsTextItem.__init__(self, parent)
        self.id = id
        self.xy = xy
        x = xy[0]
        y = xy[1]
        self.setPos(x*xOffset+TempTxtXOffset,y*yOffset+TempTxtYOffset)
        self.setVal(0)
        self.turnOn()


    def getStrVal(self, numVal):
        return "%2.1f" % numVal

    def setVal(self, newVal):
        self.numVal = newVal
        self.strVal = self.getStrVal(newVal) + u"\u00b0" + "C"
        self.setPlainText(self.strVal)

    def turnOn(self):
        self.on = True
        self.setDefaultTextColor(TempTxtColorOn)

    def turnOff(self):
        self.on = False
        self.setDefaultTextColor(TempTxtColorOff)


class visWidget(QtGui.QGraphicsView):
    visWidgetTerminated = QtCore.pyqtSignal() # data from SpiNNaker profiler program

    def __init__(self, mw, mh, okChipList, okToClose=True, parent=None):
        """
        :param mw: machine width
        :param mh: machine height
        :param chipList: list of tuples of chip X,Y coordinates
        :param parent: QWidget
        """
        self.okChipList = okChipList
        self.nOKChips = len(okChipList)

        #print self.chipList

        QtGui.QGraphicsView.__init__(self, parent)
        self.setBackgroundBrush(Qt.QBrush(DrawingCanvasBackColor))
        self.scene = QtGui.QGraphicsScene()
        #self.scene.addText("Test")

        self.drawLayout()
        #self.drawLayout(mode=1)    # it will produce messy connection!!!
        #self.scene.addRect(0,0,100,100)

        self.setScene(self.scene)
        #self.view = QtGui.QGraphicsView(self.scene, self)
        #self.view.show()

        self.sceneTimer = QtCore.QTimer(self)
        # self.cbChips.currentIndexChanged.connect(self.chipChanged)
        self.sceneTimer.timeout.connect(self.sceneTimerTimeout)
        #self.sceneTimer.start(1000) # will fire every 1000ms (every 1 sec)
        self.on = False
        self.okToClose = okToClose
        #self.saveToSVG()

    def closeEvent(self, e):
        if self.okToClose:
            self.visWidgetTerminated.emit()
            #print "closing..."
            e.accept()
            super(visWidget, self).closeEvent(e)
            # info: https://stackoverflow.com/questions/12365202/how-do-i-catch-a-pyqt-closeevent-and-minimize-the-dialog-instead-of-exiting


    @QtCore.pyqtSlot(list)
    def spinUpdate(self, prfData):
        """
        Receives profiler data from SpiNNaker sent by ifaceArduSpiNN

        :return:
        """


    @QtCore.pyqtSlot()
    def sceneTimerTimeout(self):
        """
        Update the content of the scene
        """
        for i in range(self.nOKChips):
            self.okChips[i].setBrush(DEF_INACTIVE_CHIP_COLOR)
        """
        self.on = not self.on
        if self.on is True:
            for i in range(48):
                self.chips[i].setBrush(Qt.Qt.red)
        else:
            for i in range(48):
                self.chips[i].setBrush(Qt.Qt.white)
        """

    def showOnly(self, chipList):
        """
        Only shows chips in the chipList
        """
        # first, disable all chips
        for c in self.okChips:
            c.hide()
        # also disable the links
        for i in range(self.nOKChips):
            for j in range(6):
                self.edges[i][j].hide()
            self.okChipIDTxt[i].setPlainText("")
        # then enable only the list
        for c in self.okChips:
            for xy in chipList:
                if c.xy==xy:
                    c.show()
                    self.okChipIDTxt[c.id].setPlainText(" i")
                    for i in range(6):
                        if (i != 1) or (i != 4):
                            self.edges[c.id][i].show()
        self.update()

    def ornament(self, nodeList, clrList):
        """
        Put the active chip in color based on its throughput.
        """
        n = len(nodeList)
        pen = QtGui.QPen()
        pen.setWidth(2)
        fnt14 = Qt.QFont()
        fnt14.setPointSize(36)
        fnt14.setWeight(Qt.QFont.Bold)

        for c in self.okChips:
            for i in range(n):
                if nodeList[i] == c.xy:
                    r = clrList[i][0]
                    g = clrList[i][1]
                    b = clrList[i][2]
                    clr = Qt.QColor(r,g,b)
                    pen.setColor(clr)
                    c.setPen(pen)
                    c.setBrush(clr)
                    # rename corresponding chip
                    txt = str(i)
                    self.okChipIDTxt[c.id].setPlainText(txt)
                    self.okChipIDTxt[c.id].setDefaultTextColor(Qt.Qt.white)
                    self.okChipIDTxt[c.id].setFont(fnt14)

        # draw colorbar
        self.clrBar = QtGui.QGraphicsRectItem()
        #self.clrBar.setRect(1100,-300,500,30)
        self.clrBar.setRect(600,-300,500,30)
        self.clrBar.setPen(Qt.QPen(Qt.Qt.white))
        gradient = Qt.QLinearGradient(self.clrBar.rect().topLeft(), self.clrBar.rect().topRight())
        gradient.setColorAt(0, Qt.Qt.blue)
        gradient.setColorAt(1, Qt.Qt.red)
        self.clrBar.setBrush(Qt.QBrush(gradient))
        self.scene.addItem(self.clrBar)

        self.clrInfo = QtGui.QGraphicsTextItem("Chip Activity")
        self.clrInfo.setPos(680,-360);
        fnt12 = Qt.QFont()
        fnt12.setPointSize(24)
        fnt12.setWeight(Qt.QFont.Bold)
        fnt16 = Qt.QFont()
        fnt16.setPointSize(36)
        fnt16.setWeight(Qt.QFont.Bold)
        self.clrInfo.setDefaultTextColor(Qt.Qt.black)
        self.clrInfo.setFont(fnt16)

        self.minInfo = QtGui.QGraphicsTextItem("Min")
        #self.minInfo.setPos(1100,-270)
        self.minInfo.setPos(600,-270)
        self.minInfo.setDefaultTextColor(Qt.Qt.black)
        self.minInfo.setFont(fnt12)

        self.maxInfo = QtGui.QGraphicsTextItem("Max")
        #self.maxInfo.setPos(1535,-270)
        self.maxInfo.setPos(1020,-270)
        self.maxInfo.setDefaultTextColor(Qt.Qt.black)
        self.maxInfo.setFont(fnt12)

        #border
        self.figBorder = QtGui.QGraphicsRectItem()
        self.figBorder.setRect(400, -1200, 900, 1000)
        self.figBorder.setPen(Qt.QPen(Qt.Qt.white))
        #self.figBorder.setPen(Qt.QPen(Qt.Qt.black))

        self.scene.addItem(self.figBorder)
        self.scene.addItem(self.clrInfo)
        self.scene.addItem(self.minInfo)
        self.scene.addItem(self.maxInfo)

    def saveToSVG(self, fname):
        """
        save into a file in svg format
        """
        self.svgGen = QtSvg.QSvgGenerator()
        self.svgGen.setFileName(fname)
        szF = self.scene.itemsBoundingRect()
        sz = Qt.QSize(int(szF.width()), int(szF.height()))
        #szF = self.figBorder.boundingRect()
        #sz = Qt.QSize(int(szF.width()), int(szF.height()))
        self.svgGen.setSize(sz)
        bx = Qt.QRect(0,0,sz.width(),sz.height())
        #bx = Qt.QRect(SVG_IMG_POS_X, SVG_IMG_POS_X, SVG_IMG_WIDTH, SVG_IMG_HEIGHT)
        self.svgGen.setViewBox(bx)
        self.svgGen.setTitle("TGSDP")
        self.svgGen.setDescription("By Indar Sugiarto")
        painter = Qt.QPainter(self.svgGen)
        self.scene.render(painter)

    def drawLayout(self):
        # Draw Spin5 chip layout
        w = 85  # width of the box displaying a chip
        h = 85  # height of the box displaying a chip
        xoffset = 125
        yoffset = -125
        hspace = xoffset-85

        # draw as logical layout

        self.edges = [[QtGui.QGraphicsLineItem() for _ in range(6)] for _ in range(self.nOKChips)]
        #self.chips = [QtGui.QGraphicsRectItem() for _ in range(self.nChips)]

        # instantiate box for chip visualization
        allOKIdx = list()
        self.okChips = list()
        self.okTempTexts = list()
        self.okChipIDTxt = list()
        for i in range(self.nOKChips):
            xy = self.okChipList[i]
            id = self.getIdxFromXY(xy)
            #allOKIdx.append(self.getIdxFromXY(self.okChipList[i]))
            allOKIdx.append(id)
            self.okChips.append(chipBox(id, xy))
            self.okTempTexts.append(tempTxt(id, xy))
            self.okChipIDTxt.append(nodeTxt(id, xy))
        #self.okChips = [chipBox(id) for id in allOKIdx]


        pen = QtGui.QPen()
        pen.setWidth(2)
        # for available chips
        for i in range(self.nOKChips):
            edgeIdx = i
            x = self.okChipList[i][0]
            y = self.okChipList[i][1]
            #self.okChips[i].xy = (x,y)
            #self.okChips[i].setRect(x*xoffset,y*yoffset,w,h)
            #self.okChips[i].setPen(pen)
            #self.okChips[i].setBrush(DEF_INACTIVE_CHIP_COLOR)
            self.scene.addItem(self.okChips[i])
            self.scene.addItem(self.okChipIDTxt[i])
            self.scene.addItem(self.okTempTexts[i])

            # put edge-0
            x1 = x*xoffset+w
            y1 = y*yoffset+h/2
            x2 = x1+hspace/2
            y2 = y1
            self.edges[edgeIdx][0].setLine(x1,y1,x2,y2)
            self.scene.addItem(self.edges[edgeIdx][0])
            # put edge-1
            x1 = x*xoffset+w
            y1 = y*yoffset
            x2 = x1+hspace/2
            y2 = y1-hspace/2
            self.edges[edgeIdx][1].setLine(x1,y1,x2,y2)
            #self.scene.addItem(self.edges[edgeIdx][1])
            # put edge-2
            x1 = x*xoffset+w/2
            y1 = y*yoffset
            x2 = x1
            y2 = y1-hspace/2
            self.edges[edgeIdx][2].setLine(x1,y1,x2,y2)
            self.scene.addItem(self.edges[edgeIdx][2])
            # put edge-3
            x1 = x*xoffset
            y1 = y*yoffset+h/2
            x2 = x1-hspace/2
            y2 = y1
            self.edges[edgeIdx][3].setLine(x1,y1,x2,y2)
            self.scene.addItem(self.edges[edgeIdx][3])
            # put edge-4
            x1 = x*xoffset
            y1 = y*yoffset+h
            x2 = x1-hspace/2
            y2 = y1+hspace/2
            self.edges[edgeIdx][4].setLine(x1,y1,x2,y2)
            #self.scene.addItem(self.edges[edgeIdx][4])
            # put edge-5
            x1 = x*xoffset+w/2
            y1 = y*yoffset+h
            x2 = x1
            y2 = y1+hspace/2
            self.edges[edgeIdx][5].setLine(x1,y1,x2,y2)
            self.scene.addItem(self.edges[edgeIdx][5])
            for j in range(6):
                self.edges[edgeIdx][j].setPen(pen)

    #self.scene.addLine(0,0,70,70)

    @QtCore.pyqtSlot(list)
    def updateHist(self, xy, clr=DEF_ACTIVE_CHIP_COLOR):
        """
        This Slot should be connected to sdp.histUpdate
        """
        cId = self.getIdxFromXY(xy)
        #print "xy={}, cId={}".format(xy, cId)
        #cId might be None if the chip xy is dead
        if cId is not None:
            self.chips[cId].setBrush(clr)

    def updateLink(self):
        """
        Got new information regarding paths taken for sending sdp within spinnaker machine?
        :return:
        """

    def getIdxFromXY(self, xy):   # xy is a chip coordinate as a tuple
        for i in range(self.nOKChips):
            if self.okChipList[i]==xy:
                return i

