#include <spin1_api.h>

#define NUM_OF_STREAM	100000
//#define DEF_DEL_VAL	1000 // will result in 5.2MBps
//#define DEF_DEL_VAL	750  // can go up to 6.4MBps but there was packet lost and bigger SWC error...
//#define DEF_DEL_VAL	850  // can go with 5.9MBps with a small packet loss and small SWC error...
//#define DEF_DEL_VAL	800  // can go with 6.1MBps but with many packet lost and big SWC error


//#define DEF_DEL_VAL 	900  // perfect, up to 5.7MBps

// Let's try for other chip:
#define DEF_DEL_VAL		6000	// OK, no SWC error, 1.1MBps
//#define DEF_DEL_VAL		5500	// OK, small SWC, 1.2MBps
//#define DEF_DEL_VAL		5000	// OK, small SWC, 1.3MBps
//#define DEF_DEL_VAL		4000	// OK, with medium-size SWC error (~1000), 1.6MBps
//#define DEF_DEL_VAL 	3500  // if previously has big SWC, it can be used up 940KiBps/s, but if newly started, then--> error: error RC_P2P_NOREPLY
//#define DEF_DEL_VAL 	2500  // if previously has big SWC, it can be used up 1.2MBps/s, but if newly started, then--> error: error RC_P2P_NOREPLY
//#define DEF_DEL_VAL 	1500  // if previously has big SWC, it can be used up 1.3MBps/s, but if newly started, then--> error: error RC_P2P_NOREPLY
//#define DEF_DEL_VAL 	1000  // if previously has big SWC, it can be used up 1.3MBps/s, but if newly started, then--> error: error RC_P2P_NOREPLY



// Using qt/c++ to receive packets:
// still, got missing packet and SWC error...
//#define DEF_DEL_VAL	800

/*
 * NOTE on Running with several cores:
 *   - it will produce high SWC error
 *   - the speed reported by System Monitor is the same as the speed in 1-core scenario
 *   - Hence, we CANNOT use multicore streaming to achieve higher throughput
 */

uint myCore;
ushort pktCntr = 0;
uint fr_dest = 1 << 8; // goes to core-2
uchar core2_triggered = FALSE;

uchar buf[272];

void terminate(uint arg0, uint arg1)
{
    io_printf(IO_STD, "Received -%d packets!\n", pktCntr);
    spin1_exit(0);
}

void hSDP(uint mBox, uint port)
{
    sdp_msg_t *msg = (sdp_msg_t *)mBox;
    if(port==7)
        spin1_schedule_callback(terminate, 0, 0, 1);
    else {
        pktCntr++;

        // then tell core-2 via FR to start its own thread
        if(core2_triggered==FALSE) {
            spin1_send_fr_packet(0x12345678, 0, NO_PAYLOAD);
            core2_triggered=TRUE;
        }

        // then copy the content
        sark_mem_cpy((void *)buf, (void *)&msg->cmd_rc, 272);
    }

    spin1_msg_free(msg);
}

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

void hFR(uint key, uint payload)
{
  sdp_msg_t myMsg;

  uint i;
  myMsg.flags = 0x07;	// without reply
  myMsg.tag = 1;	
  myMsg.dest_port = PORT_ETH;	// send to core-2 on port-2
  myMsg.dest_addr = sv->eth_addr;
  myMsg.srce_port = (1 << 5) + myCore;	// send from core-2 on port-1
  myMsg.srce_addr = sv->p2p_addr;
  myMsg.cmd_rc = myCore;
  myMsg.seq = myCore;
  myMsg.length = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t) + 256;
  for (i=0; i<256; i++)
    myMsg.data[i] = i;

  // then stream it
  // NOTE: sark_msg_send return 1 if successful, 0 for failure
  volatile uint delVal = 0;
  uint retVal, errCntr = 0; 
  for(i=0; i<NUM_OF_STREAM; i++) {
    do {
      retVal = sark_msg_send(&myMsg, 10);
      if(retVal==0) errCntr++;
    } while(retVal==0);
    delVal += giveDelay(DEF_DEL_VAL);
  }
  // finally send EOF packet
  myMsg.length = sizeof(sdp_hdr_t);
  for(i=0; i<10; i++)
    spin1_send_sdp_msg(&myMsg, 10);
  sark_delay_us(10000);
  io_printf(IO_STD, "Core-2 done with errCntr = %u\n\n", errCntr);

}

void c_main ()
{
  myCore = sark_core_id();
  rtr_fr_set(fr_dest);

  io_printf(IO_STD, "\n\nTest sdp round trip from/to host (without DEF_DEL_VAL). Make sure the iptag-1 is set!!!\n");

  if(myCore==1) {
    spin1_callback_on(SDP_PACKET_RX, hSDP, -1);
  }
  else {
    spin1_callback_on(FR_PACKET_RECEIVED, hFR, -1);
  }
  spin1_start(SYNC_NOWAIT);
}

