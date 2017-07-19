#include <math.h>
#include "profiler.h"

/*----------------------------------------------------------------------*/
/*---------------------- Event handler function ------------------------*/

void hMCPL(uint key, uint payload)
{
	uint subkey = key & 0xFFFFFF00;
	ushort senderID = key & 0x000000FF;
	if(subkey==MCPL_PROFILERS_REPORT_PART1) {
		sark_mem_cpy((void *)&proData[senderID], (void *)&payload, sizeof(uint));
	}
}

/* sendReport is triggered by Timer
 * Each profiler collect information and send to the root.
 * The root will combine information sent by other profilers.
*/
void sendReport(uint arg0, uint arg1)
{
#if(DEBUG_LEVEL > 0)
	cntr++;
	if(cntr >= 1000) return;
#endif
    // read temperature sensor, here we need only tempVal[2]
    readTemp(0,0); 

    pro_info_t myPro;
    myPro.freq = readSpinFreqVal();
    myPro.nActive = getNumActiveCores();
    myPro.temp = tempVal[2];

	if(sv->p2p_addr != 0) {    
	    uint key, payload;
		key = MCPL_PROFILERS_REPORT_PART1 | myID;
	    sark_mem_cpy((void *)&payload, (void *)&myPro, sizeof(uint));
		spin1_send_mc_packet(key, payload, WITH_PAYLOAD);
	} else {
		proData[0] = myPro;
	}
}

void reportToHost(uint arg0, uint arg1)
{
    // put in seg, arg1, arg2, and arg3
	basicReport.seq++;
	/* how to read board temperature? via BMP?
    report.arg1 = tempVal[0];
    report.arg2 = tempVal[1];
    report.arg3 = tempVal[2]; */
	sark_mem_cpy((void *)&basicReport.data, (void *)proData, sizeof(proData));
	spin1_send_sdp_msg (&basicReport, DEF_TIMEOUT);
	io_printf(IO_BUF, "Reporting-%d to host...\n", basicReport.seq);
}

void readSCP(uint mailbox, uint port)
{
     sdp_msg_t *msg = (sdp_msg_t *) mailbox;
    // use MY_SDP_PORT for communication with the host via eth
    if(port==DEF_SPINN_SDP_PORT) {
        switch(msg->cmd_rc) {
        case HOST_REQ_PROFILER_INFO:
            spin1_schedule_callback(reportToHost, 0, 0, APP_PRIORITY_VAL);
            break;
        }
    }
    spin1_msg_free (msg);
}

void initRouter()
{
    uint d, e, x, y, pc, dest;

    pc = 1 << (DEF_PROFILER_CORE+6); // profiler core

	x = CHIP_X(sv->p2p_addr);
	y = CHIP_Y(sv->p2p_addr);

	e = rtr_alloc(3);

	// broadcasting from root profiler:
	dest = pc;	
	if(x==y) {
		if(x==0)
			dest = 1 + (1 << 1) + (1 << 2);
		else if(x<7)
			dest += 1 + (1 << 1) + (1 << 2);
	}
	else if(x>y) {
		d = x - y;
		if(x<7 && d<4)
			dest += 1;
	}
	else if(x<y) {
		d = y - x;
		if(d<3 && y<7)
			dest += (1 << 2);
	}

	rtr_mc_set(e, MCPL_BCAST_REQ_UPDATE, 0xFFFFFFFF, dest); e++;

	// report towards root profiler in core <0,0>
	if (x>0 && y>0)			dest = (1 << 4);	// south-west
	else if(x>0 && y==0)	dest = (1 << 3);	// west
	else if(x==0 && y>0)	dest = (1 << 5);	// south
	else					dest = pc;
    // use lower 2-bytes for profiler ID
	rtr_mc_set(e, MCPL_PROFILERS_REPORT_PART1, 0xFFFFFF00, dest); e++;
	rtr_mc_set(e, MCPL_PROFILERS_REPORT_PART2, 0xFFFFFF00, dest); e++;
}



