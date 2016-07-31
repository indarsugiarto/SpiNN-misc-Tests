#include <spin1_api.h>

#define NUM_OF_STREAM	1000000
//#define DEF_DEL_VAL	1000 // will result in 5.2MBps
//#define DEF_DEL_VAL	750  // can go up to 6.4MBps but there was packet lost and bigger SWC error...
//#define DEF_DEL_VAL	850  // can go with 5.9MBps with a small packet loss and small SWC error...
//#define DEF_DEL_VAL	800  // can go with 6.1MBps but with many packet lost and big SWC error

#define DEF_DEL_VAL 	900  // perfect, up to 5.7MBps

// Using qt/c++ to receive packets:
// still, got missing packet and SWC error...
//#define DEF_DEL_VAL	800

/*
 * HASIL:
 *   Sepertinya tidak bisa digunakan untuk multi-core streaming. Karena:
 *   - menghasilkan SWC error yang besar
 *   - kecepatan maximum tidak bertambah (misal, tetap 5.9MBps untuk 2-core
 *     dengan DEF_DEL_VAL 850)
 */

sdp_msg_t myMsg;
uint myCore;
ushort pktCntr = 1;

volatile uint giveDelay(uint delVal)
{
  volatile uint dummy = delVal;
  uint step = 0;
  while(step < delVal) {
    dummy += (2 * step);
    step++;
  }
  return dummy;
}

void c_main ()
{
  myCore = sark_core_id();
  sark_delay_us(myCore*10);

  io_printf(IO_STD, "\n\nTest bursting %d sdp packet to host...\n", NUM_OF_STREAM);
  sark_delay_us(1000000);

  // init myMsg
  uint i;
  myMsg.flags = 0x07;	// without reply
  myMsg.tag = 1;	// send internally, no need for iptag
  myMsg.dest_port = PORT_ETH;	// send to core-2 on port-2
  myMsg.dest_addr = sv->eth_addr;
  myMsg.srce_port = (1 << 5) + myCore;	// send from core-1 on port-1
  myMsg.srce_addr = sv->p2p_addr;
  myMsg.cmd_rc = myCore;
  myMsg.seq = myCore;
  myMsg.length = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t) + 256;
  for (i=0; i<256; i++)
    myMsg.data[i] = i;

  // then stream it
  uint delVal = 0;
  for(i=0; i<NUM_OF_STREAM; i++) {
    spin1_send_sdp_msg(&myMsg, 10);
    delVal += giveDelay(DEF_DEL_VAL);
  }
  // finally send EOF packet
  myMsg.length = sizeof(sdp_hdr_t);
  for(i=0; i<10; i++)
    spin1_send_sdp_msg(&myMsg, 10);
  io_printf(IO_STD, "done with delVal = %u\n\n", delVal);
}

