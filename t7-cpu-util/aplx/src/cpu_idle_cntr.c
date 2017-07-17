
#include <sark.h>

uint ticks;
uint tk;

INT_HANDLER hSlowTimer (void)
{
  dma[DMA_GCTL] = 0x7FFFFFFF; // clear bit[31] of dma GCTL

  if(ticks>1000) {
	  io_printf (IO_STD, "tk - %d\n", ++tk);
	  ticks = 0;
  } else {
	  ticks++;
  }
  //io_printf (IO_BUF, "Tick %d\n", ++ticks);	// somehow, it stops (RTE) at tick 1749

  vic[VIC_VADDR] = (uint) vic;			// Tell VIC we're done
}

void c_main ()
{
  io_printf (IO_STD, "Try slow timer\n");

  sark_vic_set (SLOT_0, SLOW_CLK_INT, 1, hSlowTimer);

  cpu_sleep ();
}
