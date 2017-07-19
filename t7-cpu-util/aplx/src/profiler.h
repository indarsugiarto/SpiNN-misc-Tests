/****
* PROFILER_VERSION 444
* SUMMARY
*  - Read the SpiNNaker chip states: temperature sensor, clock frequency, 
*    ,number of running cores, and cpu loads.
*  - The master (root) profiler stays in chip <0,0>. All other profilers 
*    report to this master profiler and only the master profiler send
*    the combined profile report to the host.
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

/* Version 444 is designed to overcome the problem of modifying clock of the master profiler
 * in version 333:
 *    we used synchronized clock mechanism: root profiler will broadcast MCPL_GLOBAL_TICK
 *    every GLOBAL_TICK_PERIOD_US microseconds. NOTE: the root profiler SHOULD ALWAYS HAVE 200MHz,
 *    otherwise this mechanism won't work!
 *
 * In version 444, we use slow clock timer for cpu load counting.
 * When interrupt happens, each profiler will:
 * - read temperature
 * - read frequency
 * - read how many cores are active
 * - cpu load
 *
 * */


/* SOME IDEAS:
 * -------------
 */

#ifndef PROFILER_H
#define PROFILER_H

// We have several profiler version. For this program, use this version:
#define PROFILER_VERSION			444	// put this version in the vcpu->user0

#include <spin1_api.h>
#include <stdfix.h>

#define DEBUG_LEVEL					0

#define REAL						accum
#define REAL_CONST(x)				x##k

#define DEF_DELAY_VAL				1000	// used mainly for io_printf
#define DEF_MY_APP_ID				255
#define DEF_PROFILER_CORE			1

#define REPORT_TIMER_TICK_PERIOD_US	1000	// 1ms in 200MHz clock
#define GLOBAL_TICK_PERIOD_US		1000	// 1ms in 200MHz clock

// priority setup
// Note: somehow I cannot set the MCPL priority to -1 (for FIQ)
#define SCP_PRIORITY_VAL			0
#define MCPL_PRIORITY_VAL			1     
#define APP_PRIORITY_VAL			2
#define LOW_PRIORITY_VAL			2
#define TEMP_TIMER_PRIORITY_VAL		3
#define NON_CRITICAL_PRIORITY_VAL	4
#define LOWEST_PRIORITY_VAL			4		// I found from In spin1_api_params.h I found this: #define NUM_PRIORITIES    5
//#define IDLE_PRIORITY_VAL			NON_CRITICAL_PRIORITY_VAL
#define IDLE_PRIORITY_VAL			LOWEST_PRIORITY_VAL


// SDP-related parameters
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

// related with frequency scalling
#define PLL_ORIGINAL				0
#define PLL_ISOLATE_CPUS			1
#define PLL_ONLY_SDRAM_IN_PLL2		2

#define lnMemTable					93
#define wdMemTable					3
// memTable format: freq, MS, NS --> with default dv = 2, so that we don't have
// to modify r24
static uchar memTable[lnMemTable][wdMemTable] = {
{10,1,2},
{11,5,11},
{12,5,12},
{13,5,13},
{14,5,14},
{15,1,3},
{16,5,16},
{17,5,17},
{18,5,18},
{19,5,19},
{20,1,4},
{21,5,21},
{22,5,22},
{23,5,23},
{24,5,24},
{25,1,5},
{26,5,26},
{27,5,27},
{28,5,28},
{29,5,29},
{30,1,6},
{31,5,31},
{32,5,32},
{33,5,33},
{34,5,34},
{35,1,7},
{36,5,36},
{37,5,37},
{38,5,38},
{39,5,39},
{40,1,8},
{41,5,41},
{42,5,42},
{43,5,43},
{44,5,44},
{45,1,9},
{46,5,46},
{47,5,47},
{48,5,48},
{49,5,49},
{50,1,10},
{51,5,51},
{52,5,52},
{53,5,53},
{54,5,54},
{55,1,11},
{56,5,56},
{57,5,57},
{58,5,58},
{59,5,59},
{60,1,12},
{61,5,61},
{62,5,62},
{63,5,63},
{65,1,13},
{70,1,14},
{75,1,15},
{80,1,16},
{85,1,17},
{90,1,18},
{95,1,19},
{100,1,20},
{105,1,21},
{110,1,22},
{115,1,23},
{120,1,24},
{125,1,25},
{130,1,26},
{135,1,27},
{140,1,28},
{145,1,29},
{150,1,30},
{155,1,31},
{160,1,32},
{165,1,33},
{170,1,34},
{175,1,35},
{180,1,36},
{185,1,37},
{190,1,38},
{195,1,39},
{200,1,40},
{205,1,41},
{210,1,42},
{215,1,43},
{220,1,44},
{225,1,45},
{230,1,46},
{235,1,47},
{240,1,48},
{245,1,49},
{250,1,50},
{255,1,51},
};

