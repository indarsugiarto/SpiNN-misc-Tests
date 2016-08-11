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
volatile uint iResult = 0;
volatile float F1, F2, F3, F4;
volatile REAL K1, K2, K3, K4;
volatile uint I1, I2, I3, I4;
int i;

float freq = 200.0;

// emulated float operation
float fCompute()
{
  for(i=0; i<MAX_ITER; i++) {
    fResult += F1;
    fResult -= F2;
    fResult *= F3;
    fResult /= F4;
    fResult += F2;
    fResult -= F1;
    fResult *= F4;
    fResult /= F3;
  }
  return fResult;
}

// standard fixed point operation
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

// integer operation
uint iCompute()
{
  for(i=0; i<MAX_ITER; i++) {
    iResult += I1; 
    iResult -= I2; 
    iResult *= I3; 
    iResult /= I4; 
    iResult += I4; 
    iResult -= I3; 
    iResult *= I1; 
    iResult /= I2; 
  }
  return iResult;
}


void c_main ()
{
  F1 = 3.19; K1 = F1; I1 = 4;
  F2 = 2.91; K2 = F2; I2 = 3;
  F3 = 2.01; K3 = F3; I3 = 2;
  F4 = 1.97; K4 = F4; I4 = 1;

  uint t1_clk, t2_clk, t3_clk;
  float t1_us, t2_us, t1_s, t2_s;
  float mopus1, mopus2; // million operation per microsecond

  float fResult;
  REAL kResult;
  uint iResult;

  ENABLE_TIMER();

  /*--------------------- Floating point ------------------*/
  io_printf(IO_STD, "\n\nRunning %u floating point operation:\n", MAX_ITER*8);

  START_TIMER();
  fResult = fCompute();
  t1_clk = READ_TIMER();
  t1_us = ((float)t1_clk / freq);
  mopus1 = t1_us;
  mopus1 = (MAX_ITER*8*1000000)/t1_us;
  //io_printf(IO_STD, "t1_clk = %u, MFLOPS = %5.2k\n", t1_clk, (REAL)mopus1);
  io_printf(IO_STD, "t1_clk = %u, with final value = %k\n", t1_clk, (REAL)fResult);



  sark_delay_us(1000000);





  /*---------------------- Fixed point -------------------*/
  io_printf(IO_STD, "\n\nRunning %u fixed point operation:\n", MAX_ITER*8);

  START_TIMER();
  kResult = sCompute();
  t2_clk = READ_TIMER();
  t2_us = ((float)t2_clk / freq);
  mopus2 = t2_us;
  //io_printf(IO_STD, "t2_clk = %u, MFLOPS = %5.2k\n", t2_clk, (REAL)mopus2);
  io_printf(IO_STD, "t2_clk = %u, with final value = %k\n", t2_clk, kResult);



  sark_delay_us(1000000);





  /*----------------------- integer  ---------------------*/
  io_printf(IO_STD, "\n\nRunning %u integer operation:\n", MAX_ITER*8);

  START_TIMER();
  iResult = iCompute();
  t3_clk = READ_TIMER();
  io_printf(IO_STD, "t3_clk = %u, with final value = %u\n", t3_clk, iResult);
}

