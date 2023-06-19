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
    if (size != 2) {
        exit(-1);
    }
    if (rank == 0) { // OSMP process
        for (int i = 0; i < 20; i++) {
            bufin[0] = i + 1;
            rv = OSMP_Send(bufin, 1, OSMP_INT, 1);
        }

    } else { // OSMP process 1
        for (int i = 0; i < 20; i++) {
            printf("Moment bitte, 2 Sekunden Kaffepause\n");
            sleep(2);
            rv = OSMP_Recv(bufout, 1, OSMP_INT, &source, &len);
            printf("OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufout[0]);
        }
    }
    rv = OSMP_Finalize();
    return 0;
}