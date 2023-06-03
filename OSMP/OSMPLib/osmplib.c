
#include "osmplib.h"

//initiales shm
SharedMem *shm;

//rank des OSMP Prozesses abgespeichert, damit der Prozess intern sein Rang weiß
int rankNow = 0;

//debug methode. Schreibt in die vorher erstellte shm->log.logPath die mitgegebenen Debug Messages.
int debug(char *functionName, int srcRank, char *error, char *memory) {

    //wenn logIntensity noch immer bei -1 ist, ist logging disabled
    if (shm->log.logIntensity == -1) return OSMP_SUCCESS;

    //buffer für die debug message
    char buffer[1024];

    //timestamp wann die funktion aufgerufen wurde
    int timestamp = (int) time(NULL);

    //wenn error und memory zeitgleich nicht NULL sind, ist die Funktion falsch aufgerufen worden.
    //syntax; entweder kein error/memory, nur memory oder nur error
    if (error != NULL && memory != NULL) {
        sprintf(buffer, "Timestamp: %d, Error: MEMORY AND DEBUG != NULL IN debug(), Funktion: %s, OSMPRank: %d\n",
                timestamp, functionName, srcRank);

    //wenn syntax korrekt, dann überprüfe das logIntensitylevel, error/memory oder nichts und beschreibe entsprechend den Buffer
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

    //wenn im Buffer das Wort "Timestamp" gefunden wurde
    result = strstr(buffer, "Timestamp");

    //wenn Wort gefunden wurde, öffne schreibe den Buffer in die Datei als append
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

//OSMP_Init initialisiert alle OSMP_Prozesse. MUSS ZWINGEND DIE ERSTE FUNKTION SEIN EINES OSMP PROZESSES
int OSMP_Init(int *argc, char ***argv) {

    //öffnet den FD
    int fileDescriptor = shm_open(SharedMemName, O_CREAT | O_RDWR, 0640);

    if (fileDescriptor == -1) {
        printf("Error beim shm_open\n");
        return OSMP_ERROR;
    }

    //nimmt die stats der derzeitigen datei
    struct stat *shm_stat = calloc(1, sizeof(struct stat));

    //wenn stats nicht funktionieren, error ausgeben
    if (shm_stat == NULL) {
        printf("Error beim shm_stat\n");
        return OSMP_ERROR;
    }

    //wenn fstat error, dann breche aufruf ab.
    if (fstat(fileDescriptor, shm_stat) != 0) {
        printf("Error beim fstat\n");
        return OSMP_ERROR;
    }

    //baue die variable mit der größe der datei und mmape die shm
    size_t shm_size = (size_t) shm_stat->st_size;
    shm = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);

    //wenn map gefailed ist, error ausgeben
    if (shm == MAP_FAILED) {
        printf("Error beim mmap\n");
        return OSMP_ERROR;
    }

    //debug den calloc und free
    debug("OSMP_INIT", rankNow, NULL, "calloc");
    free(shm_stat);
    debug("OSMP_INIT", rankNow, NULL, "free");

    //definiere die eigene pid im shm
    int i = 0, breaker = 0;
    for (i = 0; i < shm->processAmount; i++) {
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


    if (shm->processesCreated = shm->processAmount) {
        pthread_cond_broadcast(&shm->allCreated);
    }
    debug("OSMP_INIT END", rankNow, NULL, NULL);


    return OSMP_SUCCESS;

}


int OSMP_Barrier() {

    debug("OSMP_BARRIER START", rankNow, NULL, NULL);

    pthread_mutex_lock(&shm->mutex);

    if (shm->barrier_all != 0) {
        shm->barrier_all--;

        if (shm->barrier_all == 0) {
            shm->barrier_all2 = shm->processAmount;
            pthread_cond_broadcast(&shm->cattr);


        } else {

            while (shm->barrier_all != 0) {
                pthread_cond_wait(&shm->cattr, &shm->mutex);
            }
        }
    } else if (shm->barrier_all2 != 0) {
        shm->barrier_all2--;
        if (shm->barrier_all2 == 0) {
            shm->barrier_all = shm->processAmount;
            pthread_cond_broadcast(&shm->cattr);


        } else {
            while (shm->barrier_all2 != 0) {
                pthread_cond_wait(&shm->cattr, &shm->mutex);
            }
        }

    } else {
        debug("OSMP_BARRIER ERROR", rankNow, "BARRIER_ALL & BARRIER_ALL2 ZERO", NULL);
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

            memcpy(shm->p[i].msg[shm->p[i].slots.firstEmptySlot].buffer, buf,
                   shm->p[i].msg[shm->p[i].slots.firstEmptySlot].msgLen);
            shm->p[i].msg[shm->p[i].slots.firstEmptySlot].full = true;
            shm->p[i].slots.firstEmptySlot++;
            shm->p[i].numberOfMessages++;
            shm->p[i].firstmsg++;

            //printf("firstmessage: %d | firstempty: %d\n" , shm->p[i].firstmsg, shm->p[i].slots.firstEmptySlot);

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
