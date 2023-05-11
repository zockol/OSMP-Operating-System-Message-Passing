
//Hier sind zusätzliche eigene Hilfsfunktionen für die interne Verwendung in der OSMP Bibliothek
//definiert.


#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>

#ifndef OSMPlib_h
#define OSMPlib_h
#define message_max_size 1024
#define max_messages 256
#define SharedMemSize 100
#define SharedMemName "/shm"
#define OSMP_ERROR -1
#define OSMP_SUCCESS 0

//mutex in shm statt bool send
//keine große unterscheidung zwischen bcast und msg

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
    size_t msgLen;
    int nextMsg;
} message;

typedef struct{
    //array/verkettete liste
    int firstEmptySlot;
    int lastEmptySlot;
} slots;

typedef struct{
    pid_t pid;
    int rank;
    int firstmsg;
    int lastmsg;
    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
} process;

typedef struct{
    int processAmount;
    message msg[max_messages];
    slots slot;
    process p[];
    Bcast broadcastMsg;
} SharedMem;

typedef enum {OSMP_INT, OSMP_SHORT, OSMP_LONG, OSMP_BYTE, OSMP_UNSIGNED_CHAR, OSMP_UNSIGNED_SHORT, OSMP_UNSIGNED, OSMP_FLOAT, OSMP_DOUBLE } OSMP_Datatype;


int OSMP_Init(int *argc, char ***argv);
int OSMP_Finalize();
int OSMP_Size(int *size);
int OSMP_Rank(int *rank);
int OSMP_Send();
int OSMP_Recv();
int OSMP_Bcast();


#endif