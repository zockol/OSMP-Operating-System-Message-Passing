/******************************************************************************
* FILE: osmp_SendRecv.c
* DESCRIPTION:
* OSMP program with a simple pair of blocking OSMP_Send/OSMP_Recv calls
*
* LAST MODIFICATION: Darius Malysiak, March 21, 2023
******************************************************************************/
#include <stdio.h>
#include "../OSMPLib/osmplib.h"

int main(int argc, char *argv[]) {
    int rv, size, rank, source;
    int bufin[3], bufout[3], len;
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);
    if (size < 3) {
        exit(-1);
    }
    if (rank == 3) { // OSMP process 0
        bufin[0] = 4711;
        bufin[1] = 4712;
        bufin[2] = 4713;
        rv = OSMP_Bcast(bufin, 3, OSMP_INT, true, NULL, NULL);
    } else { // OSMP process 1
        rv = OSMP_Bcast(bufout, 3, OSMP_INT, false, &source, &len);
        printf("OSMP process %d received %d byte from %d broadcast [%d:%d:%d] \n", rank, len, source, bufout[0], bufout[1], bufout[2]);
    }
    rv = OSMP_Finalize();
    return 0;
}