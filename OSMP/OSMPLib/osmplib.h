
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

#define SharedMemSize 1000
#define SharedMemName "/shm"
#define OSMP_ERROR -1
#define OSMP_SUCCESS 0
#define message_max_size 1024
#define max_messages 256


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
    message msg[max_messages];
    int processAmount;
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