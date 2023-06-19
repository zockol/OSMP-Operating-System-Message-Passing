
#include "osmplib.h"

//initiales shm
SharedMem *shm;

typedef struct{
    pthread_t thread;
    char* buf;
    int* source;
    int* len;
    int count;
    OSMP_Datatype datatype;
    int dest;
    pthread_cond_t request_cond;
    pthread_mutex_t request_mutex;
    int complete; //Status der Operation 0=pending; 1=complete;
} IRequest;


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



    //definiere die eigene pid im shm
    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == 0) {
            shm->p[i].pid = getpid();
            shm->p[i].rank = i;
            rankNow = i;
            break;
        }
    }


    //wenn shm MAP_FAILED returned, gebe OSMP ERROR
    if (shm == MAP_FAILED) {
        printf("Mapping Fail: %s\n", strerror(errno));
        shm_unlink(SharedMemName);
        return OSMP_ERROR;
    }
    debug("OSMP_INIT", rankNow, NULL, "calloc");
    free(shm_stat);
    debug("OSMP_INIT", rankNow, NULL, "free");

    debug("OSMP_INIT END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;

}

//locked jeden OSMP hier, bis alle an dieser Stelle angekommen sind
int OSMP_Barrier() {
    debug("OSMP_BARRIER START", rankNow, NULL, NULL);
    //if-else konstrukt für ein flip zwischen barrier_all und barrier_all2
    if (shm->barrier_all != 0) {
        pthread_mutex_lock(&shm->mutex);
        shm->barrier_all--;

        //wenn shm->barrier_all == 0 dann lasse alle anderen unten aus der while raus.
        if (shm->barrier_all == 0) {
            shm->barrier_all2 = shm->processAmount;
            pthread_cond_broadcast(&shm->cattr);

        } else {
            while (shm->barrier_all != 0) {
                pthread_cond_wait(&shm->cattr, &shm->mutex);
            }
        }
        pthread_mutex_unlock(&shm->mutex);
    } else if (shm->barrier_all2 != 0) {
        pthread_mutex_lock(&shm->mutex);
        //genau das selbe wie dadrüber nur der flip
        shm->barrier_all2--;
        if (shm->barrier_all2 == 0) {
            shm->barrier_all = shm->processAmount;
            pthread_cond_broadcast(&shm->cattr);
        } else {
            while (shm->barrier_all2 != 0) {
                pthread_cond_wait(&shm->cattr, &shm->mutex);
            }
        }
        pthread_mutex_unlock(&shm->mutex);
    } else {
        //wenn beide Barriers 0 sind, dann error ausgeben
        debug("OSMP_BARRIER ERROR", rankNow, "BARRIER_ALL & BARRIER_ALL2 ZERO", NULL);
    }

    debug("OSMP_BARRIER END", rankNow, NULL, NULL);
    return 0;
}

//muss jeder OSMP durchlaufen bevor er sich beendet und "resetted" sich damit selber
int OSMP_Finalize() {
    debug("OSMP_FINALIZE START", rankNow, NULL, NULL);

    //wenn shm nicht initialisiert, dann error
    if (shm == NULL) {
        printf("OSMPLIB.c OSMP_FINALIZE shm not initialized");
        return OSMP_ERROR;
    }

    //kompletter reset code
    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].rank == rankNow) {
            shm->p[i].pid = -1;
            shm->p[i].rank = -1;
            shm->p[i].firstmsg = -1;

            sem_destroy(&shm->p[i].empty);
            sem_destroy(&shm->p[i].full);

            shm->processesCreated--;

            if (munmap(shm, (max_messages * shm->processAmount * sizeof(message) + shm->processAmount * sizeof(process) + sizeof(logger) + sizeof(Bcast) + sizeof(int) * 4 + sizeof(pthread_mutex_t) * 3 + sizeof(pthread_cond_t) * 2)) == OSMP_ERROR) {
                debug("OSMP_FINALIZE", rankNow, "MUNMAP == OSMP_ERROR", NULL);
            }
            if (i == (shm->processAmount - 1)) {
                shm = NULL;
            }
        }
    }
    return OSMP_SUCCESS;
}

