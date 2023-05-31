
#include "osmplib.h"
//#include "../OSMPStarter/osmprun.c"
SharedMem* shm;


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
        }
    }

    if (shm == MAP_FAILED) {
        printf("Mapping Fail: %s\n", strerror(errno));
        shm_unlink(SharedMemName);
        return OSMP_ERROR;
    }

    return OSMP_SUCCESS;

}



int OSMP_Barrier(){

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
    printf("finalize\n");
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
    for(int i = 0; i<shm->processAmount;i++ ){
        if (shm->p[i].rank = dest) {
            sem_wait(&shm->p[i].full);
            pthread_mutex_lock(&shm->mutex);
            shm->p[i].msg[shm->p[i].firstmsg].srcRank = getSrcRank();
            for (int j = 0; j <= count; j++){
               /// shm->p[i].msg[shm->p[i].firstmsg].buffer = &(buf+i);
            }
            sem_post(&(shm->p[i].full));
            pthread_mutex_unlock(&shm->mutex);
        }
    }
    return 0;
}

int getSrcRank(){
    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == getpid()) {
            return shm->p[i].rank;
        }
    }
}

int OSMP_Recv() {
    return 0;
}

int OSMP_Bcast() {
    printf("broadcast\n");
    return 0;
}
