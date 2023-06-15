/******************************************************************************
* FILE: osmp_SendRecv.c
* DESCRIPTION:
* OSMP program with a simple pair of blocking OSMP_Send/OSMP_Recv calls
*
* LAST MODIFICATION: Felix Grüning, March 21, 2023
******************************************************************************/
#include <stdio.h>
#include "../OSMPLib/osmplib.h"

int main(int argc, char *argv[]) {
    int rv, size, rank, source, bcastSource;
    OSMP_Request request;
    int bufin[1],bufout[1], len, bcastLen, bcastbufin[3], bcastbufout[3];
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);

    if (size != 2) {
        printf("size != 2\n");
        exit(-1);
    }
    if (rank == 0) { // OSMP process 0 

        sleep(3);
        bufin[0] = 1337;
        rv = OSMP_Send(bufin, 1, OSMP_INT, 1);

        bcastbufin[0] = 4711;
        bcastbufin[1] = 4712;
        bcastbufin[2] = 4713;
        rv = OSMP_Bcast( bcastbufin, 3, OSMP_INT, true, NULL, NULL);

    } else { // OSMP process 1
        rv = OSMP_CreateRequest( &request );

        rv = OSMP_Irecv( bufout, size, OSMP_INT, &source, &len, request );

        rv = OSMP_Bcast( bcastbufout, 3, OSMP_INT, false, &bcastSource, &bcastLen);
        printf("BROADCAST: OSMP process %d received %d byte from %d [%d:%d:%d] \n", rank, bcastLen, bcastSource, bcastbufout[0], bcastbufout[1], bcastbufout[2]);
        rv = OSMP_Wait( request );
        printf("IRECV: OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufout[0]);
        rv = OSMP_RemoveRequest( &request );
    }
    rv = OSMP_Finalize();
    return 0;
}