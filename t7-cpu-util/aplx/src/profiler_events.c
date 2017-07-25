#include <math.h>
#include "profiler.h"

/*--------------------- routing mechanism ------------------------*/
/*
#define MCPL_BCAST_REQ_UPDATE 				0xbca50001	// broadcast from root to all other profilers
#define MCPL_PROFILERS_REPORT_PART1			0x21ead100	// go to leader in chip-0
#define MCPL_PROFILERS_REPORT_PART2			0x21ead200	// go to leader in chip-0
#define MCPL_GLOBAL_TICK					MCPL_BCAST_REQ_UPDATE	// just an alias
*/

/*------------------- SDP-related parameters ---------------------*/
#define DEF_REPORT_TAG              3
#define DEF_REPORT_PORT				40003
#define DEF_ERR_INFO_TAG			4
#define DEF_ERR_INFO_PORT			40004
#define DEF_INTERNAL_SDP_PORT		1
#define DEF_HOST_SDP_PORT           7		// port-7 has a special purpose, usually related with ETH
#define DEF_TIMEOUT					10		// as recommended

// scp sent by host, related to frequecy/PLL
#define HOST_REQ_PLL_INFO			1
#define HOST_REQ_INIT_PLL			2		// host request special PLL configuration
#define HOST_REQ_REVERT_PLL			3
#define HOST_SET_FREQ_VALUE			4		// Note: HOST_SET_FREQ_VALUE assumes that CPUs use PLL1,
                                            // if this is not the case, then use HOST_SET_CHANGE_PLL
#define HOST_REQ_PROFILER_STREAM		5		// host send this to a particular profiler to start streaming

// iptag
// IPTAG definitions from scamp:
#define IPTAG_NEW		0
#define IPTAG_SET		1
#define IPTAG_GET		2
#define IPTAG_CLR		3
#define IPTAG_TTO		4

#define SDP_DEF_HOST_IP				    0x02F0A8C0	// 192.168.240.2, dibalik!
#define SDP_IP_TAG_REPORTING        1


// sdp container
sdp_msg_t iptag;        // only for root profiler
sdp_msg_t reportMsg;	// for all profilers


/*------------------------- Other stuffs -------------------------------*/

#define TIMER_TICK_PERIOD_US        1000000



/*----------------------------------------------------------------------*/
/*----------------------- Function Prototypes --------------------------*/
void hSDP(uint mailbox, uint port);
void hMCPL(uint key, uint payload);
void hTimer(uint tick, uint Unused);
void initIPTag(uint hostIP, uint Unused);
void init_sdp_container();
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
    spin1_callback_on(USER_EVENT, collectData, SCHEDULED_PRIORITY_VAL);
    init_sdp_container();
    //initIPTag(); --> replaced with initiateIPTagReading()
    spin1_schedule_callback(initiateIPTagReading, 0, 0, 1);
}


/*----------------------------------------------------------------------*/
/*---------------------- Event handler function ------------------------*/

/* collectData is a USER_EVENT triggerd by hSlowTimer in profiler_cpuload.c
 * */
void collectData(uint None, uint Unused)
{
	myProfile.cpu_freq = readFreq(&myProfile.ahb_freq, &myProfile.rtr_freq);
	myProfile.nActive = getNumActiveCores();
	myProfile.temp3 = readTemp();
	myProfile.temp1 = tempVal[0];
	sark_mem_cpy((void *)&reportMsg.cmd_rc, (void *)&myProfile, sizeof(pro_info_t));
	sark_mem_cpy((void *)reportMsg.data, (void *)stored_cpu_idle_cntr, 18);
	if(streaming==FALSE) return;
	spin1_send_sdp_msg (&reportMsg, DEF_TIMEOUT);
}

void hTimer(uint tick, uint Unused)
{

}

void hMCPL(uint key, uint payload)
{
    /*
	uint subkey = key & 0xFFFFFF00;
	ushort senderID = key & 0x000000FF;
	if(subkey==MCPL_PROFILERS_REPORT_PART1) {
		sark_mem_cpy((void *)&proData[senderID], (void *)&payload, sizeof(uint));
	}
    */
}

void hSDP(uint mailbox, uint port)
{
     sdp_msg_t *msg = (sdp_msg_t *) mailbox;
    // use DEF_HOST_SDP_PORT for communication with the host via eth
    if(port==DEF_HOST_SDP_PORT) {
        switch(msg->cmd_rc) {
        case HOST_REQ_PLL_INFO:
            spin1_schedule_callback(showPLLinfo, 0, 0, SCHEDULED_PRIORITY_VAL);
            break;
        case HOST_REQ_PROFILER_STREAM:
            streaming = TRUE;
            break;
        case HOST_SET_FREQ_VALUE:
			{
            // seq structure: byte-1 component, byte-0 frequency
			PLL_PART comp = (PLL_PART)(msg->seq >> 8);
            uint f = msg->seq & 0xFF;
            io_printf(IO_STD, "Request for comp-%d with f=%d\n", comp, f);
            changeFreq(comp, f);
            break;
			}
		}
	}
    // use DEF_INTERNAL_SDP_PORT for internal (inter-chip) communication
    // such as with monitor core for SCP
    else if(port==DEF_INTERNAL_SDP_PORT){
        if(msg->cmd_rc==0x80){ // CMD_OK from previous request
            //hostIP = msg->arg1;
            spin1_schedule_callback(initIPTag, msg->arg1, 0, SCHEDULED_PRIORITY_VAL);
        }
    }
    spin1_msg_free (msg);
}

void init_Router()
{
    /*
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
    */
}



void init_sdp_container()
{
    // for basic report: temperature, freq, numActiveCore, etc
    reportMsg.tag = DEF_REPORT_TAG;                      // Don't forget to specify the proper iptag in ybug
    reportMsg.dest_port = PORT_ETH;                        // Ethernet
    reportMsg.dest_addr = sv->eth_addr;                    // Nearest Ethernet chip
    reportMsg.flags = 0x07;                                // no need for reply
    reportMsg.srce_port = (DEF_HOST_SDP_PORT << 5) + (uchar)sark_core_id();
    reportMsg.srce_addr = sv->p2p_addr;
    reportMsg.length = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t) + (18*sizeof(uchar));

    // prepare IPtagging mechanism, only if I'm root
	if(sv->p2p_addr==0) {
        iptag.flags = 0x07;	// no replay
        iptag.tag = 0;		// internal
        iptag.srce_addr = sv->p2p_addr;
        iptag.srce_port = (DEF_INTERNAL_SDP_PORT << 5) + (uchar)sark_core_id();	// use port-7
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


void initIPTag(uint hostIP, uint Unused)
{
    // only chip <0,0>
    if(sv->p2p_addr==0) {
        // set the report tag
        iptag.arg1 = (IPTAG_SET << 16) + DEF_REPORT_TAG;
        iptag.arg2 = DEF_REPORT_PORT;
        //iptag.arg3 = SDP_DEF_HOST_IP;
        iptag.arg3 = hostIP;
        spin1_send_sdp_msg(&iptag, DEF_TIMEOUT);
        // set the generic error info tag (for debugging)
        iptag.arg1 = (IPTAG_SET << 16) + DEF_ERR_INFO_TAG;
        iptag.arg2 = DEF_ERR_INFO_PORT;
        spin1_send_sdp_msg(&iptag, DEF_TIMEOUT);
    }
}
