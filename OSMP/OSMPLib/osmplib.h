
//Hier sind zusätzliche eigene Hilfsfunktionen für die interne Verwendung in der OSMP Bibliothek
//definiert.


#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <semaphore.h>
#include <time.h>

#ifndef OSMPlib_h
#define OSMPlib_h

#define SharedMemSize 1000
#define SharedMemName "/shm"
#define OSMP_ERROR -1
#define OSMP_SUCCESS 0
#define message_max_size 1024
#define max_messages 256
#include <stdbool.h>
#include "semaphore.h"
#include "pthread.h"
#define TRUE 1
typedef enum {OSMP_INT, OSMP_SHORT, OSMP_LONG, OSMP_BYTE, OSMP_UNSIGNED_CHAR, OSMP_UNSIGNED_SHORT, OSMP_UNSIGNED, OSMP_FLOAT, OSMP_DOUBLE } OSMP_Datatype;



typedef struct {
    char buffer[message_max_size];
    int msgLen;
    int msgLenInByte;
    bool send;
    OSMP_Datatype datatype;
    int srcRank;
} Bcast;

typedef struct{
    int srcRank;
    char buffer[message_max_size];
    OSMP_Datatype datatype;
    size_t msgLen;
    int nextMsg;
} message;

typedef struct{
    //array/verkettete liste
    int firstEmptySlot;
    int lastEmptySlot;
} slots;

typedef struct{
    bool activated;
    pthread_mutex_t ready;
    sem_t full;
} send_recieve;


typedef struct{
    pid_t pid;
    int rank;
    int firstmsg;
    int lastmsg;
    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
} process;

typedef struct {
    int logIntensity;
    char logPath[256];
} logger;

typedef struct{
    int processAmount;
    message msg[max_messages];
    slots slot;
    pthread_mutex_t mutex;
    pthread_cond_t cattr;
    Bcast broadcastMsg;
    int barrier_all;
    logger log;
    process p[];
} SharedMem;


int OSMP_Init(int *argc, char ***argv);
int OSMP_Finalize();
int OSMP_Size(int *size);
int OSMP_Rank(int *rank);
int OSMP_Send();
int OSMP_Recv();
int OSMP_Bcast();
int OSMP_Barrier();


#endif