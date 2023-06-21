
#ifndef GRP26_OSMP_H
#define GRP26_OSMP_H

#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>

#define SharedMemName "/shm"
#define OSMP_ERROR -1
#define OSMP_SUCCESS 0
#define message_max_size 1024
#define max_messages 256
#define OSMP_MAX_MESSAGES_PROC 16


typedef void* OSMP_Request;

typedef enum {
    OSMP_INT,
    OSMP_SHORT,
    OSMP_LONG,
    OSMP_BYTE,
    OSMP_UNSIGNED_CHAR,
    OSMP_UNSIGNED_SHORT,
    OSMP_UNSIGNED,
    OSMP_FLOAT,
    OSMP_DOUBLE
} OSMP_Datatype;


typedef struct {
    char buffer[message_max_size];
    size_t msgLen;
    int srcRank;
} Bcast;

typedef struct {
    int srcRank;
    char buffer[message_max_size];
    size_t msgLen;
} message;

typedef struct {
    message msg[OSMP_MAX_MESSAGES_PROC];
    pid_t pid;
    int rank;
    int firstEmptySlot;
    int firstmsg;
    sem_t empty;
    sem_t full;
} process;

typedef struct {
    int logIntensity;
    char logPath[256];
    pthread_mutex_t mutex;
} logger;

typedef struct {
    int processAmount;
    pthread_mutex_t mutex;
    pthread_cond_t cattr;
    Bcast broadcastMsg;
    int barrier_all;
    int barrier_all2;
    sem_t messages;
    logger log;
    process p[];
} SharedMem;

int OSMP_Init(int *argc, char ***argv);

int OSMP_Finalize();

int OSMP_Size(int *size);

int OSMP_Rank(int *rank);

int OSMP_Send(const void *buf, int count, OSMP_Datatype datatype, int dest);

int OSMP_Recv(void *buf, int count, OSMP_Datatype datatype, int *source, int *len);

int OSMP_Bcast(void *buf, int count, OSMP_Datatype datatype, bool send, int *source, int *len);

int OSMP_Barrier();

int OSMP_Isend(const void *buf, int count, OSMP_Datatype datatype, int dest, OSMP_Request req);

int OSMP_Irecv(void *buf, int count, OSMP_Datatype datatype, int *source, int *len, OSMP_Request req);

int OSMP_CreateRequest(OSMP_Request *request);

int OSMP_RemoveRequest(OSMP_Request *request);

int OSMP_Wait(OSMP_Request request);

int OSMP_GetShmName(char** name);

int OSMP_Test(OSMP_Request request, int *flag);

#endif //GRP26_OSMP_H
