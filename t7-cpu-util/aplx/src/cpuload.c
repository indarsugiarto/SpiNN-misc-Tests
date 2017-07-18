#include <sark.h>
#include <spin1_api.h>

uchar running_cpu_idle_cntr[18];
uchar  stored_cpu_idle_cntr[18];
static uint idle_cntr_cntr = 0; //master counter that count up to 100

extern INT_HANDLE hSlowTimer (void);

// print_cntr just for debugging
// will be called/scheduled by hSoftInt
void print_cntr(uint null, uint nill)
{
  for(int i=0; i<18; i++) {
    io_printf(IO_STD,"phys_cpu-%d = %d\n", i, stored_cpu_idle_cntr[i]);
  }
  io_printf(IO_STD,"\n\n");
}

void c_main ()
{
  io_printf (IO_STD, "Try cpu load counter\n");

  sark_vic_set (SLOT_0, SLOW_CLK_INT, 1, hSlowTimer);

  cpu_sleep ();
}
