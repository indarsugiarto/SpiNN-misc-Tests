#include <spin1_api.h>

// If handshaking mechanism is used, make sure tag-1 is reserved!
#define WITH_HANDSHAKING	FALSE

#define SDP_PRIORITY            0

uchar buf[272];	// 16 for SCP and 256 for data segment
uint normalCntr = 0;
uint wrongCntr = 0;
ushort stopLength = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t);
ushort pktLength[2];
// for use with handshaking:
sdp_msg_t replyMsg;

void hSDP(uint mBox, uint port)
{
    sdp_msg_t *msg = (sdp_msg_t *)mBox;
    if(msg->length == stopLength) {
        spin1_callback_off(SDP_PACKET_RX);
        io_printf(IO_STD, "Total received normal packets with length %d = %u\n", pktLength[0], normalCntr);
        io_printf(IO_STD, "Total received wrong packets with length %d = %u\n", pktLength[1], wrongCntr);
        io_printf(IO_STD, "Now terminate\n");
        spin1_exit(0);
    } else {
        ushort no_hdr = msg->length-8;	// remove the sdp-header
        
        if(no_hdr == 272) {
            pktLength[0] = no_hdr;
            normalCntr++;
        }
        else {
            pktLength[1] = no_hdr;
            wrongCntr++;
        }
        sark_mem_cpy((void *)buf, (void *)&msg->cmd_rc, 272);
#if (WITH_HANDSHAKING==TRUE)
        sark_msg_send(&replyMsg, 10);
#endif
    }
    spin1_msg_free(msg);
}

void hDMA(uint tid, uint tag)
{

}

void c_main(void)
{
#if (WITH_HANDSHAKING==TRUE)
    uint coreID = sark_core_id();
    // prepare reply
    replyMsg.flags = 0x07;  //no reply
    replyMsg.tag = 1;
    replyMsg.srce_port = (1 << 5) + coreID;
    replyMsg.srce_addr = sv->p2p_addr;
    replyMsg.dest_port = PORT_ETH;
    replyMsg.dest_addr = sv->eth_addr;
    replyMsg.length = sizeof(sdp_hdr_t);    // it's fix at smallest size!
#endif

    io_printf(IO_STD, "Test SDP throughput. Stream me the sdp please!\n");   

    spin1_callback_on(SDP_PACKET_RX, hSDP, SDP_PRIORITY);

    spin1_start(SYNC_NOWAIT);
}

