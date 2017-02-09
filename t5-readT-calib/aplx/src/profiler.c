/**** termometer.c
*
* SUMMARY
*  1. read the temperature sensor of the chip every (ca.) 0.1s and send to the host as SDP.
*     This timing precission will be affected by the clock frequency!
*
* AUTHOR
*  Indar Sugiarto - indar.sugiarto@manchester.ac.uk
*
* DETAILS
*  Created on       : 3 Sept 2015
*  Version          : $Revision: 1 $
*  Last modified by : $Author: indi $
*
* COPYRIGHT
*  Copyright (c) The University of Manchester, 2011. All rights reserved.
*  SpiNNaker Project
*  Advanced Processor Technologies Group
*  School of Computer Science
*
*******/

#include <math.h>
#include "profiler.h"

/* in readTemp(), read the sensors and put the result in val[] */
#define MY_CODE			1
#define PATRICK_CODE	2
#define READING_VERSION	MY_CODE	// 1 = mine, 2 = patrick's
void readTemp(uint arg1, uint arg2)
{
#if (READING_VERSION==1)
	uint i, done, S[] = {SC_TS0, SC_TS1, SC_TS2};

	for(i=0; i<3; i++) {
		done = 0;
		// set S-flag to 1 and wait until F-flag change to 1
		sc[S[i]] = 0x80000000;
		do {
			done = sc[S[i]] & 0x01000000;
		} while(!done);
		// turnoff S-flag and read the value
		sc[S[i]] = sc[S[i]] & 0x0FFFFFFF;
		tempVal[i] = sc[S[i]] & 0x00FFFFFF;
	}

#elif (READING_VERSION==2)
	uint k, temp1, temp2, temp3;

	// Start tempearture measurement
	sc[SC_TS0] = 0x80000000;
	// Wait for measurement TS0 to finish
	k = 0;
	while(!(sc[SC_TS0] & (1<<24))) k++;
	// Get value
	temp1 = sc[SC_TS0] & 0x00ffffff;
	// Stop measurement
	sc[SC_TS0] = 0<<31;
	//io_printf(IO_BUF, "k(T1):%d\n", k);

	// Start tempearture measurement
	sc[SC_TS1] = 0x80000000;
	// Wait for measurement TS1 to finish
	k=0;
	while(!(sc[SC_TS1] & (1<<24))) k++;
	// Get value
	temp2 = sc[SC_TS1] & 0x00ffffff;
	// Stop measurement
	sc[SC_TS1] = 0<<31;
	//io_printf(IO_BUF, "k(T2):%d\n", k);

	// Start tempearture measurement
	sc[SC_TS2] = 0x80000000;
	// Wait for measurement TS2 to finish
	k=0;
	while(!(sc[SC_TS2] & (1<<24))) k++;
	// Get value
	temp3 = sc[SC_TS2] & 0x00ffffff;
	// Stop measurement
	sc[SC_TS2] = 0<<31;
	//io_printf(IO_BUF, "k(T3):%d\n\n", k);
	tempVal[0] = temp1;
	tempVal[1] = temp2;
	tempVal[2] = temp3;
#endif
}

// getFreqParams() read parameter from memTable
void getFreqParams(uint f, uint *ms, uint *ns)
{
	uint i;
	for(i=0; i<lnMemTable; i++)
		if(f == (uint)memTable[i][0]) {
			*ms = (uint)memTable[i][1];
			*ns = (uint)memTable[i][2];
			break;
		}
}

// changeFreq() assumes that all cores (CPU-A and CPU-B) are controlled by PLL1
void changeFreq(uint f, uint null)
{
	r20 = sc[SC_PLL1];

	uint ns, ms;
	getFreqParams(f, &ms, &ns);
	r20 &= 0xFFFFC0C0;			// apply masking at MS and NS
	r20 |= ns;					// apply NS
	r20 |= (ms << 8);			// apply MS
	sc[SC_PLL1] = r20;			// change the value of r20 with the new parameters

	_freq = f;

	io_printf(IO_BUF, "The cpu clock is set to %uMHz\n\n", f);
}