void c_main(void)
{
    // test memTable
    /*
    uint f, ns, ms;
    if(sark_chip_id()==0) {
        for(uint i=0; i<lnMemTable; i++) {
            f = (uint)memTable[i][0];
            getFreqParams(f, &ms, &ns);
            io_printf(IO_STD, "i==%u, f==%u, ms==%u, ns==%u\n", i, f, ms, ns);

        }
        return;
    } else return;
    */

    // First: sanity check
    // Since some features require strict conditions
    /*
    if(sark_app_id() != DEF_MY_APP_ID) {
        io_printf(IO_STD, "Please assign me app-ID %u\n", DEF_MY_APP_ID);
        return;
    }
    */

    if(sark_core_id() != DEF_PROFILER_CORE) {
        io_printf(IO_STD, "Please put me in core-%u\n", DEF_PROFILER_CORE);
        return;
    }

    myID = generateProfilerID();
    if(myID==-1) {
        io_printf(IO_STD, "Invalid profiler ID for chip<%d,%d>!\n", 
                  CHIP_X(sv->p2p_addr), CHIP_Y(sv->p2p_addr));
        return;
    }

	// Test if ID generation is correct: YES!
	/*
	sark_delay_us(myID*1000);
    io_printf(IO_STD, "Profiler ID-%d is started at <%u,%u:%u>!\n", 
              myID, CHIP_X(sv->p2p_addr), CHIP_Y(sv->p2p_addr), sark_core_id());
    io_printf(IO_BUF, "Profiler ID-%d is started at <%u,%u:%u>!\n", 
              myID, CHIP_X(sv->p2p_addr), CHIP_Y(sv->p2p_addr), sark_core_id());

	return;
	*/

    // get the original PLL configuration and current frequency
    // Note: r20-->PLL1_controller, r21-->PLL2_controller, r24-->clock_mux
    //       In normal operation, CPU-A and CPU-B use PLL1
    _r20 = 0x70128;                                        // _r20 = sc[SC_PLL1];
    _r21 = 0x7011a;                                        // _r21 = sc[SC_PLL2];
    _r24 = 0x809488a5;                                    // _r24 = sc[SC_CLKMUX];
    _freq = readSpinFreqVal();

	// initialize router
	initRouter();

	// prepare SDP mechanism, only if I'm root
	if(sv->p2p_addr==0) {
		io_printf(IO_BUF, "Preparing SDP...\n");
		// for basic report: temperature, freq, numActiveCore
		szBasicReport = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t) + (48*sizeof(pro_info_t));
		basicReport.tag = DEF_REPORT_IPTAG;                      // Don't forget to specify the proper iptag in ybug
		basicReport.dest_port = PORT_ETH;                        // Ethernet
		basicReport.dest_addr = sv->eth_addr;                    // Nearest Ethernet chip
		basicReport.flags = 0x07;                                // no need for reply
		basicReport.srce_port = DEF_SPINN_SDP_PORT;
		basicReport.srce_addr = sv->p2p_addr;
		basicReport.cmd_rc = PROFILER_VERSION;
		basicReport.seq = 0;
		basicReport.arg1 = SDP_BASICREPORT_TAG;
		basicReport.length = szBasicReport;
		// for cpu idle report
		szCpuIdleReport = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t) + (48*sizeof(cpu_idle_info_t));
		cpuIdleReport.tag = DEF_REPORT_IPTAG;
		cpuIdleReport.dest_port = PORT_ETH;
		cpuIdleReport.dest_addr = sv->eth_addr;
		cpuIdleReport.flags = 7;
		cpuIdleReport.srce_port = DEF_SPINN_SDP_PORT;
		cpuIdleReport.srce_addr = sv->p2p_addr;
		cpuIdleReport.cmd_rc = PROFILER_VERSION;
		cpuIdleReport.seq = 0;
		cpuIdleReport.arg1 = SDP_CPUIDLEREPORT_TAG;
		cpuIdleReport.length = szCpuIdleReport;

		// for synchronization with arduino, it needs triggering
		spin1_callback_on(SDP_PACKET_RX, readSCP, SCP_PRIORITY_VAL);
	}

    // setup Timer for reporting:
    // it periodically collects information and sends it to the root 
    spin1_set_timer_tick(REPORT_TIMER_TICK_PERIOD_US);
    spin1_callback_on(TIMER_TICK, sendReport, APP_PRIORITY_VAL);

    // setup MCPL callback
    spin1_callback_on(MCPL_PACKET_RECEIVED, hMCPL, MCPL_PRIORITY_VAL);

    // Anything else

	sark.vcpu->user0 = PROFILER_VERSION;				// put version to vcpu->user0 to be detected by readSpin4Pwr.py
	sark_delay_us(myID*1000);
    io_printf(IO_STD, "Profiler ID-%d is started at <%u,%u:%u>!\n", 
              myID, CHIP_X(sv->p2p_addr), CHIP_Y(sv->p2p_addr), sark_core_id());
    spin1_start(SYNC_NOWAIT);
}

