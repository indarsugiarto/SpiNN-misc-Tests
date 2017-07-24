#include <math.h>
#include "profiler.h"

/*--------------------- routing mechanism ------------------------*/
#define MCPL_BCAST_REQ_UPDATE 				0xbca50001	// broadcast from root to all other profilers
#define MCPL_PROFILERS_REPORT_PART1			0x21ead100	// go to leader in chip-0
#define MCPL_PROFILERS_REPORT_PART2			0x21ead200	// go to leader in chip-0
#define MCPL_GLOBAL_TICK					MCPL_BCAST_REQ_UPDATE	// just an alias


/*------------------- SDP-related parameters ---------------------*/
#define DEF_GENERIC_IPTAG			2		// remember to put this value in ybug
#define DEF_GENERIC_UDP_PORT		20000
#define DEF_REPORT_IPTAG			3
#define DEF_REPORT_PORT				20001
#define DEF_ERR_INFO_TAG			4
#define DEF_ERR_INFO_PORT			20002
#define DEF_SPINN_SDP_PORT			7		// port-7 has a special purpose, usually related with ETH
#define DEF_TIMEOUT					10		// as recommended

// scp sent by host, related to frequecy/PLL
#define HOST_REQ_PLL_INFO			1
#define HOST_SET_CHANGE_PLL			2		// host request special PLL configuration
#define HOST_REQ_REVERT_PLL			3
#define HOST_SET_FREQ_VALUE			4		// Note: HOST_SET_FREQ_VALUE assumes that CPUs use PLL1,
                                            // if this is not the case, then use HOST_SET_CHANGE_PLL
#define HOST_REQ_PROFILER_INFO		5		// specific to profiler 333, for synchronization with Arduino

// iptag
// IPTAG definitions from scamp:
#define IPTAG_NEW		0
#define IPTAG_SET		1
#define IPTAG_GET		2
#define IPTAG_CLR		3
#define IPTAG_TTO		4

#define SDP_HOST_IP				    0x02F0A8C0	// 192.168.240.2, dibalik!
#define SDP_IP_TAG_REPORTING        1

// sdp container
sdp_msg_t iptag;

/*--------------------- Reporting Data Structure -------------------------*/
/*------------------------------------------------------------------------*/
#define SDP_BASICREPORT_TAG			1
typedef struct pro_info {
    uchar freq;
    uchar nActive;
    ushort temp;
    uchar load[18];
} pro_info_t;
pro_info_t proData[48];							// for SpiNN-5 board
sdp_msg_t basicReport;							// for other parameters report
uint szBasicReport;

// since proData will occupy 192 bytes, no more room for cpu idle data
// hence, we create a new structure only for cpu idle data
// thus, we also need a new sdp container for this
#define SDP_CPUIDLEREPORT_TAG		2
typedef struct cpu_idle_info {
    ushort maxVal;
    ushort sumVal;
} cpu_idle_info_t;
cpu_idle_info_t cpuIdleData[48];
sdp_msg_t cpuIdleReport;
uint szCpuIdleReport;

/*------------------------- Other stuffs -------------------------------*/

#define TIMER_TICK_PERIOD_US        1000000



/*----------------------------------------------------------------------*/
/*----------------------- Function Prototypes --------------------------*/
void hSDP(uint mailbox, uint port);
void hMCPL(uint key, uint payload);
void hTimer(uint tick, uint Unused);
void initIPTag();
void init_sdp_container();
void sendReport(uint arg0, uint arg1);
void reportToHost(uint arg0, uint arg1);
/*
 * In order to set iptag properly, we need to collect information
 * from SCAMP about the ip address of host-PC. Usually tag-0 contains
 * valid IP address. Hence, before we initIPTag, we need to collect
 * host-PC IP address. The initiateIPTagReading should be scheduled.
 * */
void initiateIPTagReading(uint arg1, uint arg2);


/*----------------------------------------------------------------------*/
/*--------------------- Initialization function ------------------------*/

void init_Handlers()
{
    spin1_callback_on(SDP_PACKET_RX, hSDP, SCP_PRIORITY_VAL);
    spin1_callback_on(MCPL_PACKET_RECEIVED, hMCPL, MCPL_PRIORITY_VAL);
    spin1_set_timer_tick(TIMER_TICK_PERIOD_US);
    spin1_callback_on(TIMER_TICK, hTimer, TIMER_PRIORITY_VAL);
    init_sdp_container();
    //initIPTag(); --> replaced with initiateIPTagReading()
    spin1_schedule_callback(initiateIPTagReading, 0, 0, 1);
}


/*----------------------------------------------------------------------*/
/*---------------------- Event handler function ------------------------*/

void hTimer(uint tick, uint Unused)
{

}

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

void hSDP(uint mailbox, uint port)
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

void init_Router()
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



void init_sdp_container()
{

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

        // iptagging
        iptag.flags = 0x07;	// no replay
        iptag.tag = 0;		// internal
        iptag.srce_addr = sv->p2p_addr;
        iptag.srce_port = 0xE0 + (uchar)sark_core_id();	// use port-7
        iptag.dest_addr = sv->p2p_addr;
        iptag.dest_port = 0;				// send to "root"
        iptag.cmd_rc = CMD_IPTAG;
        iptag.arg1 = (IPTAG_GET << 16) + 0; // target iptag-0
        iptag.arg2 = 1; //remember, scamp use this for some reason
        iptag.length = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t);
    }


}

void initiateIPTagReading(uint arg1, uint arg2)
{
    // only root profiler needs this
    // the result should be read through hSDP
    if(sv->p2p_addr==0) {
        spin1_send_sdp_msg(&iptag, DEF_TIMEOUT);
    }
}


void initIPTag()
{
    // only chip <0,0>
    if(sv->p2p_addr==0) {
        iptag.flags = 0x07;	// no replay
        iptag.tag = 0;		// internal
        iptag.srce_addr = sv->p2p_addr;
        iptag.srce_port = 0xE0 + (uchar)sark_core_id();	// use port-7
        iptag.dest_addr = sv->p2p_addr;
        iptag.dest_port = 0;				// send to "root"
        iptag.cmd_rc = CMD_IPTAG;
        // set the reply tag
        iptag.arg1 = (IPTAG_SET << 16) + SDP_TAG_REPLY;
        iptag.arg2 = SDP_UDP_REPLY_PORT;
        iptag.arg3 = SDP_HOST_IP;
        iptag.length = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t);
        spin1_send_sdp_msg(&iptag, 10);
        // set the result tag
        iptag.arg1 = (IPTAG_SET << 16) + SDP_TAG_RESULT;
        iptag.arg2 = SDP_UDP_RESULT_PORT;
        spin1_send_sdp_msg(&iptag, 10);
        // set the debug tag
        iptag.arg1 = (IPTAG_SET << 16) + SDP_TAG_DEBUG;
        iptag.arg2 = SDP_UDP_DEBUG_PORT;
        spin1_send_sdp_msg(&iptag, 10);
    }
}