/* NOTE: readSpinFreqVal() assumes that the value of _dv_ is always 2 */
uint readSpinFreqVal()
{
	uint i, MS1, NS1, f = sv->cpu_clk;
	r20 = sc[SC_PLL1];

	MS1 = (r20 >> 8) & 0x3F;
	NS1 = r20 & 0x3F;

	for(i=0; i<lnMemTable; i++) {
		if(memTable[i][1]==MS1 && memTable[i][2]==NS1) {
			f = memTable[i][0];
			break;
		}
	}
	return f;
}

void readSCP(uint mailbox, uint port)
{
	sdp_msg_t *msg = (sdp_msg_t *) mailbox;
	// use MY_SDP_PORT for communication with the host via eth
	if(port==DEF_SPINN_SDP_PORT) {
		switch(msg->cmd_rc) {
		case HOST_REQ_PLL_INFO:
			spin1_schedule_callback(showPLLinfo, 0, 0, NON_CRITICAL_PRIORITY_VAL);
		case HOST_SET_CHANGE_PLL:
			// the "flag" parameter for changePLL() is specified in msg->seq, with valid value: 0, 1, or 2
			spin1_schedule_callback(changePLL, msg->seq, msg->cmd_rc, NON_CRITICAL_PRIORITY_VAL);
			break;
		case HOST_REQ_REVERT_PLL:
			// the "flag" parameter for changePLL() is set to 0 (or PLL_ORIGINAL)
			spin1_schedule_callback(changePLL, PLL_ORIGINAL, msg->cmd_rc, NON_CRITICAL_PRIORITY_VAL);
			break;
		case HOST_SET_FREQ_VALUE:
			// Note: HOST_SET_FREQ_VALUE assumes that CPUs use PLL1
			//		 if this is not the case, then use HOST_SET_CHANGE_PLL
			spin1_schedule_callback(changeFreq, msg->seq, HOST_SET_FREQ_VALUE, NON_CRITICAL_PRIORITY_VAL);
			break;
		}
	}

	spin1_msg_free (msg);
}

/* SYNOPSIS
 * 		changePLL()
 * IDEA:
 *		The clock frequency of cpus are determined by PLL1. It is better if clock source for System
 * 		AHB and Router is changed to PLL2, so that we can play only with the cpu.
 *		Using my readPLL.aplx, I found that system AHB and router has 133MHz. That's why, we should
 *		change its divisor to 2, so that the frequency of these elements are kept around 130MHz
 *
 * In function changePLL(), the flag in changePLL means:
 * 0: set to original value
 * 1: set PLL2 for system AHB and router, and PLL1 just for CPUs
 * 2: set system AHB and router to use the same clock as CPUs
*/
void changePLL(uint flag, uint replyCode)
{
	if(flag==PLL_ORIGINAL) {
		sc[SC_CLKMUX] = _r24;
		sc[SC_PLL1] = _r20;
		sc[SC_PLL2] = _r21;
	}
	else if(flag==PLL_ISOLATE_CPUS) {
		r24 = sc[SC_CLKMUX];
		/* Let's change so that System AHB and Router use PLL2 */
		io_printf(IO_BUF, "System AHB and Router divisor will be changed to 2 instead of 3\n");
		// the System AHB
		r24 &= 0xFF0FFFFF; //clear "Sdiv" and "Sys"
		r24 |= 0x00600000; //set Sdiv = 2 and "Sys" = 2
		// the Router
		r24 &= 0xFFF87FFF; //clear "Rdiv" and "Rtr"
		r24 |= 0x00030000; //set Rdiv = 2 and "Rtr" = 2
		io_printf(IO_BUF, "System AHB and Router is set to PLL2\n");
		sc[SC_CLKMUX] = r24;
	}
	else if(flag==PLL_ONLY_SDRAM_IN_PLL2) {
		r24 = sc[SC_CLKMUX];
		io_printf(IO_BUF, "System AHB and Router divisor will be changed to 2 instead of 3\n");
		// the System AHB
		r24 &= 0xFF0FFFFF; //clear "Sdiv" and "Sys"
		r24 |= 0x00500000; //set Sdiv = 2 and "Sys" = 1
		// the Router
		r24 &= 0xFFF87FFF; //clear "Rdiv" and "Rtr"
		r24 |= 0x00028000; //set Rdiv = 2 and "Rtr" = 1
		io_printf(IO_BUF, "System AHB and Router is set to PLL1 just like CPUs\n");
		sc[SC_CLKMUX] = r24;
	}
}

