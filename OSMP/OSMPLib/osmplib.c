
#include "osmplib.h"

SharedMem *shm;

int rankNow = 0;

int debug(char *functionName, int srcRank, char *error, char *memory) {

    if (shm->log.logIntensity == -1) return OSMP_SUCCESS;

    char buffer[1024];

    int timestamp = (int) time(NULL);

    if (error != NULL && memory != NULL) {
        sprintf(buffer, "Timestamp: %d, Error: MEMORY AND DEBUG != NULL IN debug(), Funktion: %s, OSMPRank: %d\n",
                timestamp, functionName, srcRank);
    } else {
        if (shm->log.logIntensity >= 2 && error != NULL) {
            sprintf(buffer, "Timestamp: %d, Error: %s, Funktion: %s, OSMPRank: %d\n", timestamp, error, functionName,
                    srcRank);
        } else if (shm->log.logIntensity == 3 && memory != NULL) {
            sprintf(buffer, "Timestamp: %d, Memory: %s, Funktion: %s, OSMPRank: %d\n", timestamp, memory, functionName,
                    srcRank);
        } else if (error == NULL && memory == NULL) {
            sprintf(buffer, "Timestamp: %d, Funktion: %s, OSMPRank: %d\n", timestamp, functionName, srcRank);
        }
    }

    char *result = NULL;

    result = strstr(buffer, "Timestamp");

    if (result != NULL) {
        FILE *file = fopen(shm->log.logPath, "a");
        if (file) {
            fprintf(file, "%s", buffer);
            fclose(file);
        } else {
            printf("Fehler beim öffnen der Datei\n");
            return OSMP_ERROR;
        }
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


    shm = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);

    if (shm == MAP_FAILED) {
        printf("Error beim mmap\n");
        return OSMP_ERROR;
    }
    debug("OSMP_INIT", rankNow, NULL, "calloc");

    free(shm_stat);
    debug("OSMP_INIT", rankNow, NULL, "free");

    int i = 0, breaker = 0;
    for (i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == 0 && breaker == 0) {
            shm->p[i].pid = getpid();
            breaker = 1;
        }
    }

    for (i = 0; i < shm->processAmount; i++) {
        shm->p[i].slots.firstEmptySlot = 0;
        // shm->p[i].slots.lastEmptySlot = 0;
        shm->p[i].numberOfMessages = 0;

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


    if (shm->processesCreated = shm->processAmount) {
        pthread_cond_broadcast(&shm->allCreated);
    }
    debug("OSMP_INIT END", rankNow, NULL, NULL);


    return OSMP_SUCCESS;

}


int OSMP_Barrier() {

    debug("OSMP_BARRIER START", rankNow, NULL, NULL);

    pthread_mutex_lock(&shm->mutex);
    shm->barrier_all--;

    if (shm->barrier_all == 0) {
        pthread_cond_broadcast(&shm->cattr);
    } else {
        while (shm->barrier_all != 0) {
            pthread_cond_wait(&shm->cattr, &shm->mutex);
        }
    }
    pthread_mutex_unlock(&shm->mutex);

    debug("OSMP_BARRIER END", rankNow, NULL, NULL);
    return 0;
}

int OSMP_Finalize() {
    debug("OSMP_FINALIZE START", rankNow, NULL, NULL);

    if (shm == NULL) {
        printf("OSMPLIB.c OSMP_FINALIZE shm not initialized");
        return OSMP_ERROR;
    }

    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].rank == rankNow) {
            shm->p[i].pid = 0;
            shm->p[i].rank = -1;
            shm->p[i].firstmsg = -1;

            sem_destroy(&shm->p[i].empty);
            sem_destroy(&shm->p[i].full);

            shm->processesCreated--;

            if (munmap(shm, (shm->processAmount * sizeof(slots) + max_messages * shm->processAmount * sizeof(message) +
                             shm->processAmount * sizeof(process) + sizeof(logger) +
                             sizeof(Bcast))) == OSMP_ERROR) {
                debug("OSMP_FINALIZE", rankNow, "MUNMAP == OSMP_ERROR", NULL);
            }

            if (i == (shm->processAmount - 1)) {
                shm = NULL;
            }

        }
    }

    return OSMP_SUCCESS;

}

