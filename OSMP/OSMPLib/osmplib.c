
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

    //mutex lock
    if (pthread_mutex_lock(&shm->mutex) != 0) {
        debug("OSMP_INIT", rankNow, "PTHREAD_MUTEX_LOCK != 0", NULL);
        return OSMP_ERROR;
    }

    //fehlerbehandlung mit getpid()
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

    //mutex unlock
    if (pthread_mutex_unlock(&shm->mutex) != 0) {
        debug("OSMP_INIT", rankNow, "PTHREAD_MUTEX_UNLOCK != 0", NULL);
        return OSMP_ERROR;
    }

    //debugged die callocs und frees
    debug("OSMP_INIT", rankNow, NULL, "CALLOC");
    free(shm_stat);
    debug("OSMP_INIT", rankNow, NULL, "FREE");

    debug("OSMP_INIT END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;

}

//beschreibt den pointer size mit dem processAmount angegeben im shm struct
int OSMP_Size(int *size) {
    if (shm == NULL) {
        printf("shm not initialized\n");
        return OSMP_ERROR;
    }
    debug("OSMP_SIZE START", rankNow, NULL, NULL);

    //schreibe size auf sizeNow;
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
            //sobald barriar_all == 0 ist werden die Prozesse freigelassen und warten auf den Mutex
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

    for (int i = 0; i < shm->p[rankNow].firstEmptySlot; i++) {
        sem_post(&shm->messages);
    }

    shm->p[rankNow].pid = -1;
    shm->p[rankNow].firstmsg = -1;
    shm->p[rankNow].firstEmptySlot = 0;

    for (int i = 0; i < OSMP_MAX_MESSAGES_PROC; i++) {
        shm->p[rankNow].msg[i].srcRank = -1;
        shm->p[rankNow].msg[i].msgLen = 0;
        memset(shm->p[rankNow].msg[i].buffer, '\0', sizeof(shm->p[rankNow].msg[i].buffer));
    }

    if (sem_destroy(&shm->p[rankNow].empty) != 0) {
        if (pthread_mutex_unlock(&shm->mutex) != 0) {
            debug("OSMP_FINALIZE", -1, "(IN FOR) PTHREAD_MUTEX_UNLOCK != 0", NULL);
            return OSMP_ERROR;
        };
        debug("OSMP_FINALIZE", -1, "SEM_DESTROY(EMPTY) != 0", NULL);
        return OSMP_ERROR;
    };
    if (sem_destroy(&shm->p[rankNow].full) != 0) {
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

            //wartet zuerst auf die maximale nachrichten Semaphore und daraufhin das sem_wait(empty) minimum 1 hat um fortzufahren
            if (sem_wait(&shm->messages) == -1) {
                debug("OSMP_SEND", rankNow, "SEM_WAIT MESSAGES == -1", NULL);
                return OSMP_ERROR;
            };
            if (sem_wait(&shm->p[i].empty) == -1) {
                debug("OSMP_SEND", rankNow, "SEM_WAIT EMPTY == -1", NULL);
                return OSMP_ERROR;
            };

            //mutex lock
            if (pthread_mutex_lock(&shm->mutex) != 0) {
                debug("OSMP_SEND", rankNow, "PTHREAD_MUTEX_LOCK != NULL", NULL);
                return OSMP_ERROR;
            };

            shm->p[i].msg[shm->p[i].firstEmptySlot].msgLen = (size_t) count * OSMP_DataSize(datatype);

            //wenn die tatsächliche Nachrichtenlänge größer als erlaubt ist => Fehlerbehandlung
            if (shm->p[i].msg[shm->p[i].firstEmptySlot].msgLen > OSMP_MAX_PAYLOAD_LENGTH) {
                debug("OSMP_SEND", rankNow, "MSGLEN > OSMP_MAX_PAYLOAD_LENGTH", NULL);
                if (pthread_mutex_unlock(&shm->mutex) != 0) {
                    debug("OSMP_SEND", rankNow, "PTHREAD_MUTEX_UNLOCK != NULL AFTER MSGLEN", NULL);
                    return OSMP_ERROR;
                };
                return OSMP_ERROR;
            }

            shm->p[i].msg[shm->p[i].firstEmptySlot].srcRank = rankNow;
            memcpy(shm->p[i].msg[shm->p[i].firstEmptySlot].buffer, buf,shm->p[i].msg[shm->p[i].firstEmptySlot].msgLen);
            shm->p[i].firstEmptySlot++;
            shm->p[i].firstmsg++;

            //mutex unlock
            if (pthread_mutex_unlock(&shm->mutex) != 0) {
                debug("OSMP_SEND", rankNow, "PTHREAD_MUTEX_UNLOCK != NULL", NULL);
                return OSMP_ERROR;
            };

            //Zähle die Full Semaphore eins hoch, so dass Recv gestartet werden kann
            if (sem_post(&shm->p[i].full) == -1) {
                debug("OSMP_SEND", rankNow, "SEM_POST(FULL) == -1", NULL);
                return OSMP_ERROR;
            }
        }
    }
    debug("OSMP_SEND END", rankNow, NULL, NULL);
    return 0;
}