REAL getFreq(uchar sel, uchar dv)
{
    REAL fSrc, num, denum, _dv_, val;
    _dv_ = dv;
    switch(sel) {
    case 0: num = REAL_CONST(1.0); denum = REAL_CONST(1.0); break; // 10 MHz clk_in
    case 1: num = NS1; denum = MS1; break;
    case 2: num = NS2; denum = MS2; break;
    case 3: num = REAL_CONST(1.0); denum = REAL_CONST(4.0); break;
    }
    fSrc = REAL_CONST(10.0);
    val = (fSrc * num) / (denum * _dv_);
    //io_printf(IO_BUF, "ns = %u, ms = %u, fSrc = %k, num = %k, denum = %k, dv = %k, val = %k\n", ns, ms, fSrc, num, denum, dv, val);
    return val;
}

void get_FR_str(uchar fr)
{
    sark_delay_us(1000);
    switch(fr) {
    case 0:
        io_printf(IO_BUF, "25-50 MHz\n"); 
        break;
    case 1:
	io_printf(IO_BUF, "50-100 MHz\n"); 
        break;
    case 2:
	io_printf(IO_BUF, "100-200 MHz\n"); 
        break;
    case 3:
	io_printf(IO_BUF, "200-400 MHz\n"); 
        break;
    }

}

char *selName(uchar s)
{
    char *name;
    switch(s) {
    case 0: name = "clk_in"; break;
    case 1: name = "pll1_clk"; break;
    case 2: name = "pll2_clk"; break;
    case 3: name = "clk_in_div_4"; break;
    }
    return name;
}

void showPLLinfo(uint arg0, uint arg1)
{
    r20 = sc[SC_PLL1];
    r21 = sc[SC_PLL2];
    r24 = sc[SC_CLKMUX];

	io_printf(IO_BUF, "Register content: r20=0x%x, r21=0x%x, r24=0x%x\n\n", r20, r21, r24);

    FR1 = (r20 >> 16) & 3;
    MS1 = (r20 >> 8) & 0x3F;
    NS1 = r20 & 0x3F;
    FR2 = (r21 >> 16) & 3;
    MS2 = (r21 >> 8) & 0x3F;
    NS2 = r21 & 0x3F;

    Sdiv = ((r24 >> 22) & 3) + 1;
    Sys_sel = (r24 >> 20) & 3;
    Rdiv = ((r24 >> 17) & 3) + 1;
    Rtr_sel = (r24 >> 15) & 3;
    Mdiv = ((r24 >> 12) & 3) + 1;
    Mem_sel = (r24 >> 10) & 3;
    Bdiv = ((r24 >> 7) & 3) + 1;
    Pb = (r24 >> 5) & 3;
    Adiv = ((r24 >> 2) & 3) + 1;
    Pa = r24 & 3;

    Sfreq = getFreq(Sys_sel, Sdiv);
    Rfreq = getFreq(Rtr_sel, Rdiv);
    Mfreq = getFreq(Mem_sel, Mdiv);
    Bfreq = getFreq(Pb, Bdiv);
    Afreq = getFreq(Pa, Adiv);

    io_printf(IO_BUF, "************* CLOCK INFORMATION **************\n"); 
    io_printf(IO_BUF, "Reading sark library...\n"); 
    io_printf(IO_BUF, "Clock divisors for system & router bus: %u\n", sv->clk_div); 
    io_printf(IO_BUF, "CPU clock in MHz   : %u\n", sv->cpu_clk); 
    io_printf(IO_BUF, "SDRAM clock in MHz : %u\n\n", sv->mem_clk); 

    io_printf(IO_BUF, "Reading registers directly...\n"); 
    io_printf(IO_BUF, "PLL-1\n"); 
    io_printf(IO_BUF, "----------------------------\n"); 
    io_printf(IO_BUF, "Frequency range      : "); get_FR_str(FR1);
    io_printf(IO_BUF, "Output clk divider   : %u\n", MS1); 
    io_printf(IO_BUF, "Input clk multiplier : %u\n\n", NS1); 

    io_printf(IO_BUF, "PLL-2\n"); 
    io_printf(IO_BUF, "----------------------------\n"); 
    io_printf(IO_BUF, "Frequency range      : "); get_FR_str(FR2);
    io_printf(IO_BUF, "Output clk divider   : %u\n", MS2); 
    io_printf(IO_BUF, "Input clk multiplier : %u\n\n", NS2); 

    io_printf(IO_BUF, "Multiplerxer\n"); 
    io_printf(IO_BUF, "----------------------------\n"); 
    io_printf(IO_BUF, "System AHB clk divisor  : %u\n", Sdiv); 
    io_printf(IO_BUF, "System AHB clk selector : %u (%s)\n", Sys_sel, selName(Sys_sel)); 
    io_printf(IO_BUF, "System AHB clk freq     : %k MHz\n", Sfreq); 
    io_printf(IO_BUF, "Router clk divisor      : %u\n", Rdiv); 
    io_printf(IO_BUF, "Router clk selector     : %u (%s)\n", Rtr_sel, selName(Rtr_sel)); 
    io_printf(IO_BUF, "Router clk freq         : %k MHz\n", Rfreq); 
    io_printf(IO_BUF, "SDRAM clk divisor       : %u\n", Mdiv); 
    io_printf(IO_BUF, "SDRAM clk selector      : %u (%s)\n", Mem_sel, selName(Mem_sel)); 
    io_printf(IO_BUF, "SDRAM clk freq          : %k MHz\n", Mfreq); 
    io_printf(IO_BUF, "CPU-B clk divisor       : %u\n", Bdiv); 
    io_printf(IO_BUF, "CPU-B clk selector      : %u (%s)\n", Pb, selName(Pb)); 
    io_printf(IO_BUF, "CPU-B clk freq          : %k MHz\n", Bfreq); 
    io_printf(IO_BUF, "CPU-A clk divisor       : %u\n", Adiv); 
    io_printf(IO_BUF, "CPU-A clk selector      : %u (%s)\n", Pa, selName(Pa)); 
    io_printf(IO_BUF, "CPU-A clk freq          : %k MHz\n", Afreq); 
    io_printf(IO_BUF, "**********************************************\n\n\n");
}

