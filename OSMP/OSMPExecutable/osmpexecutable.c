//
// Created by ubuntu on 25.04.23.
//

#include "../OSMPLib/osmplib.h"

int getSHMName(int argc, char **argv) {
    int rv, size, rank;
    char* name;
    OSMP_Request request = NULL;
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);

    rv = OSMP_GetShmName(&name);

    printf("SHM-Name: %s\n", name);
    return OSMP_SUCCESS;
}

int SendRecv(int argc, char **argv) {
    int rv, size, rank, source;
    int bufin[2], bufout[2], len;
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);
    if (size != 2) {
        exit(-1);
    }
    if (rank == 0) { // OSMP process

        bufin[0] = 1337;
        bufin[1] = 7331;
        rv = OSMP_Send(bufin, 2, OSMP_INT, 1);


    } else { // OSMP process 1
        sleep(2);
        rv = OSMP_Recv(bufout, 2, OSMP_INT, &source, &len);
        printf("OSMP process %d received %d byte from %d [%d:%d] \n", rank, len, source, bufout[0], bufout[1]);

    }
    rv = OSMP_Finalize();
    return 0;
}

int IsendIRecv(int argc, char **argv) {
    int rv, size, rank, source, bcastSource;
    OSMP_Request sendRequest = NULL, recvRequest = NULL;
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

        rv = OSMP_CreateRequest( &sendRequest );
        rv = OSMP_Isend(bufin, 1, OSMP_FLOAT, 0, sendRequest);

        bcastbufin = malloc(strlen("Hello World!") + 1);
        strncpy(bcastbufin, "Hello World!", strlen("Hello World!") + 1);
        rv = OSMP_Bcast( bcastbufin, strlen("Hello World!") + 1, OSMP_BYTE, true, NULL, NULL);

        rv = OSMP_Wait( sendRequest );
        rv = OSMP_RemoveRequest( &sendRequest );

    } else { // OSMP process 1
        rv = OSMP_CreateRequest( &recvRequest );

        rv = OSMP_Irecv( bufout, 1, OSMP_FLOAT, &source, &len, recvRequest );
        bcastbufout = malloc(strlen("Hello World!") + 1);
        rv = OSMP_Bcast( bcastbufout, strlen("Hello World!") + 1, OSMP_BYTE, false, &bcastSource, &bcastLen);
        printf("BROADCAST: OSMP process %d received %d byte from %d [%s] \n", rank, bcastLen, bcastSource, bcastbufout);
        sleep(2);
        rv = OSMP_Wait( recvRequest );
        printf("IRECV: OSMP process %d received %d byte from %d [%f] \n", rank, len, source, bufout[0]);
        rv = OSMP_RemoveRequest( &recvRequest );
    }
    rv = OSMP_Finalize();
    return OSMP_SUCCESS;
}

