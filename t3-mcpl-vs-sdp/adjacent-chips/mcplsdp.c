#include <spin1_api.h>
#include <stdfix.h>

// will assume sending from chip <0,0> to <1,0> 
// will assume core-1 in both chips are used

#define REAL		accum
#define REAL_CONST(x)	x##k

#define MAX_SDP_TRANSMIT	10000
#define MAX_MCPL_TRANSMIT	MAX_SDP_TRANSMIT * 68
#define TIMER_PERIOD	1000000		// let's start after 1 second

// Enable timer - free running, 32-bit
#define ENABLE_TIMER() tc[T2_CONTROL] = 0x82

// To measure, set timer to 0
#define START_TIMER() tc[T2_LOAD] = 0

// Read timer and compute time (microseconds)
#define READ_TIMER() (0 - tc[T2_COUNT])

//#define DEBUG_MCPL
//#define DEBUG_SDP

sdp_msg_t pktSDP;
uint t1, t2;

volatile uchar cont = 1;
volatile uchar finish = 0;
uchar t1_rec = 0;
uchar t2_rec = 0;
volatile uint mcplCntr = 0;
volatile uint dummy;
uchar data[272];

void doStreaming()
{
	uint i;
	// phase-1: streaming MCPL
	io_printf(IO_STD, "[Sender] Starting phase-1......\n");
	START_TIMER();
	i=0;
	do{
		if(cont==1) {
			cont = 0;
			spin1_send_mc_packet(0x12345678, i, WITH_PAYLOAD);
#ifdef DEBUG_MCPL
			io_printf(IO_STD, "[Sender] MCPL-%d has been sent!\n", i);
#endif
			i++;
		}
	} while (i<MAX_MCPL_TRANSMIT);
	t1 = READ_TIMER();

	// wait until the last reply is received
	io_printf(IO_STD, "[Sender] Wait for final ack...\n");
	while(cont==0) {
	}
	io_printf(IO_STD, "[Sender] Ack is received. Starting phase-2...\n");

	// phase-2: streaming SDP
	START_TIMER();
	i=0;
	do{
		if(cont==1) {
			cont=0;
			spin1_send_sdp_msg(&pktSDP, 10);
#ifdef DEBUG_SDP
			io_printf(IO_STD, "[Sender] SDP-%d has been sent!\n", i);
			sark_delay_us(1000);
#endif
			i++;
		}
	} while (i<MAX_SDP_TRANSMIT);
	t2 = READ_TIMER();

	io_printf(IO_STD, "[Sender] Wait for final ack...\n");
	while(cont==0) {
	}
	io_printf(IO_STD, "[Sender] Ack is received. Signal finish...\n");
	// then tell the destination that we've done!
	spin1_send_mc_packet(0x12345678, 0xFFFFFFFF, WITH_PAYLOAD);

	// finally, print report and exit
	io_printf(IO_STD, "[Sender] t1 = %u, t2 = %u clk\n", t1, t2);
	spin1_exit(SUCCESS);
}

void hTimer(uint tick, uint None)
{
	if(tick==1) {
		spin1_callback_off(TIMER_TICK);
		doStreaming();
	}
}

void checkFinish(uint arg0, uint arg1)
{
	if(finish==1) {
		t2 = READ_TIMER();
		io_printf(IO_STD, "[Recvr] t1 = %u, t2 = %u clk\n", t1, t2);
		float f1, f2;
		REAL k1, k2;
		f1 = (float)t1/200.0; f1 /= (MAX_SDP_TRANSMIT * sizeof(uint));
		f2 = (float)t2/200.0; f2 /= (MAX_SDP_TRANSMIT * sizeof(uint));
		k1 = f1; k2 = f2;
		io_printf(IO_STD, "[Processing-time] %k-us per byte for MCPL, %k-us per byte for SDP\n", 
							k1, k2);
		spin1_exit(SUCCESS);
	} else {
		spin1_schedule_callback(checkFinish, 0, 0, 2);
	}
}

void hSDP(uint mbox, uint port)
{
	sdp_msg_t *msg = (sdp_msg_t *) mbox;
#ifdef DEBUG_SDP
		io_printf(IO_STD, "[Recvr] Got the SDP with srce_port = 0x%x...\n", msg->srce_port);
#endif
	if(t2_rec==0) {
		t1 = READ_TIMER();
		t2_rec = 1;
		START_TIMER();
	}
	if(msg->srce_port == 0x21) {
#ifdef DEBUG_SDP
		io_printf(IO_STD, "[Recvr] Got the SDP...\n");
#endif
		sark_mem_cpy(data, (void *)&msg->cmd_rc, 272);
		spin1_send_mc_packet(0x87654321, 0, NO_PAYLOAD);
#ifdef DEBUG_MCPL
		io_printf(IO_STD, "[Recvr] Sending ack...\n");
#endif
	}
	spin1_msg_free (msg);
}

void hMCPL(uint key, uint payload)
{
	if(t1_rec==0) {
		START_TIMER(); 
		t1_rec=1;
	}
	if(key==0x12345678) {
		if(payload != 0xFFFFFFFF) {
#ifdef DEBUG_MCPL
			io_printf(IO_STD, "[Recvr] MCPL-%d is received!\n", payload);
#endif
			mcplCntr++;
			dummy = payload;
#ifdef DEBUG_MCPL
			io_printf(IO_STD, "[Recvr] Sending ack...\n");
#endif
			spin1_send_mc_packet(0x87654321, 0, NO_PAYLOAD);
		} 
		// finish the streaming
		else {
			finish = 1;
			spin1_schedule_callback(checkFinish, 0, 0, 2);
#ifdef DEBUG_MCPL
			io_printf(IO_STD, "[Recvr] Finish signal is received...\n");
#endif
		}
	}
	/*
	else if(key=0x87654321) {
		io_printf(IO_STD, "[Sender] Got the ack...\n");
		cont = 1;
	}
	*/
}

void hMC(uint key, uint None)
{
	if(key=0x87654321) {
#ifdef DEBUG_MCPL
		io_printf(IO_STD, "[Sender] Got the ack...\n");
#endif
		cont = 1;
	}
}

void init()
{
	uint e = rtr_alloc(2);
	if(sv->p2p_addr==0) {
		rtr_mc_set(e, 0x12345678, 0xFFFFFFFF, 1); e++;// from <0,0> to <1,0>
		rtr_mc_set(e, 0x87654321, 0xFFFFFFFF, (1 << 7));	// for replay pkt
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
		io_printf(IO_STD, "Will send %d MCPL and %d SDP data...\n", 
						MAX_MCPL_TRANSMIT, MAX_SDP_TRANSMIT);
	}
	else {
		rtr_mc_set(e, 0x12345678, 0xFFFFFFFF, (1 << 7)); e++;
		rtr_mc_set(e, 0x87654321, 0xFFFFFFFF, (1 << 3));
	}
	ENABLE_TIMER();
}

void c_main(void)
{
	init();
	spin1_callback_on(SDP_PACKET_RX, hSDP, 1);
	spin1_callback_on(MCPL_PACKET_RECEIVED, hMCPL, 0);
	spin1_callback_on(MC_PACKET_RECEIVED, hMC, -1);

	spin1_start(SYNC_NOWAIT);
}

