
#include "osmplib.h"

//initiales shm
SharedMem *shm;

//IRequest Struct für die Requests
typedef struct{
    pthread_t thread;
    char* buf;
    int* source;
    int* len;
    int count;
    OSMP_Datatype datatype;
    int dest;
    pthread_cond_t request_cond;
    pthread_condattr_t request_condattr;
    pthread_mutex_t request_mutex;
    pthread_mutexattr_t request_mutexattr;
    int complete; //Status der Operation 0=pending; 1=complete;
} IRequest;

size_t shm_size;

//rank des OSMP Prozesses abgespeichert, damit der Prozess intern sein Rang weiß
int rankNow = -1;
int sizeNow = -1;
pid_t pidNow = -1;

//debug methode. Schreibt in die vorher erstellte shm->log.logPath die mitgegebenen Debug Messages.
int debug(char *functionName, int srcRank, char *error, char *memory) {
    if (shm == NULL) {
        printf("shm not initialized\n");
        return OSMP_ERROR;
    }

    //wenn logIntensity noch immer bei -1 ist, ist logging disabled
    pthread_mutex_lock(&shm->log.mutex);
    if (shm->log.logIntensity == -1) {
        pthread_mutex_unlock(&shm->log.mutex);
        return OSMP_SUCCESS;
    }
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
            pthread_mutex_unlock(&shm->log.mutex);
            return OSMP_SUCCESS;
        } else {
            printf("Fehler beim öffnen der Datei\n");
            pthread_mutex_unlock(&shm->log.mutex);
            return OSMP_ERROR;
        }
    }
    pthread_mutex_unlock(&shm->log.mutex);
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
    shm_size = (size_t) shm_stat->st_size;
    shm = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);

    //wenn shm MAP_FAILED returned, gebe OSMP ERROR
    if (shm == MAP_FAILED) {
        printf("Mapping Fail: %s\n", strerror(errno));
        shm_unlink(SharedMemName);
        return OSMP_ERROR;
    }

    if (pthread_mutex_lock(&shm->mutex) != 0) {
        debug("OSMP_INIT", rankNow, "PTHREAD_MUTEX_LOCK != 0", NULL);
        return OSMP_ERROR;
    }

    sizeNow = shm->processAmount;

    if ((pidNow = getpid()) == -1) {
        debug("OSMP_INIT", rankNow, "PIDNOW == -1", NULL);
        if (pthread_mutex_lock(&shm->mutex) != 0) {
            debug("OSMP_INIT", rankNow, "PTHREAD_MUTEX_LOCK != 0", NULL);
            return OSMP_ERROR;
        }
        return OSMP_ERROR;
    };

    //definiere die eigene pid im shm
    for (int i = 0; i < sizeNow; i++) {
        if (shm->p[i].pid == 0) {
            shm->p[i].pid = pidNow;
            shm->p[i].rank = i;
            rankNow = i;

            break;
        }
    }

    if (pthread_mutex_unlock(&shm->mutex) != 0) {
        debug("OSMP_INIT", rankNow, "PTHREAD_MUTEX_unLOCK != 0", NULL);
        return OSMP_ERROR;
    }

    //debugged die callocs und frees
    debug("OSMP_INIT", rankNow, NULL, "CALLOC");
    free(shm_stat);
    debug("OSMP_INIT", rankNow, NULL, "FREE");

    debug("OSMP_INIT END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;

}

