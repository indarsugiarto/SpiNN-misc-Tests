#include <spin1_api.h>
#include <stdfix.h>

// will assume sending from chip <0,0> to <1,0> 
// will assume core-1 in both chips are used

#define REAL		accum
#define REAL_CONST(x)	x##k

#define MAX_TRANSMIT	1000000
#define TIMER_PERIOD	1000000		// let's start after 1 second

// Enable timer - free running, 32-bit
#define ENABLE_TIMER() tc[T2_CONTROL] = 0x82

// To measure, set timer to 0
#define START_TIMER() tc[T2_LOAD] = 0

// Read timer and compute time (microseconds)
#define READ_TIMER() (0 - tc[T2_COUNT])

sdp_msg_t pktSDP;
uint t1, t2;

void doStreaming()
{
	uint i;
	// phase-1: streaming MCPL
	START_TIMER();
	for(i=0; i<MAX_TRANSMIT; i++)
		spin1_send_mc_packet(0x12345678, i, WITH_PAYLOAD);
	t1 = READ_TIMER();

	// phase-2: streaming SDP
	START_TIMER();
	for(i=0; i<MAX_TRANSMIT; i++)
		spin1_send_sdp_msg(&pktSDP, 10);
	t2 = READ_TIMER();

	// finally, print report and exit
	io_printf(IO_STD, "t1 = %u, t2 = %u\n", t1, t2);
	spin1_exit(SUCCESS);
}

void hTimer(uint tick, uint None)
{
	if(tick==1) {
		spin1_callback_off(TIMER_TICK);
		doStreaming();
	}
}

void hSDP(uint mbox, uint port)
{
	sdp_msg_t *msg = (sdp_msg_t *) mbox;

	spin1_msg_free (msg);
}

void hMCPL(uint key, uint payload)
{

}

void init()
{
	uint e = rtr_alloc(1);
	if(sv->p2p_addr==0) {
		rtr_mc_set(e, 0x12345678, 0xFFFFFFFF, 1); // from <0,0> to <1,0>
		pktSDP.flags = 7;
		pktSDP.tag = 0;
		pktSDP.srce_port = 0x21;	// port-1, core-1
		pktSDP.srce_addr = 0;
		pktSDP.dest_port = 0x21;	// port-1, core-1
		pktSDP.dest_addr = (1 << 8);	// to chip<1,0>
		pktSDP.cmd_rc = 0x1234;
		pktSDP.seq = 0x5678;
		for(uint i=0; i<256; i++)
			pktSDP.data[i] = i;
		pktSDP.length = sizeof(sdp_hdr_t)+sizeof(cmd_hdr_t)+256;
	        spin1_set_timer_tick(TIMER_PERIOD);
		spin1_callback_on(TIMER_TICK, hTimer, 1);
		io_printf(IO_STD, "Will send %d MCPL and SDP data...\n");
	}
	else {
		rtr_mc_set(e, 0x12345678, 0xFFFFFFFF, (1 << 7)); 
	}
	ENABLE_TIMER();
}

void c_main(void)
{
	init();
	spin1_callback_on(SDP_PACKET_RX, hSDP, 0);
	spin1_callback_on(MCPL_PACKET_RECEIVED, hMCPL, -1);
	spin1_start(SYNC_NOWAIT);
}


/* This works:

	uint i = 25;
	float f = (float)i / 2.0;
	REAL k = f;
	REAL l = 11.5;
	io_printf(IO_STD, "k = %k, l = %k\n", k, l);
	return;
 * */

/* This doesn't work:

	float f = 123.456; io_printf(IO_STD, "f = %f\n", f); return;
 * */
