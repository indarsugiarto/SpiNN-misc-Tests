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


volatile float fResult = 0.0;
volatile REAL kResult = 0.0;
volatile float N1, N2, N3, N4;
volatile REAL K1, K2, K3, K4;
int i;

float freq = 200.0;

float fCompute()
{
  for(i=0; i<MAX_ITER; i++) {
    fResult += N1;
    fResult -= N2;
    fResult *= N3;
    fResult /= N4;
    fResult += N2;
    fResult -= N1;
    fResult *= N4;
    fResult /= N3;
  }
  return fResult;
}

REAL sCompute()
{
  for(i=0; i<MAX_ITER; i++) {
    kResult += K1;
    kResult -= K2;
    kResult *= K3;
    kResult /= K4;
    kResult += K2;
    kResult -= K1;
    kResult *= K4;
    kResult /= K3;
  }
  return kResult;
}


void c_main ()
{
  N1 = 3.19; K1 = N1;
  N2 = 2.91; K2 = N2;
  N3 = 2.01; K3 = N3;
  N4 = 1.97; K4 = N4;

  uint t1_clk, t2_clk;
  float t1_us, t2_us;
  float mopus1, mopus2; // million operation per microsecond

  ENABLE_TIMER();

  io_printf(IO_STD, "\n\nRunning floating point operation %u-times\n", MAX_ITER);

  START_TIMER();
  fCompute();
  t1_clk = READ_TIMER();
  t1_us = ((float)t1_clk / freq);
  mopus1 = 8 / t1_us;
  io_printf(IO_STD, "MFLOPS = %5.2k\n", (REAL)mopus1);

  io_printf(IO_STD, "\n\nRunning fixed point operation %u-times\n", MAX_ITER);

  START_TIMER();
  sCompute();
  t2_clk = READ_TIMER();
  t2_us = ((float)t2_clk / freq);
  mopus2 = 8 / t2_us;
  io_printf(IO_STD, "MFLOPS = %5.2k\n", (REAL)mopus2);
}

