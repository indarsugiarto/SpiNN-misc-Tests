#include <spin1_api.h>
#include <stdfix.h>

#define REAL	accum

#define MAX_ITER 1000000

// Use "timer2" to measure elapsed time.
// Times up to around 10 sec should be OK.


// Enable timer - free running, 32-bit
#define ENABLE_TIMER() tc[T2_CONTROL] = 0x82

// To measure, set timer to 0
#define START_TIMER() tc[T2_LOAD] = 0

// Read timer and compute time (microseconds)
#define READ_TIMER() (0 - tc[T2_COUNT])


volatile REAL kResult = 0.0;
volatile REAL K1, K2, K3, K4;
volatile REAL opCntr = 0.0;

REAL freq = 200.0;

uint t1_clk;

uint run = 1;

// emulated REAL operation
void fCompute(uint arg0, uint arg1)
{
  while(run) {
    kResult += K1; opCntr += 0.002;
    kResult -= K2; opCntr += 0.002;
    kResult *= K3; opCntr += 0.002;
    kResult /= K4; opCntr += 0.002;
    kResult += K2; opCntr += 0.002;
    kResult -= K1; opCntr += 0.002;
    kResult *= K4; opCntr += 0.002;
    kResult /= K3; opCntr += 0.002;
  }
}

void report()
{
  REAL final = kResult;
  //opCntr *= 1000.0;
  //uint cntr = (uint)opCntr;
  io_printf(IO_STD, "Total operation = %6.3k*10^3, tclk = %u, final result = %k\n", opCntr, t1_clk, final);
}

void hTimer(uint tick, uint None)
{
    // io_printf(IO_STD, "Tick-%d\n",tick);

	if(tick==1)
        START_TIMER();
    else if(tick==2) {
        t1_clk = READ_TIMER();
        run = 0;
    }
    else {
        spin1_callback_off(TIMER_TICK);
        report();
        spin1_exit (0);
    }
}

void c_main ()
{
  K1 = 3.19; 
  K2 = 2.91; 
  K3 = 2.01; 
  K4 = 1.97; 

  ENABLE_TIMER();

  io_printf(IO_STD, "Running only stdfix operation!\n");
  spin1_set_timer_tick (1000000);
  spin1_callback_on(TIMER_TICK, hTimer, -1);
  spin1_schedule_callback (fCompute, 0, 0, 1);

  spin1_start (SYNC_NOWAIT);

  report();
}

