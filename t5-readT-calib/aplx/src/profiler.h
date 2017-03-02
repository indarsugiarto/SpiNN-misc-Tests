/* Modified profiler for t5-readT-calib:
 * 
 * - Each chip reads its sensors and send them directly to host-PC
 * - 100ms timer2 is used
 * - This profiler can be placed in core-17, so that it won't interfere
 *   with normal programs that are placed in core-1 to core-16
 * - It won't report cpu load
 * - It can still change chip freq. The host can also ask the chip to
 *   revert back its previous PLL configuration.
 */

#ifndef PROFILER_H
#define PROFILER_H

// We have several profiler version. For this program, use this version:
#define PROFILER_VERSION			222

#include <spin1_api.h>
#include <stdfix.h>

#define REAL						accum
#define REAL_CONST(x)				x##k

#define DEF_DELAY_VAL				1000	// used mainly for io_printf
#define DEF_MY_APP_ID				255
#define DEF_PROFILER_CORE			17

#define REPORT_TIMER_TICK_PERIOD_US	100000	// to get 0.1s resolution in FREQ_REF_200MHZ

// priority setup
#define SCP_PRIORITY_VAL			0
#define APP_PRIORITY_VAL			1
#define LOW_PRIORITY_VAL			2
#define TEMP_TIMER_PRIORITY_VAL		3
#define NON_CRITICAL_PRIORITY_VAL	3
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

/* global variables */
sdp_msg_t report;							// for other parameters report
uint szHeaderOnly;

// reading temperature sensors
uint tempVal[3];							// there are 3 sensors in each chip

// PLL and frequency related:
uint _r20, _r21, _r24;						// the original value of r20, r21, and r24
uint r20, r21, r24;							// the current value of r20, r21 and r24 during *this* experiment
uint _freq;
uchar FR1, MS1, NS1, FR2, MS2, NS2;
uchar Sdiv, Sys_sel, Rdiv, Rtr_sel, Mdiv, Mem_sel, Bdiv, Pb, Adiv, Pa;
REAL Sfreq, Rfreq, Mfreq, Bfreq, Afreq;

/*-------------------------------------------------------------------------------------------*/

// function prototypes
void readTemp(uint arg1, uint arg2);
void readSCP(uint mailbox, uint port);
void getFreqParams(uint f, uint *ms, uint *ns);				// read parameter from memTable
void changeFreq(uint f, uint replyCode);					// we use uchar to limit the frequency to 255
void changePLL(uint flag, uint replyCode);
REAL getFreq(uchar sel, uchar dv);							// for displaying purpose only (decimal number)
uint readSpinFreqVal();
void sendReport(uint arg0, uint arg1);
char *selName(uchar s);
void get_FR_str(uchar fr);
void showPLLinfo(uint arg0, uint arg1);


#endif // PROFILER_H