//beschreibt den pointer size mit dem processAmount angegeben im shm struct
int OSMP_Size(int *size) {
    debug("OSMP_SIZE START", rankNow, NULL, NULL);
    *size = shm->processAmount;
    debug("OSMP_SIZE END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//beschreibt den pointer rank mit dem derzeitigen rank des prozesses
int OSMP_Rank(int *rank) {
    debug("OSMP_RANK START", rankNow, NULL, NULL);

    //ERROR wenn shm nicht initialisiert durch OSMP_INIT
    if (shm == NULL) {
        printf("shm not initialized");
        return OSMP_ERROR;
    }

    //checkt welcher rang dem laufenden prozess zugewiesen wurde in der OSMP INIT
    *rank = rankNow;
    debug("OSMP_RANK END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

//sendet eine nachricht an die gewünschte destination
int OSMP_Send(const void *buf, int count, OSMP_Datatype datatype, int dest) {
    debug("OSMP_SEND START", rankNow, NULL, NULL);
    //code um den Postkasten der destination zu beschreiben sobald in diesem Platz frei ist
    for (int i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].rank == dest) {
            sem_wait(&shm->p[i].empty);
            pthread_mutex_lock(&shm->mutex);
            shm->p[i].msg[shm->p[i].firstEmptySlot].msgLen = count * OSMP_DataSize(datatype);
            shm->p[i].msg[shm->p[i].firstEmptySlot].datatype = datatype;
            shm->p[i].msg[shm->p[i].firstEmptySlot].srcRank = rankNow;
            shm->p[i].msg[shm->p[i].firstEmptySlot].destRank = dest;
            memcpy(shm->p[i].msg[shm->p[i].firstEmptySlot].buffer, buf,shm->p[i].msg[shm->p[i].firstEmptySlot].msgLen);

            shm->p[i].firstEmptySlot++;
            shm->p[i].numberOfMessages++;
            shm->p[i].firstmsg++;

            pthread_mutex_unlock(&shm->mutex);
            sem_post(&shm->p[i].full);
        }
    }
    debug("OSMP_SEND END", rankNow, NULL, NULL);
    return 0;
}

void *isend(OSMP_Request *request){
    debug("*ISEND START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) *request;
    req->complete = 0;
    OSMP_Send(&req->buf, req->count, req->datatype, req->dest);
    req->complete = 1;
    debug("*ISEND END", rankNow, NULL, NULL);
}

int OSMP_Isend(const void *buf, int count, OSMP_Datatype datatype, int dest, OSMP_Request request){
    debug("OSMP_ISEND START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;

    memcpy(&req->buf, buf, count * OSMP_DataSize(datatype));
    req->count = count;
    req->datatype = datatype;
    req->dest = dest;
    req->source = &rankNow;

    pthread_create(&req->thread, NULL, (void * (*) (void * ))isend, &req);

    debug("OSMP_ISEND END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

int OSMP_Test(OSMP_Request request, int *flag){
    IRequest *req = (IRequest*) request;
    *flag = req->complete;
    return *flag;
}

//falls Nachrichten in dem OSMP vorhanden sind, schreibe sie in buf rein
int OSMP_Recv(void *buf, int count, OSMP_Datatype datatype, int *source, int *len) {
    debug("OSMP_RECV START", rankNow, NULL, NULL);
    int i = 0;
    for (i = 0; i < shm->processAmount; i++) {
        if (shm->p[i].pid == getpid()) {
            sem_wait(&shm->p[i].full);

            pthread_mutex_lock(&shm->mutex);

            *source = shm->p[i].msg[shm->p[i].firstmsg].srcRank;
            *len = shm->p[i].msg[shm->p[i].firstmsg].msgLen;
            memcpy(buf, shm->p[i].msg[shm->p[i].firstmsg].buffer, count * OSMP_DataSize(datatype));
            shm->p[i].firstmsg--;
            shm->p[i].firstEmptySlot--;
            pthread_mutex_unlock(&shm->mutex);
            sem_post(&shm->p[i].empty);
        }

    }

    debug("OSMP_RECV END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}


void *ircv(OSMP_Request *request){
    debug("*IRCV START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) *request;
    req->complete = false;
    OSMP_Recv(req->buf, req->count, req->datatype, req->source, req->len);
    req->complete = true;
    debug("*IRCV END", rankNow, NULL, NULL);
}


int OSMP_Irecv(void *buf, int count, OSMP_Datatype datatype, int *source, int *len, OSMP_Request request){
    debug("OSMP_IRECV START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;
    req->buf = buf;
    req->count = count;
    req->datatype = datatype;
    req->source = source;
    req->len = len;
    pthread_create(&req->thread, NULL, (void * (*) (void * ))ircv, &req);
    sleep(3);
    debug("OSMP_IRECV END", rankNow, NULL, NULL);
    return 0;
    
}

int OSMP_Wait(OSMP_Request request){
    debug("OSMP_WAIT START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) request;
    pthread_join( req->thread, NULL);
    debug("OSMP_WAIT END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}



//wenn send == true ist, schreibe *buf in den broadcast buffer und warte bis alle da sind
//wenn send == false ist, warte zuvor auf alle prozesse und schreibe dann den broadcust buffer in *buf
int OSMP_Bcast(void *buf, int count, OSMP_Datatype datatype, bool send, int *source, int *len) {
    debug("OSMP_BCAST START", rankNow, NULL, NULL);

    if (send == true) {
        //sender code
        shm->broadcastMsg.datatype = datatype;
        shm->broadcastMsg.msgLen = count * OSMP_DataSize(datatype);
        shm->broadcastMsg.srcRank = rankNow;
        memcpy(shm->broadcastMsg.buffer, buf, shm->broadcastMsg.msgLen * OSMP_DataSize(datatype));

    }
    OSMP_Barrier();
    debug("OSMP_BCAST ERROR", rankNow, "Kam ich raus?", NULL);
    if (send == false) {
        //recv code
        debug("OSMP_BCAST ERROR", rankNow, "VOR MEMCPY", NULL);
        memcpy(buf, shm->broadcastMsg.buffer, shm->broadcastMsg.msgLen);
        debug("OSMP_BCAST ERROR", rankNow, "NACH MEMCPY", NULL);
        *source = shm->broadcastMsg.srcRank;
        *len = shm->broadcastMsg.msgLen;
    }

    debug("OSMP_BCAST END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}


int OSMP_CreateRequest(OSMP_Request *request){
    debug("OSMP_CREATEREQUEST START", rankNow, NULL, NULL);

    pthread_condattr_t request_cond_attr;
    pthread_condattr_init(&request_cond_attr);
    pthread_condattr_setpshared(&request_cond_attr, PTHREAD_PROCESS_SHARED);

    pthread_mutexattr_t mutex_request_attr;
    pthread_mutexattr_init(&mutex_request_attr);
    pthread_mutexattr_setpshared(&mutex_request_attr, PTHREAD_PROCESS_SHARED);

    *request = calloc(1, sizeof(IRequest));
    IRequest *req = (IRequest*) *request;
    req->thread = 0;
    memcpy(&req->buf, "\0", 1);
    req->datatype = 0;
    req->count = 0;
    req->dest = -1;
    req->source = NULL;
    req->len = NULL;
    pthread_cond_init(&req->request_cond, &request_cond_attr);
    pthread_mutex_init(&req->request_mutex, &mutex_request_attr);
    req->complete = false;

    *request = req;

    debug("OSMP_CREATEREQUEST END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;   
}



int OSMP_RemoveRequest(OSMP_Request *request){
    debug("OSMP_REMOVEREQUEST START", rankNow, NULL, NULL);
    IRequest *req = (IRequest*) *request;

    free(req);
    debug("OSMP_REMOVEREQUEST END", rankNow, NULL, NULL);
    return OSMP_SUCCESS;
}

int OSMP_DataSize(OSMP_Datatype datatype) {
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