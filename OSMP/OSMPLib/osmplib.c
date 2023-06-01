
#include "osmplib.h"

//#include "../OSMPStarter/osmprun.c"
SharedMem *shm;

int rankNow = 0;

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

    return OSMP_SUCCESS;

}


int OSMP_Barrier() {

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
    return 0;
}

int OSMP_Finalize() {

    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == getpid()) {
            printf("FINALIZED %d\n", shm->p[i].rank);
        }
    }
    return OSMP_SUCCESS;
    return 0;
}

int OSMP_Size(int *size) {
    *size = shm->processAmount;
    return OSMP_SUCCESS;
}

int OSMP_Rank(int *rank) {

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

int OSMP_Send(const void *buf, int count, OSMP_Datatype datatype, int dest) {
    int j;

    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].rank == dest) {

            sem_wait(&shm->p[i].empty);
            pthread_mutex_lock(&shm->mutex);
            shm->p[i].msg[shm->p[i].slots.firstEmptySlot].msgLen = count * sizeof(datatype);
            shm->p[i].msg[shm->p[i].slots.firstEmptySlot].datatype = datatype;
            shm->p[i].msg[shm->p[i].slots.firstEmptySlot].srcRank = getSrcRank();
            shm->p[i].msg[shm->p[i].slots.firstEmptySlot].destRank = dest;

            memcpy(shm->p[i].msg[shm->p[i].slots.firstEmptySlot].buffer, buf,
                   shm->p[i].msg[shm->p[i].slots.firstEmptySlot].msgLen);
            shm->p[i].msg[shm->p[i].slots.firstEmptySlot].full = true;
            shm->p[i].slots.firstEmptySlot++;
            shm->p[i].numberOfMessages++;
            shm->p[i].firstmsg++;

            pthread_mutex_unlock(&shm->mutex);

            sem_post(&shm->p[i].full);


            OSMP_Barrier();


        }
    }
    return 0;
}

int getSrcRank() {
    int i, j;
    for (i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == getpid()) {

            j = shm->p[i].rank;
            return j;
        }
    }

}

int OSMP_Recv(void *buf, int count, OSMP_Datatype datatype, int *source, int *len) {
    int i = 0;
    for (i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == getpid()) {
            pthread_cond_broadcast(&shm->allCreated);

            sem_wait(&shm->p[i].full);

            pthread_mutex_lock(&shm->mutex);

            datatype = shm->p[i].msg[shm->p[i].firstmsg].datatype;

            *source = shm->p[i].msg[shm->p[i].firstmsg].srcRank;
            *len = shm->p[i].msg[shm->p[i].firstmsg].msgLen;
            memcpy(buf, shm->p[i].msg[shm->p[i].firstmsg].buffer, count * sizeof(datatype));
            shm->p[i].firstmsg--;
            shm->p[i].slots.firstEmptySlot--;
            pthread_mutex_unlock(&shm->mutex);

            OSMP_Barrier();
            
            sem_post(&shm->p[i].empty);


        }

    }


    return OSMP_SUCCESS;
}

int OSMP_Bcast() {
    printf("broadcast\n");
    return 0;
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
