#include <sark.h>

volatile uint tst = 0;

void c_main (void)
{
  io_printf (IO_STD, "Start!\n");

  for(uint i=0; i<0xFFFFFFF; i++)
    tst = i*2;
  // io_printf can also do sprintf!

  io_printf (IO_STD, "Stop\n");
}
