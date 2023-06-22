//
// Created by ubuntu on 25.04.23.
//

#include "./osmpexecutable.h"

//secret for 1337 mode
int system(const char *command);

int printError(int Line) {
    printf("Error in osmpexecutable in Line: %d\n", Line);
    exit(-1);
};

//Prozess n sendet Nachricht an Prozess n+1 um die max_messages zu testen
int SendRecvNextNeighbour(int argc, char **argv) {
    int size, rank, source, rv = 0;
    int bufin[1], bufout[1], len;
    rv = OSMP_Init(&argc, &argv);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Size(&size);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Rank(&rank);
    if (rv == -1) printError(__LINE__);


    //wenn kleiner als 256 ergibt der Test wenig sinn
    if (size < 256) {
        printf("size < 256\n");
        exit(-1);
    }
    //solange rank 0-498
    if (rank < size-1) {
        bufin[0] = 1337;
        rv = OSMP_Send(bufin, 1, OSMP_INT, rank + 1);
    }
    //solange rank nicht 0
    if (rank > 0) {
        sleep(20);
        rv = OSMP_Recv(bufout, 1, OSMP_INT, &source, &len);
        printf("OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufout[0]);
    }
    rv = OSMP_Finalize();
    if (rv == -1) printError(__LINE__);
    return rv;
}

//kein kommentar zu dieser geheimen funktion :^)
int performTrainAction(int argc, char **argv) {
    int rv, size, rank;
    rv = OSMP_Init(&argc, &argv);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Size(&size);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Rank(&rank);
    if (rv == -1) printError(__LINE__);
    if (rank == 0) {
        system("sl");
    }
    rv = OSMP_Finalize();
    if (rv == -1) printError(__LINE__);
    return OSMP_SUCCESS;
}

//Simple Testfunktion um den char* name mit OSMP_GetShmName zu beschreiben
int getSHMName(int argc, char **argv) {
    int rv,  size, rank;
    char* name;
    rv = OSMP_Init(&argc, &argv);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Size(&size);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Rank(&rank);
    if (rv == -1) printError(__LINE__);

    if (rank == 0) {
        rv = OSMP_GetShmName(&name);
        if (rv == -1) printError(__LINE__);
        printf("SHM-Name: %s\n", name);
    }

    rv = OSMP_Finalize();
    if (rv == -1) printError(__LINE__);
    return rv;
}

//Simples Send und Receive in blockierenden Funktionen
int SendRecv(int argc, char **argv) {
    int rv, size, rank, source;
    int bufin[2], bufout[2], len;
    rv = OSMP_Init(&argc, &argv);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Size(&size);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Rank(&rank);
    if (rv == -1) printError(__LINE__);
    if (size != 2) {
        exit(-1);
    }
    if (rank == 0) { // OSMP process

        bufin[0] = 1337;
        bufin[1] = 7331;
        rv = OSMP_Send(bufin, 2, OSMP_INT, 1);
        if (rv == -1) printError(__LINE__);


    } else { // OSMP process 1
        sleep(2);
        rv = OSMP_Recv(bufout, 2, OSMP_INT, &source, &len);
        if (rv == -1) printError(__LINE__);
        printf("OSMP process %d received %d byte from %d [%d:%d] \n", rank, len, source, bufout[0], bufout[1]);

    }
    rv = OSMP_Finalize();
    return 0;
}

//ISendIReceive Funktion um zu testen ob beide Requests miteinander argieren. Sleeps sind eingebaut
int IsendIRecv(int argc, char **argv) {
    int rv, size, rank, source;
    OSMP_Request sendRequest = NULL, recvRequest = NULL;
    int len;
    float bufin[1],bufout[1];
    rv = OSMP_Init(&argc, &argv);
    if (rv == -1) printError(__LINE__);

    rv = OSMP_Size(&size);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Rank(&rank);
    if (rv == -1) printError(__LINE__);

    //Funktion ist auf 2 OSMP prozesse aufgebaut worden, daher muss size == 2
    if (size != 2) {
        printf("size != 2\n");
        exit(-1);
    }
    if (rank == 0) { // OSMP process 0

        sleep(10);
        bufin[0] = (float)1.234245;

        rv = OSMP_CreateRequest(&sendRequest);
        if (rv == -1) printError(__LINE__);
        rv = OSMP_Isend(bufin, 1, OSMP_FLOAT, 1, sendRequest);
        if (rv == -1) printError(__LINE__);
        rv = OSMP_Wait(sendRequest);
        if (rv == -1) printError(__LINE__);
        rv = OSMP_RemoveRequest(&sendRequest);
        if (rv == -1) printError(__LINE__);
    } else {
        rv = OSMP_CreateRequest(&recvRequest);
        if (rv == -1) printError(__LINE__);
        rv = OSMP_Irecv(bufout, 1, OSMP_FLOAT, &source, &len, recvRequest);
        if (rv == -1) printError(__LINE__);
        sleep(20);
        rv = OSMP_Wait(recvRequest);
        if (rv == -1) printError(__LINE__);
        printf("IRECV: OSMP process %d received %d byte from %d [%f] \n", rank, len, source, bufout[0]);
        rv = OSMP_RemoveRequest(&recvRequest);
        if (rv == -1) printError(__LINE__);
    }
    OSMP_Finalize();
    return OSMP_SUCCESS;
}

//Funktion um Send und Receive zu testen, wenn der Sender mehr als die Mailbox-Größe sendet
//Receiver Empfängt nur alle 2 Sekunden, so dass es zur maximalen Mailbox-Auslastung kommt
int SendRecvFull(int argc, char **argv) {
    int size, rank, source;
    int rv, bufin[3], bufout[3], len;

    rv = OSMP_Init(&argc, &argv);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Size(&size);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Rank(&rank);
    if (rv == -1) printError(__LINE__);
    if (size != 2) {
        printf("size != 2\n");
        exit(-1);
    }
    if (rank == 0) { // OSMP process
        for (int i = 0; i < 20; i++) {
            bufin[0] = i + 1;
            rv = OSMP_Send(bufin, 1, OSMP_INT, 1);
            if (rv == -1) printError(__LINE__);
        }

    } else { // OSMP process 1
        for (int i = 0; i < 20; i++) {
            printf("1 Sekunden Kaffeepause\n");
            sleep(1);
            rv = OSMP_Recv(bufout, 1, OSMP_INT, &source, &len);
            if (rv == -1) printError(__LINE__);
            printf("OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufout[0]);
        }
    }
    rv = OSMP_Finalize();
    if (rv == -1) printError(__LINE__);
    return 0;
}

//Testet ein Send und Receive mit allen möglichen Datentypen
int DatatypeTest(int argc, char **argv) {
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
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Size(&size);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Rank(&rank);
    if (rv == -1) printError(__LINE__);
    if (size != 2) {
        exit(-1);
    }
    if (rank == 0) { // OSMP process

        //INT
        bufinINT[0] = 1;
        bufinINT[1] = 1337;
        rv = OSMP_Send(bufinINT, 2, OSMP_INT, 1);
        if (rv == -1) printError(__LINE__);

        //SHORT
        bufinSHORT[0] = 2;
        rv = OSMP_Send(bufinSHORT, 1, OSMP_SHORT, 1);
        if (rv == -1) printError(__LINE__);

        //LONG
        bufinLONG[0] = 3;
        rv = OSMP_Send(bufinLONG, 1, OSMP_LONG, 1);
        if (rv == -1) printError(__LINE__);

        //BYTE
        bufinBYTE = malloc(strlen("Hier kommt die 4!") + 1);
        strncpy(bufinBYTE, "Hier kommt die 4!", strlen("Hier kommt die 4!") + 1);
        rv = OSMP_Send(bufinBYTE, strlen("Hier kommt die 4!") + 1, OSMP_BYTE, 1);
        if (rv == -1) printError(__LINE__);

        //UNSIGNED_CHAR
        bufinUNSIGNEDCHAR[0] = 5;
        rv = OSMP_Send(bufinUNSIGNEDCHAR, 1, OSMP_UNSIGNED_CHAR, 1);
        if (rv == -1) printError(__LINE__);

        //UNSIGNED_SHORT
        bufinUNSIGNEDSHORT[0] = 6;
        rv = OSMP_Send(bufinUNSIGNEDSHORT, 1, OSMP_UNSIGNED_SHORT, 1);
        if (rv == -1) printError(__LINE__);

        //UNSIGNED
        bufinUNSIGNED[0] = 7;
        rv = OSMP_Send(bufinUNSIGNED, 1, OSMP_UNSIGNED, 1);
        if (rv == -1) printError(__LINE__);

        //FLOAT
        bufinFLOAT[0] = (float)8.8888;
        rv = OSMP_Send(bufinFLOAT, 1, OSMP_FLOAT, 1);
        if (rv == -1) printError(__LINE__);

        //DOUBLE
        bufinDOUBLE[0] = 9.9999;
        rv = OSMP_Send(bufinDOUBLE, 1, OSMP_DOUBLE, 1);
        if (rv == -1) printError(__LINE__);


    } else { // OSMP process 1
        sleep(3);
        rv = OSMP_Recv(bufoutDOUBLE, 1, OSMP_DOUBLE, &source, &len);
        if (rv == -1) printError(__LINE__);
        printf("DOUBLE: OSMP process %d received %d byte from %d [%f] \n", rank, len, source, bufoutDOUBLE[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutFLOAT, 1, OSMP_FLOAT, &source, &len);
        if (rv == -1) printError(__LINE__);
        printf("FLOAT: OSMP process %d received %d byte from %d [%f] \n", rank, len, source, bufoutFLOAT[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutUNSIGNED, 1, OSMP_UNSIGNED, &source, &len);
        if (rv == -1) printError(__LINE__);
        printf("UNSIGNED: OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufoutUNSIGNED[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutUNSIGNEDSHORT, 1, OSMP_UNSIGNED_SHORT, &source, &len);
        if (rv == -1) printError(__LINE__);
        printf("UNSIGNED_SHORT: OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufoutUNSIGNEDSHORT[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutUNSIGNEDCHAR, 1, OSMP_UNSIGNED_CHAR, &source, &len);
        if (rv == -1) printError(__LINE__);
        printf("UNSIGNED_CHAR: OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufoutUNSIGNEDCHAR[0]);
        sleep(1);
        bufoutBYTE = malloc(strlen("Hier kommt die 4!") + 1);
        rv = OSMP_Recv(bufoutBYTE, strlen("Hier kommt die 4!") + 1, OSMP_BYTE, &source, &len);
        if (rv == -1) printError(__LINE__);
        printf("BYTE: OSMP process %d received %d byte from %d [%s] \n", rank, len, source, bufoutBYTE);
        sleep(1);
        rv = OSMP_Recv(bufoutLONG, 1, OSMP_LONG, &source, &len);
        if (rv == -1) printError(__LINE__);
        printf("LONG: OSMP process %d received %d byte from %d [%ld] \n", rank, len, source, bufoutLONG[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutSHORT, 1, OSMP_SHORT, &source, &len);
        if (rv == -1) printError(__LINE__);
        printf("SHORT: OSMP process %d received %d byte from %d [%d] \n", rank, len, source, bufoutSHORT[0]);
        sleep(1);
        rv = OSMP_Recv(bufoutINT, 2, OSMP_INT, &source, &len);
        if (rv == -1) printError(__LINE__);
        printf("INT: OSMP process %d received %d byte from %d [%d:%d] \n", rank, len, source, bufoutINT[0], bufoutINT[1]);
        sleep(1);

    }
    rv = OSMP_Finalize();
    if (rv == -1) printError(__LINE__);
    return 0;
}

//sendet ein OSMP_Send und ein IReceive. Während des IRecv wird die OSMP_Test kontrolliert
int sendIrecvTest(int argc, char **argv) {
    int rv, size, rank, source, flag;
    OSMP_Request request = NULL;
    int len;
    float bufin[1],bufout[1];
    rv = OSMP_Init(&argc, &argv);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Size(&size);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Rank(&rank);
    if (rv == -1) printError(__LINE__);

    if (size != 2) {
        printf("size != 2\n");
        exit(-1);
    }
    if (rank == 1) { // OSMP process 0
        sleep(15);
        bufin[0] = (float)1.234;
        rv = OSMP_Send(bufin, 1, OSMP_FLOAT, 0);
        if (rv == -1) printError(__LINE__);
    } else { // OSMP process 1
        rv = OSMP_CreateRequest(&request);
        if (rv == -1) printError(__LINE__);
        rv = OSMP_Irecv(bufout, 1, OSMP_FLOAT, &source, &len, request);
        if (rv == -1) printError(__LINE__);
        rv = OSMP_Test(request, &flag);
        if (rv == -1) printError(__LINE__);
        flag == 1 ? printf("request completed!\n") : printf("request not completed!\n");
        sleep(5);
        rv = OSMP_Test(request, &flag);
        if (rv == -1) printError(__LINE__);
        rv = OSMP_Wait(request);
        if (rv == -1) printError(__LINE__);
        flag == 1 ? printf("request completed!\n") : printf("request not completed!\n");
        printf("IRECV: OSMP process %d received %d byte from %d [%f] \n", rank, len, source, bufout[0]);
        rv = OSMP_RemoveRequest(&request);
    }
    rv = OSMP_Finalize();
    if (rv == -1) printError(__LINE__);
    return OSMP_SUCCESS;
}

//Simpler Test ob Broadcast an alle läuft
int BroadcastTest(int argc, char **argv) {
    int rv, size, rank, source;
    int bufin[3], bufout[3], len;
    rv = OSMP_Init(&argc, &argv);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Size(&size);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Rank(&rank);
    if (rv == -1) printError(__LINE__);
    if (size < 3) {
        printf("size < 3\n");
        exit(-1);
    }
    printf("Prozess %d erstellt\n", rank);
    if (rank == 3) { // OSMP process 0
        bufin[0] = 4711;
        bufin[1] = 4712;
        bufin[2] = 4713;
        OSMP_Bcast(bufin, 3, OSMP_INT, true, NULL, NULL);
    } else { // OSMP process 1
        rv = OSMP_Bcast(bufout, 3, OSMP_INT, false, &source, &len);
        if (rv == -1) printError(__LINE__);
        printf("OSMP process %d received %d byte from %d broadcast [%d:%d:%d] \n", rank, len, source, bufout[0], bufout[1], bufout[2]);
    }
    rv = OSMP_Finalize();
    if (rv == -1) printError(__LINE__);
    return OSMP_SUCCESS;
}

//Barriertest mit 10 Iterationen
int BarrierTest(int argc, char **argv) {
    int rv, size = 0, rank = 0;
    rv = OSMP_Init(&argc, &argv);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Size(&size);
    if (rv == -1) printError(__LINE__);
    rv = OSMP_Rank(&rank);
    if (rv == -1) printError(__LINE__);

    for (int i = 0; i < 10; i++) {
        printf("[%d] catched  | [%d] iteration\n", rank, i+1);
        rv = OSMP_Barrier();
        if (rv == -1) printError(__LINE__);
        printf("[%d] released | [%d] iteraion\n", rank, i+1);
    }

    rv = OSMP_Finalize();
    if (rv == -1) printError(__LINE__);
    return OSMP_SUCCESS;
}

//Extra Funktion, welches quasi als Menu gilt bei keiner Eingabe
int printTestValues() {
    printf("1 = BarrierTest\n");
    printf("2 = BroadcastTest\n");
    printf("3 = sendIrecv and OSMP_Test\n");
    printf("4 = DatatypeTest\n");
    printf("5 = SendRecvFull\n");
    printf("6 = ISendIRecv\n");
    printf("7 = SendRecv\n");
    printf("8 = getSHMName\n");
    printf("9 = SendRecvNextNeighbour\n");
    printf("1337 = hidden action\n");
    printf("----------------------------\n");
    return OSMP_SUCCESS;
}

int main(int argc, char *argv[]) {

    if (argc > 1) {
        switch (atoi(argv[1])) {
            case 1:
                BarrierTest(argc, argv);
                break;
            case 2:
                BroadcastTest(argc, argv);
                break;
            case 3:
                sendIrecvTest(argc, argv);
                break;
            case 4:
                DatatypeTest(argc, argv);
                break;
            case 5:
                SendRecvFull(argc, argv);
                break;
            case 6:
                IsendIRecv(argc, argv);
                break;
            case 7:
                SendRecv(argc, argv);
                break;
            case 8:
                getSHMName(argc, argv);
                break;
            case 9:
                SendRecvNextNeighbour(argc, argv);
                break;
            case 1337:
                performTrainAction(argc, argv);
                break;
            default:
                printTestValues();
                break;
        }
    } else {
        printTestValues();
    }

    return OSMP_SUCCESS;
}

