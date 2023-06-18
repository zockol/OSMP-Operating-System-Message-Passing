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
    OSMP_Request request = NULL;
    int bufin[1],bufout[1], len, bcastLen;
    char *bcastbufin, *bcastbufout;
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);

    if (size != 2) {
        printf("size != 2\n");
        exit(-1);
    }
    if (rank == 1) { // OSMP process 0

        sleep(4);
        bufin[0] = 1337;
        rv = OSMP_Send(bufin, 1, OSMP_INT, 0);
        bcastbufin = malloc(strlen("Hello World!") + 1);
        strncpy(bcastbufin, "Hello World!", strlen("Hello World!") + 1);
        rv = OSMP_Bcast( bcastbufin, strlen("Hello World!") + 1, OSMP_BYTE, true, NULL, NULL);

    } else { // OSMP process 1
        rv = OSMP_CreateRequest( &request );

        rv = OSMP_Irecv( bufout, 1, OSMP_INT, &source, &len, request );
        bcastbufout = malloc(strlen("Hello World!") + 1);
        rv = OSMP_Bcast( bcastbufout, strlen("Hello World!") + 1, OSMP_BYTE, false, &bcastSource, &bcastLen);
        printf("BROADCAST: OSMP process %d received %d byte from %d [%s] \n", rank, bcastLen, bcastSource, bcastbufout);
        sleep(2);
        rv = OSMP_Wait( request );
        printf("IRECV: OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufout[0]);
        rv = OSMP_RemoveRequest( &request );
    }
    rv = OSMP_Finalize();
    return 0;
}