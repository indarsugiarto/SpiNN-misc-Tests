#include "profiler.h"

/* The following is reminiscent of previous version:
#define lnIDTable					48
#define wdIDTable					2
static uchar profIDTable[lnIDTable][wdIDTable] = {
{0,0},{1,0},{2,0},{3,0},{4,0},
{0,1},{1,1},{2,1},{3,1},{4,1},{5,1},
{0,2},{1,2},{2,2},{3,2},{4,2},{5,2},{6,2},
{0,3},{1,3},{2,3},{3,3},{4,3},{5,3},{6,3},{7,3},
	  {1,4},{2,4},{3,4},{4,4},{5,4},{6,4},{7,4},
			{2,5},{3,5},{4,5},{5,5},{6,5},{7,5},
				  {3,6},{4,6},{5,6},{6,6},{7,6},
						{4,7},{5,7},{6,7},{7,7}};

*/


/*----------------------- Misc. Utilities ------------------------*/

/* Update:
 * generateProfilerID() now is based on rtr_p2p_get() from sark
 * Previously, it is defined using static profIDTable.
 * NOTE uint rtr_p2p_get (uint entry):
 *    Gets a P2P table entry. Returns a value in the range 0 to 7.
 *    \param entry the entry number in the range 0..65535
 *    \return table entry (range 0 to 7)
 *
 * The purpose of generageProfilerID is to identify orderly the
 * profiler chips. The process is:
 * 1. the root profiler will do generateProfilerID() and create a list
 *    of ID based on rtr_p2p_get() result.
 * 2. the root profiler will send the ID to the corresponding chip
 *    afterwards, each chip will report based on this ID
 * 3. the root profiler inform host-PC about this list so that the PC
 *    can interpret the incoming profiler data correctly.
*/

ushort my_pID;
ushort *pID_list;   // profiler ID list
ushort nChips;		// number of chips in the system

void generateProfilerID()
{
	uint r;
	uint dest;

	// only root needs complete list of ID
	if(sv->p2p_addr == 0) {
		// first, allocate ID-list buffer in sdram
		pID_list = sark_xalloc(sv->sdram_heap, 65536, 0, ALLOC_LOCK);

		my_pID = 0;
		// the following is a slow process
		nChips = 1; // the root profiler
		for(r=1; r<=0xFFFF; r++) {
			dest = rtr_p2p_get(r);
			if(r != 6) {
				pID_list[nChips] = r; // r can be extracted using CHIP_X() CHIP_Y()
				nChips++;
			}
		}

#if(DEBUG_LEVEL>0)
		io_printf(IO_BUF, "Found %d-chips in the system\n", nChips);
		ushort XY;
		for(ushort i=0; i<nChips; i++) {
			XY = pID_list[i];
			io_printf(IO_BUF, "ID-%d is assigned to [%d,%d]\n",
					  i, CHIP_X(XY), CHIP_Y(XY));
		}
#endif

	}
	// for other profiler chip, just get its own ID
	else {
		nChips = 0;
		for(r=0; r<=0xFFFF; r++) {
			dest = rtr_p2p_get(r);
			if(r != 6) {
				if(r == 7) {
					my_pID = nChips;
					break;
				}
				else {
					nChips++;
				}
			}
		}
#if(DEBUG_LEVEL>0)
		io_printf(IO_BUF, "My pID = %d\n", my_pID);
#endif
	}


	/* old version:
	short id=-1, i;

	uint x = CHIP_X(sv->p2p_addr);
	uint y = CHIP_Y(sv->p2p_addr);

	for(i=0; i<48; i++) {
		if(profIDTable[i][0]==x && profIDTable[i][1]==y) {
			id = i; break;
		}
	}
	return id;
	*/
}


uchar getNumActiveCores()
{
	uchar i, nCores = sv->num_cpus, nApp = 0;
	for(i=0; i<nCores; i++) {
		if(sv->vcpu_base[i].cpu_state >= CPU_STATE_RUN &&
		   sv->vcpu_base[i].cpu_state < CPU_STATE_EXIT)
			nApp++;
	}
	return nApp; //nApp includes the monitor core and the profiler core!!!
}

void sanityCheck()
{
    if(sark_core_id() != DEF_PROFILER_CORE) {
        io_printf(IO_STD, "[ERR] Put me at core-%d!\n", DEF_PROFILER_CORE);
        rt_error(RTE_SWERR);
    }
    if(sark_app_id() != DEF_MY_APP_ID) {
        io_printf(IO_STD, "[ERR] Give me app id-%d!\n", DEF_MY_APP_ID);
        rt_error(RTE_SWERR);
    }
}
