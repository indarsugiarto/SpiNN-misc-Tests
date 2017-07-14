#include <spin1_api.h>
#include <stdfix.h>

#define REAL 		accum
#define REAL_CONST(x)	x##k

uchar FR1, MS1, NS1, FR2, MS2, NS2;
uchar Sdiv, Sys_sel, Rdiv, Rtr_sel, Mdiv, Mem_sel, Bdiv, Pb, Adiv, Pa;
REAL Sfreq, Rfreq, Mfreq, Bfreq, Afreq;

void get_FR_str(uchar fr)
{
    sark_delay_us(1000);
    switch(fr) {
    case 0:
        io_printf(IO_STD, "25-50 MHz\n"); sark_delay_us(1000);
        break;
    case 1:
	io_printf(IO_STD, "50-100 MHz\n"); sark_delay_us(1000);
        break;
    case 2:
	io_printf(IO_STD, "100-200 MHz\n"); sark_delay_us(1000);
        break;
    case 3:
	io_printf(IO_STD, "200-400 MHz\n"); sark_delay_us(1000);
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

void c_main(void)
{
    uint x = sark_chip_id() >> 8;
    uint y = sark_chip_id() & 0xFF;
    spin1_delay_us((x*2+y)*1000000); 

    uint r20 = sc[SC_PLL1];
    uint r21 = sc[SC_PLL2];
    uint r24 = sc[SC_CLKMUX];
    io_printf(IO_STD, "Starting of c_main at 0x%x with r20=0x%x, r21=0x%x, r24=0x%x\n\n", c_main, r20, r21, r24); sark_delay_us(1000);

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

    // Note on SDRAM:
    // internally, it has a by-2-divider, so we must take it into account
    Mfreq *= 0.5;

    io_printf(IO_STD, "************* CLOCK INFORMATION **************\n");
    io_printf(IO_STD, "Reading from sark variables...\n");
    io_printf(IO_STD, "Clock divisors for system & router bus: %u\n", sv->clk_div);
    io_printf(IO_STD, "CPU clock in MHz         : %u\n", sv->cpu_clk);
    io_printf(IO_STD, "SDRAM clock in MHz       : %u\n\n", sv->mem_clk);

    io_printf(IO_STD, "Reading registers directly...\n");
    io_printf(IO_STD, "PLL-1 (r20)\n");
    io_printf(IO_STD, "----------------------------\n");
    io_printf(IO_STD, "Frequency range          : "); get_FR_str(FR1);
    io_printf(IO_STD, "Output clk divider (MS)  : %u\n", MS1);
    io_printf(IO_STD, "Input clk multiplier (NS): %u\n\n", NS1);

    io_printf(IO_STD, "PLL-2 (r21)\n");
    io_printf(IO_STD, "----------------------------\n");
    io_printf(IO_STD, "Frequency range          : "); get_FR_str(FR2);
    io_printf(IO_STD, "Output clk divider (MS)  : %u\n", MS2);
    io_printf(IO_STD, "Input clk multiplier (NS): %u\n\n", NS2);

    io_printf(IO_STD, "Multiplexer (r24)\n");
    io_printf(IO_STD, "----------------------------\n");
    io_printf(IO_STD, "System AHB clk divisor   : %u\n", Sdiv);
    io_printf(IO_STD, "System AHB clk selector  : %u (%s)\n", Sys_sel, selName(Sys_sel));
    io_printf(IO_STD, "System AHB clk freq      : %k MHz\n", Sfreq);
    io_printf(IO_STD, "Router clk divisor       : %u\n", Rdiv);
    io_printf(IO_STD, "Router clk selector      : %u (%s)\n", Rtr_sel, selName(Rtr_sel));
    io_printf(IO_STD, "Router clk freq          : %k MHz\n", Rfreq);
    io_printf(IO_STD, "SDRAM clk divisor        : %u\n", Mdiv);
    io_printf(IO_STD, "SDRAM clk selector       : %u (%s)\n", Mem_sel, selName(Mem_sel));
    io_printf(IO_STD, "SDRAM clk freq           : %k MHz\n", Mfreq);
    io_printf(IO_STD, "CPU-B clk divisor        : %u\n", Bdiv);
    io_printf(IO_STD, "CPU-B clk selector       : %u (%s)\n", Pb, selName(Pb));
    io_printf(IO_STD, "CPU-B clk freq           : %k MHz\n", Bfreq);
    io_printf(IO_STD, "CPU-A clk divisor        : %u\n", Adiv);
    io_printf(IO_STD, "CPU-A clk selector       : %u (%s)\n", Pa, selName(Pa));
    io_printf(IO_STD, "CPU-A clk freq           : %k MHz\n", Afreq);
    io_printf(IO_STD, "**********************************************\n\n");
}
