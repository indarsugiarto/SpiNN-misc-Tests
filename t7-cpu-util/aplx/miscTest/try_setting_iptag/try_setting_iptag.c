
#include <spin1_api.h>

// IPTAG definitions from scamp:
#define IPTAG_NEW		0
#define IPTAG_SET		1
#define IPTAG_GET		2
#define IPTAG_CLR		3
#define IPTAG_TTO		4

sdp_msg_t iptag;

void hSDP(uint mBox, uint port)
{
     sdp_msg_t *msg = (sdp_msg_t *) mBox;
     io_printf(IO_STD, "Got sdp:\n");
     io_printf(IO_STD, "cmd_rc = 0x%x\n", msg->cmd_rc);
     io_printf(IO_STD, "seq    = 0x%x\n", msg->seq);
     io_printf(IO_STD, "arg1   = 0x%x\n", msg->arg1);
     io_printf(IO_STD, "arg2   = 0x%x\n", msg->arg2);
     io_printf(IO_STD, "arg3   = 0x%x\n", msg->arg3);
     spin1_msg_free (msg);
}

void initiate(uint arg1, uint arg2)
{
	io_printf(IO_STD, "Reading iptag is initiated!\n");
	iptag.flags = 0x87;	// no replay
	iptag.tag = 0;		// internal
	iptag.srce_addr = 0;
	iptag.srce_port = 0xE0 + (uchar)sark_core_id();	// use port-7
	iptag.dest_addr = 0;
	iptag.dest_port = 0;				// send to "root"
	iptag.cmd_rc = CMD_IPTAG;
	uint op = IPTAG_GET << 16;
        iptag.arg1 = op;
        iptag.length = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t);
	spin1_send_sdp_msg(&iptag, 10);
}

void setIPtag()
{
	iptag.flags = 7;
	iptag.tag = 0;
	iptag.srce_addr = 0;
	iptag.srce_port = 0xE0 + (uchar)sark_core_id();
	iptag.dest_addr = 0; iptag.dest_port = 0;
	iptag.cmd_rc = CMD_IPTAG;
	iptag.arg1 = (1 << 16) + 3;
	iptag.arg2 = 12345;
	iptag.arg3 = 0;
	iptag.length= sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t);
	spin1_send_sdp_msg(&iptag, 10);
}

void c_main (void)
{
	//spin1_callback_on(SDP_PACKET_RX, hSDP, 0);
	//spin1_schedule_callback(initiate, 0, 0, 1);
	setIPtag();
	//spin1_start(SYNC_NOWAIT);
}
