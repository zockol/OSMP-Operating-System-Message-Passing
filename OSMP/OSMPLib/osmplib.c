#include <unistd.h>
#include <stdio.h>
#define SharedMemSize 100
#define SharedMemName "/shm"


int OSMP_Init() {
    printf("init\n");
    return 0;
}

int OSMP_Finalize() {
    printf("finalize\n");
    return 0;
}

int OSMP_Size() {
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
