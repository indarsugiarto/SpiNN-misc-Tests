#include "profiler.h"

void init_app()
{
  // initialize profiler ID, or create a list for it
  generateProfilerID(); // see profiler_util.c

  // initialize cpu load counter
  init_idle_cntr(); // see profiler_cpuload.cp

  // initialize router
  init_Router();    // see profiler_events.c

  // initialize event handler
  init_Handlers();  // see profiler_events.c

  // others:
  streaming = FALSE;    // by default we silent
  // put version to vcpu->user0 to be detected by host GUI
  sark.vcpu->user0 = PROFILER_VERSION;
}



void c_main ()
{
  sanityCheck();        // Since some features require strict conditions
  init_app();
  if(sv->p2p_addr==0)
	io_printf(IO_STD, "Profiler-%d is ready!\n", my_pID);
  else
	io_printf(IO_BUF, "Profiler-%d is ready!\n", my_pID);
  spin1_start(SYNC_NOWAIT);
}