void sendReport(uint arg0, uint arg1)
{
	// read temperature sensor
	readTemp(0,0);
	// put in arg1, arg2, and arg3
	report.arg1 = tempVal[0];
	report.arg2 = tempVal[1];
	report.arg3 = tempVal[2];
	spin1_send_sdp_msg (&report, DEF_TIMEOUT);
}

void c_main(void)
{
	if(sark_app_id() != DEF_MY_APP_ID) {
		io_printf(IO_STD, "Please assign me app-ID %u\n", DEF_MY_APP_ID);
		return;
	}

	// get the original PLL configuration and current frequency
	// Note: r20-->PLL1_controller, r21-->PLL2_controller, r24-->clock_mux
	//       In normal operation, CPU-A and CPU-B use PLL1
	_r20 = 0x70128;										// _r20 = sc[SC_PLL1];
	_r21 = 0x7011a;										// _r21 = sc[SC_PLL2];
	_r24 = 0x809488a5;									// _r24 = sc[SC_CLKMUX];
	_freq = readSpinFreqVal();

	szHeaderOnly = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t);
	report.tag = DEF_REPORT_IPTAG;						// Don't forget to specify the proper iptag in ybug
	report.dest_port = PORT_ETH;						// Ethernet
	report.dest_addr = sv->eth_addr;					// Nearest Ethernet chip
	report.flags = 0x07;								// no need for reply
	report.srce_port = DEF_SPINN_SDP_PORT;
	report.srce_addr = sv->p2p_addr;
	report.cmd_rc = PROFILER_VERSION;
	report.seq = 0;
	report.length = szHeaderOnly;

	// Anything else
	uint myChipID = sark_chip_id();
	uint myCoreID = sark_core_id();
	io_printf(IO_STD, "Profiler is started at <%u,%u,%u>!\n", myChipID >> 8, myChipID & 0xFF, myCoreID);

	// setup Timer for reporting 
	spin1_set_timer_tick(REPORT_TIMER_TICK_PERIOD_US);
	spin1_callback_on(TIMER_TICK, sendReport, APP_PRIORITY_VAL);

	// setup SCP (SDP) handler
	spin1_callback_on(SDP_PACKET_RX, readSCP, SCP_PRIORITY_VAL);

	spin1_start(SYNC_NOWAIT);
}

