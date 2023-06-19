//
// Created by ubuntu on 25.04.23.
//

#include "../OSMPLib/osmplib.h"

int SendRecvAllDatatypes(int argc, char **argv) {
    int rv, size, rank, source;
    int bufinInt[1], bufoutInt[1], len;
    short bufinShort[1], b
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);
    if (size != 2) {
        exit(-1);
    }
    if (rank == 0) { // OSMP process
        //INT
        bufinInt[0] = 1337;
        rv = OSMP_Send(bufinInt, 1, OSMP_INT, 1);

        //SHORT
        bufinShort[0] = 1337;
        rv = OSMP_Send(bufinShort, 1, OSMP_SHORT, 1);

        //LONG
        bufinLong[0] = 1337;
        rv = OSMP_Send(bufinLong, 1, OSMP_LONG, 1);

        //BYTE
        bufinBYTE = "Hello World!";
        rv = OSMP_Send(bufinBYTE, 1, OSMP_BYTE, 1);

        //UNSIGNED_CHAR
        bufinUNSIGNEDCHAR = "Hello World!";
        rv = OSMP_Send(bufinUNSIGNEDCHAR, 1, OSMP_UNSIGNED_CHAR, 1);

        //UNSIGNED_SHORT
        bufinUNSIGNEDSHORT = "Hello World!";
        rv = OSMP_Send(bufinUNSIGNEDSHORT, 1, OSMP_UNSIGNED_SHORT, 1);

        //UNSIGNED
        bufinUNSIGNED = "Hello World!";
        rv = OSMP_Send(bufinUNSIGNED, 1, OSMP_UNSIGNED, 1);

        //FLOAT
        bufinFLOAT = "Hello World!";
        rv = OSMP_Send(bufinFLOAT, 1, OSMP_FLOAT, 1);

        //DOUBLE
        bufinDOUBLE = "Hello World!";
        rv = OSMP_Send(bufinDOUBLE, 1, OSMP_DOUBLE, 1);

        rv = OSMP_Send()

    } else { // OSMP process 1
            sleep(2);
            rv = OSMP_Recv(bufoutInt, 1, OSMP_INT, &source, &len);
            printf("OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufoutInt[0]);
    }
    rv = OSMP_Finalize();
    return 0;
}

int IsendIRecv(int argc, char **argv) {
    int rv, size, rank, source, bcastSource;
    OSMP_Request request = NULL;
    int len, bcastLen;
    float bufin[1],bufout[1];
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
        bufin[0] = 1.234245;
        rv = OSMP_Send(bufin, 1, OSMP_FLOAT, 0);
        bcastbufin = malloc(strlen("Hello World!") + 1);
        strncpy(bcastbufin, "Hello World!", strlen("Hello World!") + 1);
        rv = OSMP_Bcast( bcastbufin, strlen("Hello World!") + 1, OSMP_BYTE, true, NULL, NULL);

    } else { // OSMP process 1
        rv = OSMP_CreateRequest( &request );

        rv = OSMP_Irecv( bufout, 1, OSMP_FLOAT, &source, &len, request );
        bcastbufout = malloc(strlen("Hello World!") + 1);
        rv = OSMP_Bcast( bcastbufout, strlen("Hello World!") + 1, OSMP_BYTE, false, &bcastSource, &bcastLen);
        printf("BROADCAST: OSMP process %d received %d byte from %d [%s] \n", rank, bcastLen, bcastSource, bcastbufout);
        sleep(2);
        rv = OSMP_Wait( request );
        printf("IRECV: OSMP process %d received %d byte from %d [%f] \n", rank, len, source, bufout[0]);
        rv = OSMP_RemoveRequest( &request );
    }
    rv = OSMP_Finalize();
    return OSMP_SUCCESS;
}

int BroadcastTest(int argc, char **argv) {
    int rv, size, rank, source;
    int bufin[3], bufout[3], len;
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);
    if (size < 3) {
        printf("wrong size");
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
    return OSMP_SUCCESS;
}

int BarrierTest(int argc, char **argv) {
    int rv = 0, size = 0, rank = 0;
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);

    for (int i = 0; i < 3; i++) {
        printf("[%d] catched  | [%d] iteration\n", rank, i+1);
        OSMP_Barrier();
        printf("[%d] released | [%d] iteraion\n", rank, i+1);
        usleep(500*1000);
    }
    printf("finished: %d\n", rank);

    OSMP_Finalize();
    return OSMP_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (atoi(argv[1]) == 1) {
        BarrierTest(argc, argv);
    } else if (atoi(argv[1]) == 2) {
        BroadcastTest(argc, argv);
    } else if (atoi(argv[1]) == 3) {
        IsendIRecv(argc, argv);
    } else if (atoi(argv[1]) == 2) {
        SendRecvAllDatatypes(argc, argv);
    }

    return OSMP_SUCCESS;
}