int SendRecvMoreThan16(int argc, char **argv) {
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

int SendRecvAllDatatypes(int argc, char **argv) {
    int rv, size, rank, source;
    int bufinINT[2], bufoutINT[2], len;
    short bufinSHORT[1], bufoutSHORT[1];
    long bufinLONG[1], bufoutLONG[1];
    char *bufinBYTE, *bufoutBYTE;
    unsigned char bufinUNSIGNEDCHAR[1], bufoutUNSIGNEDCHAR[1];
    unsigned short bufinUNSIGNEDSHORT[1], bufoutUNSIGNEDSHORT[1];
    unsigned bufinUNSIGNED[1], bufoutUNSIGNED[1];
    float bufinFLOAT[1], bufoutFLOAT[1];
    double bufinDOUBLE[1], bufoutDOUBLE[1];
    rv = OSMP_Init(&argc, &argv);
    rv = OSMP_Size(&size);
    rv = OSMP_Rank(&rank);
    if (size != 2) {
        exit(-1);
    }
    if (rank == 0) { // OSMP process

        //INT
        bufinINT[0] = 1;
        bufinINT[1] = 1337;
        rv = OSMP_Send(bufinINT, 2, OSMP_INT, 1);

        //SHORT
        bufinSHORT[0] = 2;
        rv = OSMP_Send(bufinSHORT, 1, OSMP_SHORT, 1);

        //LONG
        bufinLONG[0] = 3;
        rv = OSMP_Send(bufinLONG, 1, OSMP_LONG, 1);

        //BYTE
        bufinBYTE = malloc(strlen("Hier kommt die 4!") + 1);
        strncpy(bufinBYTE, "Hier kommt die 4!", strlen("Hier kommt die 4!") + 1);
        rv = OSMP_Send(bufinBYTE, strlen("Hier kommt die 4!") + 1, OSMP_BYTE, 1);

        //UNSIGNED_CHAR
        bufinUNSIGNEDCHAR[0] = 5;
        rv = OSMP_Send(bufinUNSIGNEDCHAR, 1, OSMP_UNSIGNED_CHAR, 1);

        //UNSIGNED_SHORT
        bufinUNSIGNEDSHORT[0] = 6;
        rv = OSMP_Send(bufinUNSIGNEDSHORT, 1, OSMP_UNSIGNED_SHORT, 1);

        //UNSIGNED
        bufinUNSIGNED[0] = 7;
        rv = OSMP_Send(bufinUNSIGNED, 1, OSMP_UNSIGNED, 1);

        //FLOAT
        bufinFLOAT[0] = 8.8888;
        rv = OSMP_Send(bufinFLOAT, 1, OSMP_FLOAT, 1);

        //DOUBLE
        bufinDOUBLE[0] = 9.9999;
        rv = OSMP_Send(bufinDOUBLE, 1, OSMP_DOUBLE, 1);


    } else { // OSMP process 1
        sleep(3);
        rv = OSMP_Recv(bufoutDOUBLE, 1, OSMP_DOUBLE, &source, &len);
        printf("DOUBLE: OSMP process %d received %d byte from %d [%f] \n", rank, len, source, bufoutDOUBLE[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutFLOAT, 1, OSMP_FLOAT, &source, &len);
        printf("FLOAT: OSMP process %d received %d byte from %d [%f] \n", rank, len, source, bufoutFLOAT[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutUNSIGNED, 1, OSMP_UNSIGNED, &source, &len);
        printf("UNSIGNED: OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufoutUNSIGNED[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutUNSIGNEDSHORT, 1, OSMP_UNSIGNED_SHORT, &source, &len);
        printf("UNSIGNED_SHORT: OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufoutUNSIGNEDSHORT[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutUNSIGNEDCHAR, 1, OSMP_UNSIGNED_CHAR, &source, &len);
        printf("UNSIGNED_CHAR: OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufoutUNSIGNEDCHAR[0]);
        sleep(1);
        bufoutBYTE = malloc(strlen("Hier kommt die 4!") + 1);
        rv = OSMP_Recv(bufoutBYTE, strlen("Hier kommt die 4!") + 1, OSMP_BYTE, &source, &len);
        printf("BYTE: OSMP process %d received %d byte from %d [%s] \n", rank, len, source, bufoutBYTE);
        sleep(1);
        rv = OSMP_Recv(bufoutLONG, 1, OSMP_LONG, &source, &len);
        printf("LONG: OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufoutLONG[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutSHORT, 1, OSMP_SHORT, &source, &len);
        printf("SHORT: OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufoutSHORT[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutINT, 2, OSMP_INT, &source, &len);
        printf("INT: OSMP process %d received %d byte from %d [%d:%d] \n", rank, len, source, bufoutINT[0], bufoutINT[1]);
        sleep(1);

    }
    rv = OSMP_Finalize();
    return 0;
}

int sendBcastIRecvTest(int argc, char **argv) {
    int rv, size, rank, source, bcastSource, flag;
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
        rv = OSMP_Test(request, &flag);
        flag == 1 ? printf("request completed!\n") : printf("request not completed!\n");

        bcastbufout = malloc(strlen("Hello World!") + 1);
        rv = OSMP_Bcast( bcastbufout, strlen("Hello World!") + 1, OSMP_BYTE, false, &bcastSource, &bcastLen);
        printf("BROADCAST: OSMP process %d received %d byte from %d [%s] \n", rank, bcastLen, bcastSource, bcastbufout);
        sleep(2);

        rv = OSMP_Wait( request );
        rv = OSMP_Test(request, &flag);
        flag == 1 ? printf("request completed!\n") : printf("request not completed!\n");
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
        sendBcastIRecvTest(argc, argv);
    } else if (atoi(argv[1]) == 4) {
        SendRecvAllDatatypes(argc, argv);
    } else if (atoi(argv[1]) == 5) {
        SendRecvMoreThan16(argc, argv);
    } else if (atoi(argv[1]) == 6) {
        IsendIRecv(argc, argv);
    } else if (atoi(argv[1]) == 7) {
        SendRecv(argc, argv);
    } else if (atoi(argv[1]) == 8) {
        getSHMName(argc, argv);
    }

    return OSMP_SUCCESS;
}