//locked jeden OSMP hier, bis alle an dieser Stelle angekommen sind
int OSMP_Barrier() {
    if (shm == NULL) {
        printf("shm not initialized\n");
        return OSMP_ERROR;
    }

    debug("OSMP_BARRIER START", rankNow, NULL, NULL);
    //if-else konstrukt für ein flip zwischen barrier_all und barrier_all2
    if (shm->barrier_all != 0) {
        if (pthread_mutex_lock(&shm->mutex) != 0) {
            debug("OSMP_BARRIER", rankNow, "(BARRIER_ALL1) PTHREAD_MUTEX_LOCK != 0", NULL);
        };
        shm->barrier_all--;
        //wenn shm->barrier_all == 0 dann lasse alle anderen unten aus der while raus.
        if (shm->barrier_all == 0) {
            shm->barrier_all2 = shm->processAmount;
            if (pthread_cond_broadcast(&shm->cattr) != 0) {
                debug("OSMP_BARRIER", rankNow, "(BARRIER_ALL1) PTHREAD_COND_BROADCAST != 0", NULL);
            };
        } else {
            while (shm->barrier_all != 0) {
                if (pthread_cond_wait(&shm->cattr, &shm->mutex) != 0) {
                    debug("OSMP_BARRIER", rankNow, "(BARRIER_ALL1) PTHREAD_COND_WAIT != 0", NULL);
                };
            }
        }
        if (pthread_mutex_unlock(&shm->mutex) != 0) {
            debug("OSMP_BARRIER", rankNow, "(BARRIER_ALL1) PTHREAD_MUTEX_UNLOCK != 0", NULL);
        };


    } else if (shm->barrier_all2 != 0) {
        if (pthread_mutex_lock(&shm->mutex) != 0) {
            debug("OSMP_BARRIER", rankNow, "(BARRIER_ALL2) PTHREAD_MUTEX_LOCK != 0", NULL);
        };
        //genau das selbe wie dadrüber nur der flip
        shm->barrier_all2--;
        if (shm->barrier_all2 == 0) {
            shm->barrier_all = shm->processAmount;
            if (pthread_cond_broadcast(&shm->cattr) != 0) {
                debug("OSMP_BARRIER", rankNow, "(BARRIER_ALL2) PTHREAD_COND_BROADCAST != 0", NULL);
            };
        } else {
            while (shm->barrier_all2 != 0) {
                if (pthread_cond_wait(&shm->cattr, &shm->mutex) != 0) {
                    debug("OSMP_BARRIER", rankNow, "(BARRIER_ALL2) PTHREAD_COND_WAIT != 0", NULL);
                };
            }
        }
        if (pthread_mutex_unlock(&shm->mutex) != 0) {
            debug("OSMP_BARRIER", rankNow, "(BARRIER_ALL2) PTHREAD_MUTEX_UNLOCK != 0", NULL);
        };
    } else {
        //wenn beide Barriers 0 sind, dann error ausgeben
        debug("OSMP_BARRIER", rankNow, "BARRIER_ALL & BARRIER_ALL2 ZERO", NULL);
        return OSMP_ERROR;
    }

    debug("OSMP_BARRIER END", rankNow, NULL, NULL);
    return 0;
}

//muss jeder OSMP durchlaufen bevor er sich beendet und "resetted" sich damit selber
int OSMP_Finalize() {
    //wenn shm nicht initialisiert, dann error
    if (shm == NULL) {
        printf("OSMPLIB.c OSMP_FINALIZE shm not initialized");
        return OSMP_ERROR;
    }

    debug("OSMP_FINALIZE START", rankNow, NULL, NULL);

    if (pthread_mutex_lock(&shm->mutex) != 0) {
        debug("OSMP_FINALIZE", rankNow, "PTHREAD_MUTEX_LOCK != 0", NULL);
        return OSMP_ERROR;
    };
    //kompletter reset code
    for (int i = 0; i < sizeNow; i++) {
        if (shm->p[i].rank == rankNow) {
            shm->p[i].pid = -1;
            shm->p[i].rank = -1;
            shm->p[i].firstmsg = -1;

            if (sem_destroy(&shm->p[i].empty) != 0) {
                if (pthread_mutex_unlock(&shm->mutex) != 0) {
                    debug("OSMP_FINALIZE", -1, "(IN FOR) PTHREAD_MUTEX_UNLOCK != 0", NULL);
                    return OSMP_ERROR;
                };
                debug("OSMP_FINALIZE", -1, "SEM_DESTROY(EMPTY) != 0", NULL);
                return OSMP_ERROR;
            };
            if (sem_destroy(&shm->p[i].full) != 0) {
                if (pthread_mutex_unlock(&shm->mutex) != 0) {
                    debug("OSMP_FINALIZE", -1, "(IN FOR) PTHREAD_MUTEX_UNLOCK != 0", NULL);
                    return OSMP_ERROR;
                };
                debug("OSMP_FINALIZE", -1, "SEM_DESTROY(FULL) != 0", NULL);
                return OSMP_ERROR;
            };

            if (pthread_mutex_unlock(&shm->mutex) != 0) {
                debug("OSMP_FINALIZE", -1, "(IN FOR) PTHREAD_MUTEX_UNLOCK != 0", NULL);
                return OSMP_ERROR;
            };
            if (munmap(shm, shm_size) == OSMP_ERROR) {
                debug("OSMP_FINALIZE", rankNow, "MUNMAP == OSMP_ERROR", NULL);
                return OSMP_ERROR;
            }
            shm = NULL;
            return OSMP_SUCCESS;
        }
    }
    if (pthread_mutex_unlock(&shm->mutex) != 0) {
        debug("OSMP_FINALIZE", -1, "(OUT OF FOR) PTHREAD_MUTEX_UNLOCK != 0", NULL);
        return OSMP_ERROR;
    };
    return OSMP_ERROR;
}

