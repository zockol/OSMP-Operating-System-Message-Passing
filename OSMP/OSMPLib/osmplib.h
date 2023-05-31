
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

#ifndef OSMPlib_h
#define OSMPlib_h

#define SharedMemSize 1000
#define SharedMemName "/shm"
#define OSMP_ERROR -1
#define OSMP_SUCCESS 0
#define message_max_size 1024
#define max_messages 256
#define OSMP_MAX_MESSAGES_PROC 16

#include <stdbool.h>
#include "semaphore.h"
#include "pthread.h"

#define TRUE 1
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
    int msgLen;
    int msgLenInByte;
    bool send;
    OSMP_Datatype datatype;
    int srcRank;
} Bcast;

typedef struct {
    bool empty;
    int srcRank;
    int destRank;
    char buffer[message_max_size];
    OSMP_Datatype datatype;
    size_t msgLen;
    int nextMsg;
} message;

typedef struct {
    //array/verkettete liste
    int firstEmptySlot;
    int lastEmptySlot;
} slots;


typedef struct {
    message msg[max_messages];
    pid_t pid;
    int rank;
    slots slots;
    int firstmsg;
    int lastmsg;

    //pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
} process;

typedef struct {
    int processAmount;
    pthread_mutex_t mutex;
    pthread_cond_t cattr;
    Bcast broadcastMsg;
    int barrier_all;
    process p[];
} SharedMem;


int OSMP_Init(int *argc, char ***argv);

int OSMP_Finalize();

int OSMP_Size(int *size);

int OSMP_Rank(int *rank);

int OSMP_Send(const void *buf, int count, OSMP_Datatype datatype, int dest);

int OSMP_Recv(void *buf, int count, OSMP_Datatype datatype, int *source, int *len);

int OSMP_Bcast();

int OSMP_Barrier();

int calculateStruct(int *rank);


int getSrcRank();


#endif