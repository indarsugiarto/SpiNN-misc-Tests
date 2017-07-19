/*----------------------------------------------------------------------*/
/*------------------------- Freq/PLL Manipulation ----------------------*/

/* **************************
 * The most useful functions:
 * initPLL()
 * changeFreq()
 * showPLLinfo()
 * revertPLL()
 ****************************/

#include <math.h>
#include "profiler.h"


#define lnMemTable					93
#define wdMemTable					3
// memTable format: freq, MS, NS --> with default dv = 2, so that we don't have
// to modify r24
static const uchar memTable[lnMemTable][wdMemTable] = {
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

// Variables:
uint _r20, _r21, _r24;		// the original value of r20, r21, and r24
uint r20, r21, r24;			// the current value of r20, r21 and r24
static uint isModified = FALSE;

// Local prototypes
static void changePLL(uint flag);
static void getFreqParams(uint f, uint *ms, uint *ns);

/**************************************************************************
 * initPLL() will put system AHB and router into PLL2 so that PLL1 will
 * be used exclusively for cpu clocks.
 * The reason is because we want to have "fine-grained" frequency control
 * for cpus, hence we have to change the PLL frequency. Altering the PLL
 * frequency will affect component (like system AHB and router) frequency.
 * When we change the PLL frequency to achieve certain cpu clock,
 * we don't want the system AHB nor router clock be altered.
 **************************************************************************/

void initPLL()
{
    // get the initial PLL values
    _r24 = sc[SC_CLKMUX];
    _r20 = sc[SC_PLL1];
    _r21 = sc[SC_PLL2];

    // change PLL to exclusive mode
    changePLL(1);
    isModified = TRUE;
}

/***************************************************************************
 * changeFreq() assumes that all cores (CPU-A and CPU-B) are controlled by
 * PLL1 to achieve fine-grained control. System, Router, and SDRAM will be
 * controlled by PLL-2.
 * component definition:
 * 0 - cpu
 * 1 - system AHB
 * 2 - router
 * Due to SDRAM requires 260MHz, we cannot change PLL2 because PLL2 is
 * set to 520MHz already.
 **************************************************************************/
void changeFreq(uint component, uint f)        // we use uchar to limit the frequency to 255
{
    if(isModified==FALSE) {
#if(DEBUG_LEVEL>0
        io_printf(IOBUF, "[ERR] Not available!\n");
#endif
        return;
    }
    if(component==0) {
        // then alter PLL1 (specified by r20)
        r20 = sc[SC_PLL1];

        uint ns = 0;
        uint ms = 0;
        getFreqParams(f, &ms, &ns);
        if(ms==0 && ns==0) { //not found in table
#if(DEBUG_LEVEL>0
        io_printf(IOBUF, "[ERR] Not available!\n");
#endif
            return;
        }
        //io_printf(IO_STD, "Check: ms==%u, ns==%u\n", ms, ns);
        r20 &= 0xFFFFC0C0;            // apply masking at MS and NS
        r20 |= ns;                    // apply NS
        r20 |= (ms << 8);            // apply MS
        sc[SC_PLL1] = r20;            // change the value of r20 with the new parameters

        curr_freq = f;
    }
    else if(component==1) {         // for system AHB
        // here the only possible value is 130 or 173.33
        if(f==130) {

        }
        else {

        }
    }
    else if(component==2) {         // for router
        // here the only possible value is 130 or 173.33
        if(f==130) {

        }
        else {

        }
    }
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
    //io_printf(IO_STD, "Found ms==%u, ns==%u\n", *ms, *ns);
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

/* SYNOPSIS
 *         changePLL()
 * IDEA:
 * - PLL-1 will be used exclusively for cpu
 * - PLL-2 will be used for SDRAM, System, and router
 *
 * PLL-2 can be set to 520MHz:
 * - divided by two will yield 260MHz for SDRAM
 * - divided by four will yield 130MHz for AHB and router, OR
 *   divided by three will yield 173.33MHz --> try this!
 *
 * In function changePLL(), the flag in changePLL means:
 * 0: set to original value
 * 1: set PLL2 for system AHB and router, and PLL1 just for CPUs
*/
void changePLL(uint flag)
{
    if(flag==0) {
        if(isModified==TRUE) {
            sc[SC_CLKMUX] = _r24;
            sc[SC_PLL1] = _r20;
            sc[SC_PLL2] = _r21;
        }
    }
    else {
        r24 = sc[SC_CLKMUX];
        /* Let's change so that System AHB and Router use PLL2
         * with initial frequencies are set to: 260 for SDRAM, 130
         * for System and Router. Bits location in r24:
           Sdiv[1:0]    23:22
           Sys[1:0]     21:20
           Rdiv[1:0]    18:17
           Rtr[1:0]     16:15
           Mdiv[1:0]    13:12
           Mem[1:0]     11:10

         ***********************/
#if(DEBUG_LEVEL>0)
        io_printf(IO_BUF, "System AHB and Router is set to PLL2\n");
        io_printf(IO_BUF, "System AHB and Router divisor will be changed to 4\n");
        io_printf(IO_BUF, "SDRAM divisor will be change to 2\n");
        io_printf(IO_BUF, "PLL-2 will be set to 520MHz\n";
#endif
        // the System AHB
        r24 &= 0xFF0FFFFF; //clear "Sdiv" and "Sys"
        r24 |= (0xE << 20); //set Sdiv = 4 and "Sys" = 2
        // the Router
        r24 &= 0xFF0FFFFF; //clear "Sdiv" and "Sys"
        r24 |= (0xE << 15); //set Rdiv = 4 and "Rtr" = 2
        // the SDRAM
        r24 &= 0xFFFFC3FF; // clear "Mdiv" and "Mem"
        r24 |= (0x6 << 10); // set Mdiv = 2 and Mem = 2
        sc[SC_CLKMUX] = r24;

#if(DEBUG_LEVEL>0)
        io_printf(IO_BUF, "PLL-2 will be set to 520MHz\n";
#endif
        r21 &= 0xFFFFC0C0;            // apply masking at MS and NS
        r21 |= 52;                    // apply NS
        r21 |= (1 << 8);              // apply MS
        sc[SC_PLL2] = r21;            // change the value of r20 with the new parameters
    }
}
