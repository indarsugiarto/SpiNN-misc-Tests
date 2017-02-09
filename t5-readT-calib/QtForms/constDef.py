"""
SYNOPSYS
    Constant values for global definition.

Created on 2 Nov 2015
@author: indar.sugiarto@manchester.ac.uk
"""

#default port (usually)
HOST = '192.168.240.253'
SEND_PORT = 17893	# the default port for sending sdp to SpiNNaker
TUBO_PORT = 17892	# Tubotron normally uses this port

#port for streaming data from SpiNNaker
RECV_PORT = 20001	# previously: 17899
REPORT_IPTAG = 3 	# SpiNNaker profiler program uses this tag number, optional
SPINN_SDP_PORT = 7	# SpiNNaker profiler program uses this port number for comm with host

#port for streaming data from Raspberry pi
RASPI_ADDR = '192.168.123.1' # the Pi3 has been set as a adhoc network ("pi-adhoc")
RASPI_PORT = 30001

"""
TEMP1_OFFSET = 6250
TEMP1_GRAD = -4.9
TEMP2_OFFSET = 9102
TEMP2_GRAD = 21.92
"""
TEMP1_GRAD   = [-23.83, -21.18, -21.27, -22.52]  #Evangelos: -4.9, Patrick: -5.85
TEMP1_OFFSET = [7307.61, 7421.88, 7046.29, 7754.77]     #Evangelos: 6250
TEMP2_GRAD   = [73.25, 60.85, 64.64, 63.21]  #Evangelos: 21.92, Patrick: 22.45
TEMP2_OFFSET = [6861.84, 7408.09, 7049.93, 7681.19]     #Evangelos: 9102
MAX_T_SCALE_Y_DEG = 100
MIN_T_SCALE_Y_DEG = 20
MIN_T_SCALE_Y_INT = 5000
MAX_T_SCALE_Y_INT = 12000

MAX_U_SCALE_AT_250 = 36330  # if I change priority level, I can get also 38050
MAX_U_SCALE_AT_10 = 1480    # again, different priority level will produce different value, eg. 1550

MAX_F_SCALE = 260
MIN_F_SCALE = 0

"""
My own experiment-4 (old): 
Chip    f=10MHz                 f=250Mhz                Regression
<0,0>   30.8:6517,9290          44.4:6252,10139         m1=-19.265,b1=7107.4; m2=62.426,b2=7367.3
<0,1>   29.5:6729,9394          36.8:6449,10227         m1=-38.356,b1=7864.3; m2=96.575,b2=6545.2
<1,0>   32.0:6334,9210          45.3:6076,10021         m1=-19.398,b1=6954.7; m2=60.977,b2=7258.7
<1,1>   30.3:7007,9774          40.4:6721,10601         m1=-28.317,b1=7865,0; m2=81.881,b2=7292.8
My own experiment-4 (new): 
<0,0>   33.3:6514,9301          60.5:5804,11541         m1=-26.102941,b1=7383.227941; m2=82.352941,b2=6558.647059
<0,1>   32.9:6725,9410          51.3:6399,10298         m1=-17.717391,b1=7307.902174; m2=48.260870,b2=7822.217391
<1,0>   33.4:6334,6216          52.4:6003,10115         m1=-17.421053,b1=6915.863158; m2=205.210526,b2=-638.031579
<1,1>   33.3:7005,9786          58.2:6240,11858         m1=-30.722892,b1=8028.072289; m2=83.212851,b2=7015.012048
My own experiment-5 (newest): 
Chip    f=10MHz                 f=250Mhz                Regression
<0,0>   33.3:6514,9301          60.7:5861,11308         m1=-23.832117,b1=7307.609489; m2=73.248175,b2=6861.835766
<0,1>   32.9:6725,9410          65.9:6026,11418         m1=-21.181818,b1=7421.881818; m2=60.848485,b2=7408.084848
<1,0>   33.4:6334,6216          64.2:5681,11200         m1=-21.266234,b1=7046.292208; m2=64.642857,b2=7049.928571
<1,1>   33.3:7005,9786          65.1:6289,11796         m1=-22.515723,b1=7754.773585; m2=63.207547,b2=7681.188679

"""


######################### GUI parameters ########################
PEN_WIDTH = 2