#define lnIDTable					48
#define wdIDTable					2
static uchar profIDTable[lnIDTable][wdIDTable] = {
{0,0},{1,0},{2,0},{3,0},{4,0},
{0,1},{1,1},{2,1},{3,1},{4,1},{5,1},
{0,2},{1,2},{2,2},{3,2},{4,2},{5,2},{6,2},
{0,3},{1,3},{2,3},{3,3},{4,3},{5,3},{6,3},{7,3},
      {1,4},{2,4},{3,4},{4,4},{5,4},{6,4},{7,4},
            {2,5},{3,5},{4,5},{5,5},{6,5},{7,5},
                  {3,6},{4,6},{5,6},{6,6},{7,6},
                        {4,7},{5,7},{6,7},{7,7}};

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
/*-------------------------------------- end of Reporting Data Structure --*/


/* global variables */
ushort myID;

// reading temperature sensors
uint tempVal[3];							// there are 3 sensors in each chip

// PLL and frequency related:
uint _r20, _r21, _r24;						// the original value of r20, r21, and r24
uint r20, r21, r24;							// the current value of r20, r21 and r24 during *this* experiment
uint _freq;
uchar FR1, MS1, NS1, FR2, MS2, NS2;
uchar Sdiv, Sys_sel, Rdiv, Rtr_sel, Mdiv, Mem_sel, Bdiv, Pb, Adiv, Pa;
REAL Sfreq, Rfreq, Mfreq, Bfreq, Afreq;


// routing mechanism
#define MCPL_BCAST_REQ_UPDATE 				0xbca50001	// broadcast from root to all other profilers
#define MCPL_PROFILERS_REPORT_PART1			0x21ead100	// go to leader in chip-0
#define MCPL_PROFILERS_REPORT_PART2			0x21ead200	// go to leader in chip-0
#define MCPL_GLOBAL_TICK					MCPL_BCAST_REQ_UPDATE	// just an alias

/*-------------------------------------------------------------------------------------------*/

// function prototypes
void readTemp(uint arg1, uint arg2);
void readSCP(uint mailbox, uint port);
void getFreqParams(uint f, uint *ms, uint *ns);	// read parameter from memTable
void changeFreq(uint f, uint replyCode);        // we use uchar to limit the frequency to 255
void changePLL(uint flag, uint replyCode);
REAL getFreq(uchar sel, uchar dv);	            // for displaying purpose only (decimal number)
uint readSpinFreqVal();
void sendReport(uint arg0, uint arg1);
char *selName(uchar s);
void get_FR_str(uchar fr);
void showPLLinfo(uint arg0, uint arg1);
void reportToHost(uint arg0, uint arg1);
short generateProfilerID();
uchar getNumActiveCores();
void initRouter();                              // sub-profilers report to the root profiler
void hMCPL(uint key, uint payload);

// for debugging
#define DEBUG_LEVEL 0
static uint cntr=0;

#endif // PROFILER_H
