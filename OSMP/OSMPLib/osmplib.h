
//Hier sind zusätzliche eigene Hilfsfunktionen für die interne Verwendung in der OSMP Bibliothek
//definiert.


#include <sys/wait.h>
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

#define SharedMemSize 100
#define SharedMemName "/shm"
#define OSMP_ERROR -1
#define OSMP_SUCCESS 0
#define max_messages 256
#define message_max_size 1024

typedef enum {OSMP_INT, OSMP_SHORT, OSMP_LONG, OSMP_BYTE, OSMP_UNSIGNED_CHAR, OSMP_UNSIGNED_SHORT, OSMP_UNSIGNED, OSMP_FLOAT, OSMP_DOUBLE } OSMP_Datatype;



typedef struct{
    int srcRank;
    OSMP_Datatype type;
    int msgLen;
    int nextMsg;
    char buffer[message_max_size];
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
} SharedMem;


int OSMP_Init(int *argc, char ***argv);
int OSMP_Finalize();
int OSMP_Size(int *size);
int OSMP_Rank(int *rank);
int OSMP_Send();
int OSMP_Recv();
int OSMP_Bcast();


#endif