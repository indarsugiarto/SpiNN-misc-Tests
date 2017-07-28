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

#define DEBUG_LEVEL					1

/*--------- Some functionalities needs fix point representation ----------*/
#define REAL						accum
#define REAL_CONST(x)				(x##k)
/*------------------------------------------------------------------------*/


#define DEF_DELAY_VAL				1000	// used mainly for io_printf
#define DEF_MY_APP_ID				255
#define DEF_PROFILER_CORE			1

// priority setup
// we allocate FIQ for slow timer
#define SCP_PRIORITY_VAL			0
#define MCPL_PRIORITY_VAL			1     
#define APP_PRIORITY_VAL			2
#define LOW_PRIORITY_VAL			2
#define TIMER_PRIORITY_VAL          3
#define NON_CRITICAL_PRIORITY_VAL	4
#define LOWEST_PRIORITY_VAL			4		// I found from In spin1_api_params.h I found this: #define NUM_PRIORITIES    5
#define IDLE_PRIORITY_VAL			LOWEST_PRIORITY_VAL
#define SCHEDULED_PRIORITY_VAL      1


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
#define HOST_REQ_PROFILER_STREAM	5		// host send this to a particular profiler to start streaming
#define HOST_TELL_STOP				6


/*--------------------- Reporting Data Structure -------------------------*/
/* The idea is: since this version uses slow recording, then each profiler
 * can report directly to host-PC via SDP.
/*------------------------------------------------------------------------*/
typedef struct pro_info {
    ushort v;           // profiler version, in cmd_rc
    ushort pID;         // which chip sends this?, in seq
    uchar cpu_freq;     // in arg1
    uchar rtr_freq;     // in arg1
    uchar ahb_freq;     // in arg1
    uchar nActive;      // in arg1
    ushort  temp1;         // from sensor-1, for arg2
    ushort temp3;         // from sensor-3, for arg2
    uint memfree;       // optional, sdram free (in bytes), in arg3
	//uchar load[18];
} pro_info_t;

pro_info_t myProfile;
ushort my_pID;

/*====================== PLL-related Functions =======================*/

enum PLL_COMP_e {PLL_CPU, PLL_AHB, PLL_RTR};
typedef enum PLL_COMP_e PLL_PART;

void initPLL();                             // set PLLs to fine-grained mode
void changeFreq(PLL_PART component, uint f);
void showPLLinfo(uint output, uint arg1);
void revertPLL();                           // return back PLL configuration
uchar readFreq(uchar *fAHB, uchar *fRTR);		// read freqs of three components


/*====================== CPUidle-related Functions =======================*/
uchar running_cpu_idle_cntr[18];
uchar stored_cpu_idle_cntr[18];
uint idle_cntr_cntr; //master counter that count up to 100
void init_idle_cntr();
void startProfiling(uint null, uint nill);
void stopProfiling(uint null, uint nill);


/*==================== Temperature-related Functions =====================*/
uint tempVal[3];							// there are 3 sensors in each chip
uint readTemp();                            // in addition, sensor-2 will be returned


/*======================= Event-related Functions =====================*/
void init_Router();                         // sub-profilers report to the root profiler
void init_Handlers();



/*======================== Misc. Functions ===========================*/
void sanityCheck();
void generateProfilerID();
void print_cntr(uint null, uint nill);
uchar getNumActiveCores();


/*********************** Logging mechanism ****************************
 * For logging, the host may send a streaming request to a certain
 * profiler.
 * */
void collectData(uint None, uint Unused);
volatile uint streaming; // do we need streaming? initially no!

#endif // PROFILER_H
