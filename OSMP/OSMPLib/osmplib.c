
#include "osmplib.h"

SharedMem* shm;
sem_t FULL;




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

    shm = mmap(0, SharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);

    
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(shm->MUTEX, &mutex_attr);
    sem_init(shm->empty, 1, message_max_size);
    sem_init(shm->full, 1, 0);


    //getpid
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

int OSMP_Finalize() {
    printf("finalize\n");
    return 0;
}

int OSMP_Size(int *size) {
    *size = shm->processAmount;
    return OSMP_SUCCESS;
}

int OSMP_Rank(int *rank) {
    int getpiD = getpid();
    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == getpiD) {
            *rank = shm->p[i].rank;
        }
    }
    return OSMP_SUCCESS;
}

int OSMP_Send() {
    pthread_mutex_lock(&shm->MUTEX);
    printf("send\n");
    pthread_mutex_unlock(&shm->MUTEX);

    return 0;
}


int OSMP_Recv() {
    pthread_mutex_lock(&shm->MUTEX);
    printf("recieve\n");
    pthread_mutex_unlock(&shm->MUTEX);
    return 0;
}

int OSMP_Bcast() {
    pthread_mutex_lock(&shm->MUTEX);
    printf("broadcast\n");
    pthread_mutex_unlock(&shm->MUTEX);
    return 0;
}
