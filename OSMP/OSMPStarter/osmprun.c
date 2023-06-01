//
// Created by fegrue on 21.04.23.
//
//In dieser Quelltext-Datei ist die Funktionalität des OSMP-Starters implementiert

#include "./osmprun.h"

SharedMem *shm;


int shm_create(int pidAmount) {


    shm->processAmount = 0;
    shm->processesCreated = 0;

    for (int i = 0; i < pidAmount; i++) {
        shm->p[i].pid = 0;
        shm->p[i].rank = i;
        shm->p[i].firstmsg = -1;
        shm->p[i].numberOfMessages = 0;
        shm->p[i].slots.firstEmptySlot = 0;

        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&shm->mutex, &mutex_attr);

        pthread_mutexattr_t mutex_attr2;
        pthread_mutexattr_init(&mutex_attr2);
        pthread_mutexattr_setpshared(&mutex_attr2, PTHREAD_PROCESS_SHARED);

        // pthread_mutex_init(&shm->p[i].msg[k].send, &mutex_attr2);


        pthread_condattr_t barrier;
        pthread_condattr_init(&barrier);
        pthread_condattr_setpshared(&barrier, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&shm->cattr, &barrier);


        pthread_condattr_t create;
        pthread_condattr_init(&create);
        pthread_condattr_setpshared(&create, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&shm->allCreated, &create);


        sem_init(&shm->p[i].empty, 1, OSMP_MAX_MESSAGES_PROC);
        sem_init(&shm->p[i].full, 1, 0);


        for (int j = 0; j < OSMP_MAX_MESSAGES_PROC; j++) {
            shm->p[i].msg[j].srcRank = -1;
            if (i == OSMP_MAX_MESSAGES_PROC - 1) {
                shm->p[i].msg[j].nextMsg = -1;
            } else {
                shm->p[i].msg[j].nextMsg = i + 1;
            }
            shm->p[i].msg[j].msgLen = 0;
            memcpy(shm->p[i].msg[j].buffer, "\0", 1);

            shm->p[i].msg[j].full = false;


            shm->p[i].msg[j].nextMsg = 0;
            pthread_condattr_t read;
            pthread_condattr_init(&read);
            pthread_condattr_setpshared(&read, PTHREAD_PROCESS_SHARED);
            pthread_cond_init(&shm->p[i].msg[j].read, &read);
        }
    }

    return OSMP_SUCCESS;
}

int start_shm(int pidAmount) {

    size_t sizeOfSharedMem = (pidAmount * sizeof(slots) + max_messages * pidAmount * sizeof(message) +
                              pidAmount * sizeof(process) +
                              sizeof(Bcast));

    int fileDescriptor = shm_open(SharedMemName, O_CREAT | O_RDWR, 0640);

    if (fileDescriptor == -1) {
        return -1;
    }

    int ftrunc = ftruncate(fileDescriptor, sizeOfSharedMem);

    if (ftrunc == -1) {
        printf("Fehler bei ftruncate %s\n", strerror(errno));
        return -1;
    }


    shm = mmap(NULL, sizeOfSharedMem, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);

    if (shm == MAP_FAILED) {
        printf("Mapping Fail: %s\n", strerror(errno));
        shm_unlink(SharedMemName);
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[]) {


    //Exit wenn keine Argumente mit angegeben
    if (argc == 1) {
        printf("Bitte gebe Argumente an. Syntax hierbei wäre ./osmprun <ChildNumber> <executable>\n");
        exit(-1);
    }

    //argc[1] wird zum Integer umgewandelt, bei falschangabe automatisch 0
    int pidAmount = atol(argv[1]);
    pid_t pid;

    //wenn pidAmount bei 0 unter drunter liegt wird OSMP_ERROR returned
    if (pidAmount < 1) {
        printf("Bitte gebe eine korrekte Anzahl an Childs ein, die erzeugt werden sollen\n");
        return -1;
    }


    start_shm(pidAmount);


    shm_create(pidAmount);


    shm->processAmount = pidAmount;
    shm->barrier_all = pidAmount;


    //Parent und Child Trennung
    int i;
    for (i = 0; i < pidAmount; i++) {
        pid = fork();


        if (pid < 0) {
            printf("Fehler beim forken\n");
            shm_unlink(SharedMemName);
            return -1;
        } else if (pid == 0) {

            sleep(2);
            if (argc > 2) {

                int a = execlp(argv[2], "osmpexecutable", NULL);
                if (a == -1) {
                    printf("execlp failure\n");
                    return -1;
                }
            } else {

                int a = execlp("./OSMP/OSMPExecutable/osmpexecutable", "osmpexecutable", NULL);
                if (a == -1) {
                    printf("execlp failure\n");
                    return -1;
                }
            }

            shm_unlink(SharedMemName);
            return 0;
        } else if (pid > 0) {
            sleep(1);
        }
    }
    if (shm->processesCreated = shm->processAmount) {
        pthread_cond_broadcast(&shm->allCreated);
    }
    for (int i = 0; i < pidAmount; i++) {
        waitpid(-1, NULL, 0);
    }

    return 0;
}