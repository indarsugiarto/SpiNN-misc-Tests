#include <math.h>
#include "profiler.h"


/*----------------------------------------------------------------------*/
/*------------------------- Freq/PLL Manipulation ----------------------*/

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

// changeFreq() assumes that all cores (CPU-A and CPU-B) are controlled by PLL1
void changeFreq(uint f, uint null)
{

    // first, we need to go into PLL_ISOLATE_CPUS mode
    //io_printf(IO_STD, "Setting PLL into mode PLL_ISOLATE_CPUS...\n");
    changePLL(PLL_ISOLATE_CPUS,0);

    // then alter PLL1 (specified by r20)
    //io_printf(IO_STD, "Altering r20...\n");
    r20 = sc[SC_PLL1];

    uint ns, ms;
    getFreqParams(f, &ms, &ns);
    //io_printf(IO_STD, "Check: ms==%u, ns==%u\n", ms, ns);
    r20 &= 0xFFFFC0C0;            // apply masking at MS and NS
    r20 |= ns;                    // apply NS
    r20 |= (ms << 8);            // apply MS
    sc[SC_PLL1] = r20;            // change the value of r20 with the new parameters

    _freq = f;
#if(DEBUG_LEVEL>0)
    io_printf(IO_BUF, "The cpu clock is set to %uMHz\n\n", f);
#endif
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

/* SYNOPSIS
 *         changePLL()
 * IDEA:
 *        The clock frequency of cpus are determined by PLL1. It is better if clock source for System
 *         AHB and Router is changed to PLL2, so that we can play only with the cpu.
 *        Using my readPLL.aplx, I found that system AHB and router has 133MHz. That's why, we should
 *        change its divisor to 2, so that the frequency of these elements are kept around 130MHz
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
#if(DEBUG_LEVEL>0)
        io_printf(IO_BUF, "System AHB and Router divisor will be changed to 2 instead of 3\n");
#endif
        // the System AHB
        r24 &= 0xFF0FFFFF; //clear "Sdiv" and "Sys"
        r24 |= 0x00600000; //set Sdiv = 2 and "Sys" = 2
        // the Router
        r24 &= 0xFFF87FFF; //clear "Rdiv" and "Rtr"
        r24 |= 0x00030000; //set Rdiv = 2 and "Rtr" = 2
#if(DEBUG_LEVEL>0)
        io_printf(IO_BUF, "System AHB and Router is set to PLL2\n");
#endif
        sc[SC_CLKMUX] = r24;
    }
    else if(flag==PLL_ONLY_SDRAM_IN_PLL2) {
        r24 = sc[SC_CLKMUX];
#if(DEBUG_LEVEL>0)
        io_printf(IO_BUF, "System AHB and Router divisor will be changed to 2 instead of 3\n");
#endif
        // the System AHB
        r24 &= 0xFF0FFFFF; //clear "Sdiv" and "Sys"
        r24 |= 0x00500000; //set Sdiv = 2 and "Sys" = 1
        // the Router
        r24 &= 0xFFF87FFF; //clear "Rdiv" and "Rtr"
        r24 |= 0x00028000; //set Rdiv = 2 and "Rtr" = 1
#if(DEBUG_LEVEL>0)
        io_printf(IO_BUF, "System AHB and Router is set to PLL1 just like CPUs\n");
#endif
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

