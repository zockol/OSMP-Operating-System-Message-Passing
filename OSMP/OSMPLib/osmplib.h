
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

#ifndef OSMPlib_h
#define OSMPlib_h

#define SharedMemSize 100
#define SharedMemName "/shm"
#define OSMP_ERROR -1
#define OSMP_SUCCESS 0

typedef struct{
    pid_t pid;
    int rank;
} process;

typedef struct{
    int processAmount;
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