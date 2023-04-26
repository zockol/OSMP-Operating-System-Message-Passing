
#include "osmplib.h"

SharedMem* shm;

int OSMP_Init(int *argc, char ***argv) {
    int fileDescriptor = shm_open(SharedMemName, O_CREAT | O_RDWR, 0640);

    if (fileDescriptor == -1) {
        printf("Error beim shm_open\n");
        return OSMP_ERROR;
    }

    int ftrunc = ftruncate(fileDescriptor, SharedMemSize);

    if (ftrunc == -1) {
        printf("Fehler bei ftruncate %s\n", strerror(errno));
        return OSMP_ERROR;
    }

    shm = mmap(NULL, SharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);


    int i = 0, breaker = 0;
    for (i = 0; i<shm->processAmount; i++) {
        if (shm->p[i].pid == 0 && breaker == 0) {
            shm->p[i].pid = getpid();
            breaker = 1;
        }

    }

    if (shm == MAP_FAILED) {
        printf("Mapping Fail: %s\n", strerror(errno));
        shm_unlink(SharedMemName);
        return OSMP_ERROR;
    }


    return OSMP_SUCCESS;
}

int OSMP_Finalize() {
    printf("finalize\n");
    return 0;
}

int OSMP_Size(int *size) {
    *size = shm->processAmount;
    return OSMP_SUCCESS;
}

int OSMP_Rank(int *rank) {
    for (int i = 0; i < shm->processAmount; i++) {
/*        if (shm->p[i].pid == getpid()) {
            *rank = i;
        }*/
    printf("Testprint %d\n", shm->p[i].pid);
    }
    return OSMP_SUCCESS;
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