//beschreibt den pointer size mit dem processAmount angegeben im shm struct
int OSMP_Size(int *size) {
    if (shm == NULL) {
        printf("shm not initialized\n");
        return OSMP_ERROR;
    }
    debug("OSMP_SIZE START", rankNow, NULL, NULL);

    *size = sizeNow;

    debug("OSMP_SIZE END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//beschreibt den pointer rank mit dem derzeitigen rank des prozesses
int OSMP_Rank(int *rank) {
    //ERROR wenn shm nicht initialisiert durch OSMP_INIT
    if (shm == NULL) {
        printf("shm not initialized\n");
        return OSMP_ERROR;
    }

    debug("OSMP_RANK START", rankNow, NULL, NULL);
    //checkt welcher rang dem laufenden prozess zugewiesen wurde in der OSMP INIT
    *rank = rankNow;
    debug("OSMP_RANK END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//sendet eine nachricht an die gewünschte destination
int OSMP_Send(const void *buf, int count, OSMP_Datatype datatype, int dest) {
    if (shm == NULL) {
        printf("shm not initialized\n");
        return OSMP_ERROR;
    }

    debug("OSMP_SEND START", rankNow, NULL, NULL);
    //code um den Postkasten der destination zu beschreiben sobald in diesem Platz frei ist

    for (int i = 0; i < sizeNow; i++) {
        if (i == dest) {
            sem_wait(&shm->messages);
            sem_wait(&shm->p[i].empty);
            pthread_mutex_lock(&shm->mutex);
            shm->p[i].msg[shm->p[i].firstEmptySlot].msgLen = (size_t) count * OSMP_DataSize(datatype);

            if (shm->p[i].msg[shm->p[i].firstEmptySlot].msgLen > message_max_size) {
                debug("OSMP_SEND", rankNow, "MSGLEN > MESSAGE_MAX_SIZE", NULL);
                return OSMP_ERROR;
            }

            shm->p[i].msg[shm->p[i].firstEmptySlot].srcRank = rankNow;
            memcpy(shm->p[i].msg[shm->p[i].firstEmptySlot].buffer, buf,shm->p[i].msg[shm->p[i].firstEmptySlot].msgLen);
            shm->p[i].firstEmptySlot++;
            shm->p[i].firstmsg++;
            pthread_mutex_unlock(&shm->mutex);
            sem_post(&shm->p[i].full);
        }
    }
    debug("OSMP_SEND END", rankNow, NULL, NULL);
    return 0;
}

//pointer Funktion die als initiale Startfunktion für den Thread von iSend gilt
void *isend(void* request){
    if (request == NULL){
        printf("There is no request to wait for! ");
        return (void*)OSMP_ERROR;
    }
    debug("*ISEND START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;
    pthread_mutex_lock(&req->request_mutex);
    req->complete = 0;
    pthread_mutex_unlock(&req->request_mutex);

    OSMP_Send(&req->buf, req->count, req->datatype, req->dest);

    pthread_mutex_lock(&req->request_mutex);
    req->complete = 1;
    pthread_mutex_unlock(&req->request_mutex);
    debug("*ISEND END", rankNow, NULL, NULL);
    return 0;
}

//startet ein Send-Aufruf im Thread
int OSMP_Isend(const void *buf, int count, OSMP_Datatype datatype, int dest, OSMP_Request request){
    debug("OSMP_ISEND START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;

    pthread_mutex_lock(&req->request_mutex);


    if ((size_t) count * OSMP_DataSize(datatype) > message_max_size) {
        debug("OSMP_ISEND", rankNow, "MSGLEN > MESSAGE_MAX_SIZE", NULL);
        return OSMP_ERROR;
    }


    req->complete = 0;
    memcpy(&req->buf, buf, (size_t) count * OSMP_DataSize(datatype));
    req->count = count;
    req->datatype = datatype;
    req->dest = dest;
    req->source = &rankNow;



    pthread_create(&req->thread, NULL, (void * (*) (void * ))isend, request);

    pthread_mutex_unlock(&req->request_mutex);

    debug("OSMP_ISEND END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//OSMP_Test setzt die mitgegebene flag auf 0 oder 1, basierend auf dem Status des Threads (req->complete)
int OSMP_Test(OSMP_Request request, int *flag){
    if (request == NULL){
        printf("There is no request to wait for! ");
        return OSMP_ERROR;
    }
    IRequest *req = (IRequest*) request;
    pthread_mutex_lock(&req->request_mutex);
    *flag = req->complete;
    pthread_mutex_unlock(&req->request_mutex);
    return OSMP_SUCCESS;
}

//GetShmName setzt den mitgegebenen char auf das Makro SharedMemName
int OSMP_GetShmName(char** name) {
    *name = SharedMemName;
    return OSMP_SUCCESS;
}

//falls Nachrichten in dem OSMP vorhanden sind, schreibe sie in buf rein
int OSMP_Recv(void *buf, int count, OSMP_Datatype datatype, int *source, int *len) {
    debug("OSMP_RECV START", rankNow, NULL, NULL);

    sem_wait(&shm->p[rankNow].full);
    pthread_mutex_lock(&shm->mutex);
    *source = shm->p[rankNow].msg[shm->p[rankNow].firstmsg].srcRank;

    if (shm->p[rankNow].msg[shm->p[rankNow].firstEmptySlot].msgLen > message_max_size) {
        debug("OSMP_RECV", rankNow, "MSGLEN > MESSAGE_MAX_SIZE", NULL);
        return OSMP_ERROR;
    }

    *len = (int) shm->p[rankNow].msg[shm->p[rankNow].firstmsg].msgLen;


    memcpy(buf, shm->p[rankNow].msg[shm->p[rankNow].firstmsg].buffer, (size_t) count * OSMP_DataSize(datatype));
    shm->p[rankNow].firstmsg--;
    shm->p[rankNow].firstEmptySlot--;
    sem_post(&shm->p[rankNow].empty);
    sem_post(&shm->messages);
    pthread_mutex_unlock(&shm->mutex);



    debug("OSMP_RECV END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//initiale Startfunktion des Threads der iRecv funktion
void *ircv(void* request){
    if (shm == NULL) {
        printf("shm not initialized\n");
        return (void*)OSMP_ERROR;
    }

    if (request == NULL){
        printf("There is no request to wait for! ");
        return (void* )OSMP_ERROR;
    }

    debug("*IRCV START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;
    pthread_mutex_lock(&req->request_mutex);
    req->complete = 0;
    pthread_mutex_unlock(&req->request_mutex);

    OSMP_Recv(req->buf, req->count, req->datatype, req->source, req->len);

    pthread_mutex_lock(&req->request_mutex);
    req->complete = 1;
    pthread_mutex_unlock(&req->request_mutex);
    debug("*IRCV END", rankNow, NULL, NULL);
    return 0;
}

//Erstellt einen Thread der *ircv aufruft
int OSMP_Irecv(void *buf, int count, OSMP_Datatype datatype, int *source, int *len, OSMP_Request request){
    if (shm == NULL) {
        printf("shm not initialized\n");
        return OSMP_ERROR;
    }

    debug("OSMP_IRECV START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;
    pthread_mutex_lock(&req->request_mutex);
    req->complete = 0;
    req->buf = buf;
    req->count = count;
    req->datatype = datatype;
    req->source = source;
    req->len = len;

    pthread_create(&req->thread, NULL, (void * (*) (void * ))ircv, request);
    pthread_mutex_unlock(&req->request_mutex);
    debug("OSMP_IRECV END", rankNow, NULL, NULL);
    return 0;

}

//Gilt als Schranke. Ab hier wird gewartet bis der Thread durchgelaufen ist
int OSMP_Wait(OSMP_Request request){
    if (shm == NULL) {
        printf("shm not initialized\n");
        return OSMP_ERROR;
    }
    if (request == NULL){
        printf("There is no request to wait for! ");
        return OSMP_ERROR;
    }
    debug("OSMP_WAIT START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;
    pthread_mutex_lock(&req->request_mutex);
    pthread_t thread = req->thread;
    pthread_mutex_unlock(&req->request_mutex);
    pthread_join( thread, NULL);
    pthread_mutex_lock(&req->request_mutex);
    req->complete = -1;
    pthread_mutex_unlock(&req->request_mutex);


    debug("OSMP_WAIT END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}



//wenn send == true ist, schreibe *buf in den broadcast buffer und warte bis alle da sind
//wenn send == false ist, warte zuvor auf alle prozesse und schreibe dann den broadcust buffer in *buf
int OSMP_Bcast(void *buf, int count, OSMP_Datatype datatype, bool send, int *source, int *len) {
    if (shm == NULL) {
        printf("shm not initialized\n");
        return OSMP_ERROR;
    }

    debug("OSMP_BCAST START", rankNow, NULL, NULL);

    if (send == true) {
        pthread_mutex_lock(&shm->mutex);
        //sender code
        shm->broadcastMsg.msgLen = (size_t) count * OSMP_DataSize(datatype);
        shm->broadcastMsg.srcRank = rankNow;

        if (shm->broadcastMsg.msgLen > message_max_size) {
            debug("OSMP_BCAST", rankNow, "(SENDER) MSGLEN > MESSAGE_MAX_SIZE", NULL);
            return OSMP_ERROR;
        }

        memcpy(shm->broadcastMsg.buffer, buf, shm->broadcastMsg.msgLen * OSMP_DataSize(datatype));
        pthread_mutex_unlock(&shm->mutex);
    }
    OSMP_Barrier();
    //debug("OSMP_BCAST ERROR", rankNow, "Kam ich raus?", NULL);
    if (send == false) {
        pthread_mutex_lock(&shm->mutex);
        //recv code

        if (shm->broadcastMsg.msgLen > message_max_size) {
            debug("OSMP_BCAST", rankNow, "(RECEIVER) MSGLEN > MESSAGE_MAX_SIZE", NULL);
            return OSMP_ERROR;
        }

        memcpy(buf, shm->broadcastMsg.buffer, shm->broadcastMsg.msgLen);

        *source = shm->broadcastMsg.srcRank;
        *len = (int) shm->broadcastMsg.msgLen;
        pthread_mutex_unlock(&shm->mutex);
    }

    debug("OSMP_BCAST END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//Initialisierung des mitgegebenen Requests
int OSMP_CreateRequest(OSMP_Request *request){
    if (shm == NULL) {
        printf("shm not initialized\n");
        return OSMP_ERROR;
    }

    debug("OSMP_CREATEREQUEST START", rankNow, NULL, NULL);
    debug("OSMP_CREATEREQUEST", rankNow, NULL, "CALLOC");
    *request = calloc(1, sizeof(IRequest));
    IRequest *req = (IRequest*) *request;

    pthread_condattr_init(&req->request_condattr);
    pthread_condattr_setpshared(&req->request_condattr, PTHREAD_PROCESS_SHARED);

    pthread_mutexattr_init(&req->request_mutexattr);
    pthread_mutexattr_setpshared(&req->request_mutexattr, PTHREAD_PROCESS_SHARED);

    req->thread = 0;
    memcpy(&req->buf, "\0", 1);
    req->datatype = 0;
    req->count = 0;
    req->dest = -1;
    req->source = NULL;
    req->len = NULL;
    req->complete = -1;
    pthread_cond_init(&req->request_cond, &req->request_condattr);
    pthread_mutex_init(&req->request_mutex, &req->request_mutexattr);
    req->complete = false;

    *request = req;

    debug("OSMP_CREATEREQUEST END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//Freed den Speicher vom request
int OSMP_RemoveRequest(OSMP_Request *request){
    if (shm == NULL) {
        printf("shm not initialized\n");
        return OSMP_ERROR;
    }

    debug("OSMP_REMOVEREQUEST START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) *request;

    pthread_mutex_destroy(&req->request_mutex);
    pthread_cond_destroy(&req->request_cond);
    pthread_mutexattr_destroy(&req->request_mutexattr);
    pthread_condattr_destroy(&req->request_condattr);

    debug("OSMP_REMOVEREQUEST", rankNow, NULL, "FREE");
    free(req);
    debug("OSMP_REMOVEREQUEST END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//Custom Funktion um die sizeof eines Datentypens wieder zu bekommen. Basiert auf den Enum Values "OSMP_Datatype"
size_t OSMP_DataSize(OSMP_Datatype datatype) {
    if (datatype == 0) return sizeof(int);
    else if (datatype == 1) return sizeof(short);
    else if (datatype == 2) return sizeof(long);
    else if (datatype == 3) return sizeof(char);
    else if (datatype == 4) return sizeof(unsigned char);
    else if (datatype == 5) return sizeof(unsigned short);
    else if (datatype == 6) return sizeof(unsigned);
    else if (datatype == 7) return sizeof(float);
    else if (datatype == 8) return sizeof(double);
    return OSMP_SUCCESS;
}