#include "profiler.h"

/*----------------------------------------------------------------------*/
/*------------------------- Temperature Reading ------------------------*/

/***********************************************************************
 * in readTemp(), read the sensors and put the result in val[]
 * In addition, the value of sensor-2 will be used as return value.
 ***********************************************************************/
uint readTemp()
{
    uint i, done, S[] = {SC_TS0, SC_TS1, SC_TS2};

    for(i=0; i<3; i++) {
        done = 0;
        // set S-flag to 1 and wait until F-flag change to 1
        sc[S[i]] = 0x80000000;
        do {
            done = sc[S[i]] & 0x01000000;
        } while(!done);
        // turnoff S-flag and read the value
        sc[S[i]] = sc[S[i]] & 0x0FFFFFFF;
        tempVal[i] = sc[S[i]] & 0x00FFFFFF;
    }
    return tempVal[2];
}

