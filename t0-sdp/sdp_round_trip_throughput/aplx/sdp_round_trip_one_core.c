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

        ushort tmpSrcA = msg->srce_addr;
        uchar tmpSrcP = msg->srce_port;
        msg->tag = 1;
        msg->srce_addr = msg->dest_addr;
        msg->srce_port = msg->dest_port;
        msg->dest_addr = tmpSrcA;
        msg->dest_port = tmpSrcP;

        spin1_send_sdp_msg(msg, 10);
    }

    spin1_msg_free(msg);
}

void c_main ()
{
  myCore = sark_core_id();

  io_printf(IO_STD, "\n\nTest sdp round trip from/to host (without DEF_DEL_VAL). Make sure the iptag-1 is set!!!\n");

  spin1_callback_on(SDP_PACKET_RX, hSDP, -1);

  spin1_start(SYNC_NOWAIT);
}