//pointer Funktion die als initiale Startfunktion für den Thread von iSend gilt
void *isend(void* request){
    debug("*ISEND START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;

    //mutex lock
    if (pthread_mutex_lock(&req->request_mutex) != 0) {
        debug("*ISEND", rankNow, "PTHREAD_MUTEX_LOCK != 0", NULL);
        return (void*)OSMP_ERROR;
    }
    //complete auf start (0) setzen
    req->complete = 0;

    //Die Aufgabe des Threads ausführen
    if (OSMP_Send(&req->buf, req->count, req->datatype, req->dest) != OSMP_SUCCESS) {
        debug("*ISEND", rankNow, "OSMP_SEND != OSMP_SUCCESS", NULL);
        req->complete = -1;
        if (pthread_mutex_unlock(&req->request_mutex) != 0) {
            debug("*ISEND", rankNow, "(IN OSMP_SEND ERROR) PTHREAD_MUTEX_UNLOCK != 0", NULL);
        }
        return (void*)OSMP_ERROR;
    };

    //complete auf fertig setzen (1)
    req->complete = 1;
    if (pthread_mutex_unlock(&req->request_mutex) != 0) {
        debug("*ISEND", rankNow, "PTHREAD_MUTEX_unLOCK != 0", NULL);
        return (void*)OSMP_ERROR;
    }
    debug("*ISEND END", rankNow, NULL, NULL);
    return (void*)OSMP_ERROR;
}

//startet ein Send-Aufruf im Thread
int OSMP_Isend(const void *buf, int count, OSMP_Datatype datatype, int dest, OSMP_Request request){
    debug("OSMP_ISEND START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;

    if (pthread_mutex_lock(&req->request_mutex) != 0) {
        debug("OSMP_ISEND", rankNow, "PTHREAD_MUTEX_LOCK != 0", NULL);
        return OSMP_ERROR;
    }

    //überprüfe ob die tatsächliche Länge größer als erlaubt ist, ansonsten Fehlerbehandlung
    if ((size_t) count * OSMP_DataSize(datatype) > OSMP_MAX_PAYLOAD_LENGTH) {
        debug("OSMP_ISEND", rankNow, "MSGLEN > OSMP_MAX_PAYLOAD_LENGTH", NULL);
        if (pthread_mutex_unlock(&req->request_mutex) != 0) {
            debug ("OSMP_ISEND", rankNow, "(AFTER MSGLEN ERROR) PTHREAD_MUTEX_UNLOCK != 0", NULL);
        }
        return OSMP_ERROR;
    }

    //wenn req-thread > 0 ist, existiert bereits ein Thread. Breche ab
    if(req->thread >0){
        debug("OSMP_SEND", rankNow, "THREAD IS ALREADY EXISTING, CANNOT CREATE A NEW ONE", NULL);
        if (pthread_mutex_unlock(&req->request_mutex) != 0) {
            debug ("OSMP_ISEND", rankNow, "(AFTER THREAD ERROR) PTHREAD_MUTEX_UNLOCK != 0", NULL);
        }
        return OSMP_ERROR;
    }

    //beschreiben der request
    req->complete = 0;
    memcpy(&req->buf, buf, (size_t) count * OSMP_DataSize(datatype));
    req->count = count;
    req->datatype = datatype;
    req->dest = dest;
    req->source = &rankNow;

    //erstellen des Prozesses
    if (pthread_create(&req->thread, NULL, (void * (*) (void * ))isend, request) != 0) {
        debug("ISEND", rankNow, "PTHREAD_CREATE != 0", NULL);
        if (pthread_mutex_unlock(&req->request_mutex) != 0) {
            debug ("OSMP_ISEND", rankNow, "(AFTER PTHREAD_CREATE ERROR) PTHREAD_MUTEX_UNLOCK != 0", NULL);
        }
        return OSMP_ERROR;
    };

    //Mutex unlocken
    if (pthread_mutex_unlock(&req->request_mutex) != 0) {
        debug ("OSMP_ISEND", rankNow, "PTHREAD_MUTEX_UNLOCK != 0", NULL);
        return OSMP_ERROR;
    }

    debug("OSMP_ISEND END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//OSMP_Test setzt die mitgegebene flag auf 0 oder 1, basierend auf dem Status des Threads (req->complete)
int OSMP_Test(OSMP_Request request, int *flag){
    debug("OSMP_TEST START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;
    //mutex lock
    if (pthread_mutex_lock(&req->request_mutex) != 0) {
        debug("OSMP_TEST", rankNow, "PTHREAD_MUTEX_LOCK != 0", NULL);
        return OSMP_ERROR;
    }
    //setze die flag auf req->complete
    *flag = req->complete;
    //mutex unlock
    if (pthread_mutex_unlock(&req->request_mutex) != 0) {
        debug("OSMP_TEST", rankNow, "PTHREAD_MUTEX_UNLOCK != 0", NULL);
        return OSMP_ERROR;
    }
    debug("OSMP_TEST END", rankNow, NULL, NULL);
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

    //wenn sem_wait(full) bei 1 ist, gehe weiter
    if (sem_wait(&shm->p[rankNow].full) != 0) {
        debug("OSMP_RECV", rankNow, "SEM_WAIT(FULL) != 0", NULL);
        return OSMP_ERROR;
    };
    //locke mutex
    if (pthread_mutex_lock(&shm->mutex) != 0) {
        debug("OSMP_RECV", rankNow, "PTHREAD_MUTEX_LOCK != 0", NULL);
        return OSMP_ERROR;
    };

    *source = shm->p[rankNow].msg[shm->p[rankNow].firstmsg].srcRank;

    //wenn msg.len größer als Zulässig, breche ab
    if (shm->p[rankNow].msg[shm->p[rankNow].firstEmptySlot].msgLen > OSMP_MAX_PAYLOAD_LENGTH) {
        debug("OSMP_RECV", rankNow, "MSGLEN > OSMP_MAX_PAYLOAD_LENGTH", NULL);
        if (pthread_mutex_unlock(&shm->mutex) != 0) {
            debug("OSMP_RECV", rankNow, "(IN MSGLEN ERROR) PTHREAD_MUTEX_UNLOCK != 0", NULL);
        };
        return OSMP_ERROR;
    }

    *len = (int) shm->p[rankNow].msg[shm->p[rankNow].firstmsg].msgLen;

    memcpy(buf, shm->p[rankNow].msg[shm->p[rankNow].firstmsg].buffer, (size_t) count * OSMP_DataSize(datatype));
    shm->p[rankNow].firstmsg--;
    shm->p[rankNow].firstEmptySlot--;

    //Zähle die Semaphore Empty eins hoch, so dass der Sender weiß, dass wieder ein Platz frei ist.
    if (sem_post(&shm->p[rankNow].empty) != 0) {
        debug("OSMP_RECV", rankNow, "SEM_POST(EMPTY) != 0", NULL);
        return OSMP_ERROR;
    };

    //Zähle die Maximal erlaubte Messages Semaphore eins Hoch
    if (sem_post(&shm->messages) != 0) {
        debug("OSMP_RECV", rankNow, "SEM_POST(MESSAGES) != 0", NULL);
        return OSMP_ERROR;
    };

    //mutex unlock
    if (pthread_mutex_unlock(&shm->mutex) != 0) {
        debug("OSMP_RECV", rankNow, "PTHREAD_MUTEX_UNLOCK != 0", NULL);
        return OSMP_ERROR;
    };

    debug("OSMP_RECV END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//initiale Startfunktion des Threads der iRecv funktion
void *ircv(void* request){
    debug("*IRCV START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;
    if (pthread_mutex_lock(&req->request_mutex) != 0) {
        debug("*IRCV", rankNow, "PTHREAD_MUTEX_LOCK != NULL", NULL);
        return NULL;
    }

    //req->complete auf start setzen (0)
    req->complete = 0;

    //start der ThreadAufgabe
    if (OSMP_Recv(req->buf, req->count, req->datatype, req->source, req->len) != OSMP_SUCCESS) {
        debug("*IRCV", rankNow, "OSMP_RECV != OSMP_SUCCESS", NULL);
        req->complete = -1;
        if (pthread_mutex_unlock(&req->request_mutex) != 0) {
            debug("*IRCV", rankNow, "(IN OSMP_RECV ERROR) PTHREAD_MUTEX_UNLOCK != NULL", NULL);
        }
        return NULL;
    };

    //req->complete auf finish setzen (1)
    req->complete = 1;
    if (pthread_mutex_unlock(&req->request_mutex) != 0) {
        debug("*IRCV", rankNow, "PTHREAD_MUTEX_UNLOCK != NULL", NULL);
        return NULL;
    }
    debug("*IRCV END", rankNow, NULL, NULL);
    return NULL;
}

//Erstellt einen Thread der *ircv aufruft
int OSMP_Irecv(void *buf, int count, OSMP_Datatype datatype, int *source, int *len, OSMP_Request request){
    debug("OSMP_IRECV START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;

    //Mutex Lock
    if (pthread_mutex_lock(&req->request_mutex) != 0) {
        debug("OSMP_IRECV", rankNow, "PTHREAD_MUTEX_LOCK != 0", NULL);
        return OSMP_ERROR;
    };

    //wenn req->Thread > 0, existiert ein Thread bereits in diesem Request. Breche ab
    if(req->thread >0){
        debug("OSMP_SEND", rankNow, "THREAD IS ALREADY EXISTING, CANNOT CREATE A NEW ONE", NULL);
        if (pthread_mutex_unlock(&req->request_mutex) != 0) {
            debug("OSMP_IRECV", rankNow, "(REQUEST THREAD > 0) PTHREAD_MUTEX_UNLOCK != 0", NULL);
        };
        return OSMP_ERROR;
    }

    req->complete = 0;
    req->buf = buf;
    req->count = count;
    req->datatype = datatype;
    req->source = source;
    req->len = len;

    //Thread wird erstellt
    if (pthread_create(&req->thread, NULL, (void * (*) (void * ))ircv, request) != 0) {
        debug("IRECV", rankNow, "PTHREAD_CREATE != 0", NULL);
        if (pthread_mutex_unlock(&req->request_mutex) != 0) {
            debug("OSMP_IRECV", rankNow, "PTHREAD_MUTEX_UNLOCK != 0", NULL);
        }
        return OSMP_ERROR;
    };

    //Mutex Unlock
    if (pthread_mutex_unlock(&req->request_mutex) != 0) {
        debug("OSMP_IRECV", rankNow, "PTHREAD_MUTEX_UNLOCK != 0", NULL);
        return OSMP_ERROR;
    };
    debug("OSMP_IRECV END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;

}

//Gilt als Schranke. Ab hier wird gewartet bis der Thread durchgelaufen ist
int OSMP_Wait(OSMP_Request request){
    debug("OSMP_WAIT START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;

    //Mutex Lock
    if (pthread_mutex_lock(&req->request_mutex) != 0) {
        debug("OSMP_Wait", rankNow, "(1) PTHREAD_MUTEX_LOCK != 0", NULL);
        return OSMP_ERROR;
    };

    //setze complete wieder auf reset
    if (req->thread<=0){
        req->complete = -1;
    }
    if (pthread_mutex_unlock(&req->request_mutex) != 0) {
        debug("OSMP_Wait", rankNow, "(1) PTHREAD_MUTEX_UNLOCK != 0", NULL);
        return OSMP_ERROR;
    };
    if (pthread_join( req->thread, NULL) != 0) {
        debug("OSMP_WAIT", rankNow, "PTHREAD_JOIN != 0", NULL);
        return OSMP_ERROR;
    };
    if (pthread_mutex_lock(&req->request_mutex) != 0) {
        debug("OSMP_Wait", rankNow, "(2) PTHREAD_MUTEX_LOCK != 0", NULL);
        return OSMP_ERROR;
    };
    req->complete = -1;
    if (pthread_mutex_unlock(&req->request_mutex) != 0) {
        debug("OSMP_Wait", rankNow, "(2) PTHREAD_MUTEX_UNLOCK != 0", NULL);
        return OSMP_ERROR;
    };

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
        if (pthread_mutex_lock(&shm->mutex) != 0) {
            debug("OSMP_BCAST", rankNow, "(SENDER) PTHREAD_MUTEX_LOCK != 0", NULL);
            return OSMP_ERROR;
        };
        //sender code
        shm->broadcastMsg.msgLen = (size_t) count * OSMP_DataSize(datatype);
        shm->broadcastMsg.srcRank = rankNow;

        if (shm->broadcastMsg.msgLen > OSMP_MAX_PAYLOAD_LENGTH) {
            debug("OSMP_BCAST", rankNow, "(SENDER) MSGLEN > OSMP_MAX_PAYLOAD_LENGTH", NULL);
            if (pthread_mutex_lock(&shm->mutex) != 0) {
                debug("OSMP_BCAST", rankNow, "(SENDER | IN MSGLEN ERROR) PTHREAD_MUTEX_LOCK != 0", NULL);
            };
            return OSMP_ERROR;
        }

        memcpy(shm->broadcastMsg.buffer, buf, shm->broadcastMsg.msgLen * OSMP_DataSize(datatype));
        if (pthread_mutex_unlock(&shm->mutex) != 0) {
            debug("OSMP_BCAST", rankNow, "(SENDER) PTHREAD_MUTEX_UNLOCK != 0", NULL);
            return OSMP_ERROR;
        };
    }
    //Setze Barrier und Staue alle Prozesse hier an
    if (OSMP_Barrier() != OSMP_SUCCESS) {
        debug("OSMP_BCAST", rankNow, "OSMP_BARRIER != OSMP_SUCCESS", NULL);
        return OSMP_ERROR;
    };

    if (send == false) {
        if (pthread_mutex_lock(&shm->mutex) != 0) {
            debug("OSMP_BCAST", rankNow, "(RECEIVER) PTHREAD_MUTEX_LOCK != 0", NULL);
            if (pthread_mutex_lock(&shm->mutex) != 0) {
                debug("OSMP_BCAST", rankNow, "(RECEIVER | IN MSGLEN ERROR) PTHREAD_MUTEX_LOCK != 0", NULL);
            };
            return OSMP_ERROR;
        };
        //recv code

        if (shm->broadcastMsg.msgLen > OSMP_MAX_PAYLOAD_LENGTH) {
            debug("OSMP_BCAST", rankNow, "(RECEIVER) MSGLEN > OSMP_MAX_PAYLOAD_LENGTH", NULL);
            return OSMP_ERROR;
        }

        memcpy(buf, shm->broadcastMsg.buffer, shm->broadcastMsg.msgLen);

        *source = shm->broadcastMsg.srcRank;
        *len = (int) shm->broadcastMsg.msgLen;
        if (pthread_mutex_unlock(&shm->mutex) != 0) {
            debug("OSMP_BCAST", rankNow, "(SENDER) PTHREAD_MUTEX_UNLOCK != 0", NULL);
            return OSMP_ERROR;
        };
    }

    debug("OSMP_BCAST END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//Initialisierung des mitgegebenen Requests
int OSMP_CreateRequest(OSMP_Request *request){
    debug("OSMP_CREATEREQUEST START", rankNow, NULL, NULL);
    debug("OSMP_CREATEREQUEST", rankNow, NULL, "CALLOC");
    *request = calloc(1, sizeof(IRequest));
    IRequest *req = (IRequest*) *request;

    if (pthread_condattr_init(&req->request_condattr) != 0) {
        debug("OSMP_CREATEREQUEST", rankNow, "PTHREAD_CONDATTR_INIT != 0", NULL);
        return OSMP_ERROR;
    };
    if (pthread_condattr_setpshared(&req->request_condattr, PTHREAD_PROCESS_SHARED) != 0) {
        debug("OSMP_CREATEREQUEST", rankNow, "PTHREAD_CONDATTR_SETPSHARED != 0", NULL);
        return OSMP_ERROR;
    };

    if (pthread_mutexattr_init(&req->request_mutexattr) != 0) {
        debug("OSMP_CREATEREQUEST", rankNow, "PTHREAD_MUTEXATTR_INIT != 0", NULL);
        return OSMP_ERROR;
    };

    if (pthread_mutexattr_setpshared(&req->request_mutexattr, PTHREAD_PROCESS_SHARED) != 0) {
        debug("OSMP_CREATEREQUEST", rankNow, "PTHREAD_MUTEXATTR_SETPSHARED != 0", NULL);
        return OSMP_ERROR;
    };

    req->thread = 0;
    memcpy(&req->buf, "\0", 1);
    req->datatype = 0;
    req->count = 0;
    req->dest = -1;
    req->source = NULL;
    req->len = NULL;
    req->complete = -1;
    if (pthread_cond_init(&req->request_cond, &req->request_condattr) != 0) {
        debug("OSMP_CREATEREQUEST", rankNow, "PTHREAD_COND_INIT != 0", NULL);

        return OSMP_ERROR;
    };
    if (pthread_mutex_init(&req->request_mutex, &req->request_mutexattr) != 0) {
        debug("OSMP_CREATEREQUEST", rankNow, "PTHREAD_MUTEX_INIT != 0", NULL);
        return OSMP_ERROR;
    };

    req->complete = false;

    *request = req;

    debug("OSMP_CREATEREQUEST END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;   
}

//Freed den Speicher vom request
int OSMP_RemoveRequest(OSMP_Request *request){
    debug("OSMP_REMOVEREQUEST START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) *request;

    if (pthread_mutex_destroy(&req->request_mutex) != 0) {
        debug("OSMP_REMOVEREQUEST", rankNow, "PTHREAD_MUTEX_DESTROY != 0", NULL);
        return OSMP_ERROR;
    };
    if (pthread_cond_destroy(&req->request_cond) != 0) {
        debug("OSMP_REMOVEREQUEST", rankNow, "PTHREAD_COND_DESTROY != 0", NULL);
        return OSMP_ERROR;
    };
    if (pthread_mutexattr_destroy(&req->request_mutexattr) != 0) {
        debug("OSMP_REMOVEREQUEST", rankNow, "PTHREAD_MUTEXATTR_DESTROY != 0", NULL);
        return OSMP_ERROR;
    };
    if (pthread_condattr_destroy(&req->request_condattr) != 0) {
        debug("OSMP_REMOVEREQUEST", rankNow, "PTHREAD_CONDATTR_DESTROY != 0", NULL);
        return OSMP_ERROR;
    };

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