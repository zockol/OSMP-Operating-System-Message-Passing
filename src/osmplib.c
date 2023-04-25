#include <unistd.h>
#include <stdio.h>
#include "osmplib.h"



int OSMP_Init() {
    printf("init\n");
    return 0;
}

int OSMP_Finalize(void) {
    printf("finalize\n");
    return 0;
}

int OSMP_Size(int *size) {
    printf("size\n");
    return 0;
}

int OSMP_Rank() {
    printf("rank\n");
    return 0;
}

int OSMP_Send() {
    printf("send\n");
    return 0;
}

int OSMP_Recv() {
    printf("receive\n");
    return 0;
}

int OSMP_Bcast() {
    printf("broadcast\n");
    return 0;
}