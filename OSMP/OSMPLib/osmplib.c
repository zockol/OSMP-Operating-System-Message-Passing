
#include "osmplib.h"
//#include "../OSMPStarter/osmprun.c"
SharedMem* shm;

int rankNow = 0;

int debug(char *functionName, int srcRank, char error[], int memory ) {

    char buffer[1024];

    int timestamp = (int)time(NULL);

    sprintf(buffer, "Timestamp: %d, Funktion: %s, OSMPRank: %d\n", timestamp, functionName, srcRank);

    // if level 2 -> log to file error

    // if level 3 -> memory 1 = log


    FILE* file = fopen(shm->log.logPath, "a");
    if (file) {
        fprintf(file, "%s", buffer);
        fclose(file);
    } else {
      printf("Fehler beim öffnen der Datei\n");
        return OSMP_ERROR;
    }

    return OSMP_SUCCESS;
}

int OSMP_Init(int *argc, char ***argv) {



    int fileDescriptor = shm_open(SharedMemName, O_CREAT | O_RDWR, 0640);

    if (fileDescriptor == -1) {
        printf("Error beim shm_open\n");
        return OSMP_ERROR;
    }

    struct stat *shm_stat = calloc(1, sizeof(struct stat));
    if (shm_stat == NULL) {
        printf("Error beim shm_stat\n");
        return OSMP_ERROR;
    }

    if (fstat(fileDescriptor, shm_stat) != 0) {
        printf("Error beim fstat\n");
        return OSMP_ERROR;
    }
    size_t shm_size = (size_t) shm_stat->st_size;
    free(shm_stat);



    shm = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);

    if (shm == MAP_FAILED) {
        printf("Error beim mmap\n");
        return OSMP_ERROR;
    }


    int i = 0, breaker = 0;
    for (i = 0; i<shm->processAmount; i++) {
        if (shm->p[i].pid == 0 && breaker == 0) {
            shm->p[i].pid = getpid();
            breaker = 1;
        }
    }



    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == getpid()) {
            shm->p[i].rank = i;
            rankNow = i;
        }
    }

    if (shm == MAP_FAILED) {
        printf("Mapping Fail: %s\n", strerror(errno));
        shm_unlink(SharedMemName);
        return OSMP_ERROR;
    }

    debug("OSMP_INIT", rankNow, NULL, 0);

    return OSMP_SUCCESS;

}



int OSMP_Barrier(){

    debug("OSMP_BARRIER", rankNow, NULL, 0);

    pthread_mutex_lock(&shm->mutex);
    shm->barrier_all--;

    if( shm->barrier_all == 0) {
        pthread_cond_broadcast(&shm->cattr);
    }else{
        while( shm->barrier_all != 0) {
            pthread_cond_wait(&shm->cattr, &shm->mutex);
        }
    }
    pthread_mutex_unlock(&shm->mutex);
    return 0;
}

int OSMP_Finalize() {
    debug("OSMP_FINALIZE", rankNow, NULL, 0);
    printf("finalize\n");
    return 0;
}

int OSMP_Size(int *size) {
    debug("OSMP_SIZE", rankNow, NULL, 0);
    *size = shm->processAmount;
    return OSMP_SUCCESS;
}

int OSMP_Rank(int *rank) {
    debug("OSMP_RANK", rankNow, NULL, 0);

    if (shm == NULL) {
        printf("shm not initialized");
        return OSMP_ERROR;
    }

    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == getpid()) {
            *rank = shm->p[i].rank;
        }
    }
    return OSMP_SUCCESS;
}

int OSMP_Send() {
    debug("OSMP_SEND", rankNow, NULL, 0);
    printf("send\n");
    return 0;
}

int OSMP_Recv() {
    debug("OSMP_RECV", rankNow, NULL, 0);
    printf("receive\n");
    return 0;
}

int OSMP_Bcast() {
    debug("OSMP_BCAST", rankNow, NULL, 0);
    printf("broadcast\n");
    return 0;
}
