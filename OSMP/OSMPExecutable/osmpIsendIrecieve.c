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
    int rv, size, rank, source;
    OSMP_Request request;
    int bufin[3],bufout, len;
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);
    
    
    if (size != 2) {
        
        exit(-1);
    }
    if (rank == 0) { // OSMP process 0 
        for (int i = 0; i < 2; i++) {
            bufin[0] = i + 1;
            rv = OSMP_Send(bufin, 1, OSMP_INT, 1);
        }

    } else { // OSMP process 1
    printf("Halloooooooooooooooo");
        // bufout = malloc(size); // check for != NULL
        // rv = OSMP_CreateRequest( &request );
        // rv = OSMP_Irecv( bufout, size, OSMP_INT, &source, &len, request );
        // // do something important…
        // // check if operation is completed and wait if not
        // rv = OSMP_Wait( request );
        // printf("OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufout);
        // rv = OSMP_RemoveRequest( &request );
    }
    rv = OSMP_Finalize();
    return 0;
}