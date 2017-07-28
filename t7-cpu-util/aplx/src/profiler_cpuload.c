/* The idea for cpu utilization using slow timer:
 * - Slow timer runs at about 32kHz. Whenever it fires, it detects
 *   the sleep state of each cores.
 * - After 100 times counting, it generates software interrupt.
 * - This 100 limit equivalent to 100%. Hence, if a core counter has 
 *   the value of 100, it means that core is 100% utilized during the
 *   sampling period of 1000000/32000 us == 3.124ms
 */
//#include <sark.h>
#include "profiler.h"

// the following two variables are just for debugging
uint myTick = 0;
uint tick_cnt = 0;

INT_HANDLER hSlowTimer (void)
{
  uint r25, i;

  // found problem here: solution &=, not =
  dma[DMA_GCTL] &= 0x7FFFFFFF; // clear bit[31] of dma GCTL

  //try this:
  vic[VIC_SOFT_CLR] = 1 << SLOW_CLK_INT;

  if(tick_cnt < 100) {
	  if(++myTick >= 10000) {
		//if run too long, it make RTE:
		io_printf(IO_BUF, "tick-%d, nAct=%d, temp=%d, fcpu=%dMHz\n",
				  tick_cnt++, myProfile.nActive, myProfile.temp3, myProfile.cpu_freq);
		myTick = 0;
	  }
  }

  if(++idle_cntr_cntr>100) {
	// copy to stored_cpu_idle_cntr
	// sark_mem_cpy(stored_cpu_idle_cntr, running_cpu_idle_cntr, 18);
	// clear running_cpu_idle_cntr and idle_cntr_cntr
	for(int i=0; i<18; i++) {
	  stored_cpu_idle_cntr[i] = 100 - running_cpu_idle_cntr[i];
	  running_cpu_idle_cntr[i] = 0;
	}
	idle_cntr_cntr = 0;
	// trigger software interrupt for the main profiler loop
	// the software interrupt is handled via user event
	spin1_trigger_user_event(0, 0);
	//spin1_schedule_callback(collectData, 0, 0, SCHEDULED_PRIORITY_VAL);
  }
  else {
	// let's split the work:
	// when counter = 94, read temperature
	if(idle_cntr_cntr==94) {
	  myProfile.temp3 = readTemp();
	  myProfile.temp1 = tempVal[0];
	}

	// when counter = 96, read frequency
	// by calling readFreq, all parameters in pll are read (eg. Sfreq, Sdiv, etc.)
	if(idle_cntr_cntr==96)
		myProfile.cpu_freq = readFreq(&myProfile.ahb_freq, &myProfile.rtr_freq);

	// when counter = 98, read nActiveCores
	if(idle_cntr_cntr==98)
	  myProfile.nActive = getNumActiveCores();

	// detect idle state of all cores
	r25 = sc[SC_SLEEP];
	for(i=0; i<18; i++)
		running_cpu_idle_cntr[i] += (r25 >> i) & 1;
  }


  vic[VIC_VADDR] = (uint) vic;			// Tell VIC we're done
}

/* hSoftInt() is triggered when idle_cntr_cntr in hSlowTimer()
 * reaches 100. This signify ends of sampling period.
 * profiler should response this to collect the cpu utilization
 * value.
 * Not used, because we use user event.
 */
INT_HANDLER hSoftInt (void)
{
  print_cntr(0,0);
  //spin1_schedule_callback(print_cntr, 0, 0, 1);
  vic[VIC_VADDR] = (uint) vic;
}


// print_cntr just for debugging
// will be called/scheduled by hSoftInt
void print_cntr(uint null, uint nill)
{
  //for(int i=0; i<18; i++) {
    //io_printf(IO_STD,"phys_cpu-%d = %d\n", i, stored_cpu_idle_cntr[i]);
    io_printf(IO_STD, "%03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d %03d\n"
                    , stored_cpu_idle_cntr[0]
                    , stored_cpu_idle_cntr[1]
                    , stored_cpu_idle_cntr[2]
                    , stored_cpu_idle_cntr[3]
                    , stored_cpu_idle_cntr[4]
                    , stored_cpu_idle_cntr[5]
                    , stored_cpu_idle_cntr[6]
                    , stored_cpu_idle_cntr[7]
                    , stored_cpu_idle_cntr[8]
                    , stored_cpu_idle_cntr[9]
                    , stored_cpu_idle_cntr[10]
                    , stored_cpu_idle_cntr[11]
                    , stored_cpu_idle_cntr[12]
                    , stored_cpu_idle_cntr[13]
                    , stored_cpu_idle_cntr[14]
                    , stored_cpu_idle_cntr[15]
                    , stored_cpu_idle_cntr[16]
                    , stored_cpu_idle_cntr[17]);
  //}
  //io_printf(IO_STD,"\n\n");
}

void init_idle_cntr()
{
	for(uint i=0; i<18; i++) {
		running_cpu_idle_cntr[i] = 0;
		stored_cpu_idle_cntr[i] = 0;
	}
	idle_cntr_cntr = 0; //master counter that count up to 100
	//sark_vic_set (SLOT_FIQ, SLOW_CLK_INT, 1, hSlowTimer);
	//sark_vic_set (SLOT_10, SLOW_CLK_INT, 1, hSlowTimer);

	if(vic[VIC_SELECT]) {
#if(DEBUG_LEVEL>2)
		io_printf(IO_BUF, "VIC_SELECT = 0x%x\n", vic[VIC_SELECT]);
		io_printf(IO_BUF, "Using IRQ for idle-cntr\n");
#endif
		sark_vic_set (SLOT_10, SLOW_CLK_INT, 0, hSlowTimer); // don't enable it yet? wait for synchronization
	}
	else {
#if(DEBUG_LEVEL>2)
		io_printf(IO_BUF, "Using FIQ for idle-cntr\n");
#endif
		sark_vic_set (SLOT_FIQ, SLOW_CLK_INT, 0, hSlowTimer);
	}

}

// startProfiling is scheduled in the c_main and waits synchronization signal
void startProfiling(uint null, uint nill)
{
	vic[VIC_ENABLE] = 1 << SLOW_CLK_INT;
}
