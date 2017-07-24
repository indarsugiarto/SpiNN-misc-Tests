#include "profiler.h"

uchar running_cpu_idle_cntr[18];
uchar  stored_cpu_idle_cntr[18];
uint idle_cntr_cntr = 0; //master counter that count up to 100

uint tick = 0;
uint tick_cnt = 0;

extern INT_HANDLER hSlowTimer (void);

void init_app()
{
  // initialize profiler ID, or create a list for it
  generateProfilerID();

  // initialize router

  // initialize counter
  for(int i=0; i<18; i++) {
    running_cpu_idle_cntr[i] = 0;
    stored_cpu_idle_cntr[i]  = 0;
  } 
  
}



void c_main ()
{
  init_app();
  sark_vic_set (SLOT_10, SLOW_CLK_INT, 1, hSlowTimer);
  spin1_callback_on(USER_EVENT, print_cntr, 1);
  if(sv->p2p_addr==0)
	io_printf(IO_STD, "Profiler-%d is ready!\n", my_pID);
  else
	io_printf(IO_BUF, "Profiler-%d is ready!\n", my_pID);
  spin1_start(SYNC_NOWAIT);
}