int OSMP_Size(int *size) {
    debug("OSMP_SIZE START", rankNow, NULL, NULL);
    *size = shm->processAmount;
    debug("OSMP_SIZE END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

int OSMP_Rank(int *rank) {
    debug("OSMP_RANK START", rankNow, NULL, NULL);

    if (shm == NULL) {
        printf("shm not initialized");
        return OSMP_ERROR;
    }

    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == getpid()) {
            *rank = shm->p[i].rank;
        }
    }

    debug("OSMP_RANK END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}


int OSMP_Send(const void *buf, int count, OSMP_Datatype datatype, int dest) {
    debug("OSMP_SEND START", rankNow, NULL, NULL);
    int j;

    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].rank == dest) {

            sem_wait(&shm->p[i].empty);
            pthread_mutex_lock(&shm->mutex);
            shm->p[i].msg[shm->p[i].slots.firstEmptySlot].msgLen = count * sizeof(datatype);
            shm->p[i].msg[shm->p[i].slots.firstEmptySlot].datatype = datatype;
            shm->p[i].msg[shm->p[i].slots.firstEmptySlot].srcRank = rankNow;
            shm->p[i].msg[shm->p[i].slots.firstEmptySlot].destRank = dest;

            memcpy(shm->p[i].msg[shm->p[i].slots.firstEmptySlot].buffer, buf, shm->p[i].msg[shm->p[i].slots.firstEmptySlot].msgLen);
            shm->p[i].msg[shm->p[i].slots.firstEmptySlot].full = true;
            shm->p[i].slots.firstEmptySlot++;
            shm->p[i].numberOfMessages++;
            shm->p[i].firstmsg++;

            pthread_mutex_unlock(&shm->mutex);

            sem_post(&shm->p[i].full);


        }
    }
    debug("OSMP_SEND END", rankNow, NULL, NULL);
    return 0;
}

int OSMP_Recv(void *buf, int count, OSMP_Datatype datatype, int *source, int *len) {
    debug("OSMP_RECV START", rankNow, NULL, NULL);
    int i = 0;
    for (i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == getpid()) {
            pthread_cond_broadcast(&shm->allCreated);

            sem_wait(&shm->p[i].full);

            pthread_mutex_lock(&shm->mutex);

            *source = shm->p[i].msg[shm->p[i].firstmsg].srcRank;
            *len = shm->p[i].msg[shm->p[i].firstmsg].msgLen;
            memcpy(buf, shm->p[i].msg[shm->p[i].firstmsg].buffer, count * sizeof(datatype));
            shm->p[i].firstmsg--;
            shm->p[i].slots.firstEmptySlot--;
            pthread_mutex_unlock(&shm->mutex);


            sem_post(&shm->p[i].empty);


        }

    }

    debug("OSMP_RECV END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

int OSMP_Bcast(void *buf, int count, OSMP_Datatype datatype, bool send, int *source, int *len) {
    debug("OSMP_BCAST START", rankNow, NULL, NULL);

    if (send == true) {

        shm->broadcastMsg.datatype = datatype;
        shm->broadcastMsg.msgLen = count * sizeof(datatype);
        shm->broadcastMsg.srcRank = rankNow;
        memcpy(shm->broadcastMsg.buffer, buf, shm->broadcastMsg.msgLen);
    }
    OSMP_Barrier();
    if (send == false) {
        memcpy(buf, shm->broadcastMsg.buffer, shm->broadcastMsg.msgLen);
        *source = shm->broadcastMsg.srcRank;
        *len = shm->broadcastMsg.msgLen;
    }

    debug("OSMP_BCAST END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}


int calculateStruct(int *rank) {
    int i = 0, j = 0, all = 0, b = 0;

    for (i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].rank == *rank) {
            for (j = 0; j < OSMP_MAX_MESSAGES_PROC; j++) {
                if (!(shm->p[i].msg[j].full) && b == 0) {
                    shm->p[i].slots.firstEmptySlot = j;
                    b += 1;
                }
                if ((shm->p[i].msg[j].full)) {
                    all++;
                    printf("number of messages = %d", all);
                }
            }
        }
        shm->p[i].numberOfMessages = all;

        all = 0;
        b = 0;
    }
    return 0;
}
