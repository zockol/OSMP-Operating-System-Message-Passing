//
// Created by fegrue on 21.04.23.
//
#include <stdio.h>
#include "stdlib.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include "osmprun.h"


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

void forker(int nprocesses, pid_t pid[])
{
    pid[nprocesses];

    if(nprocesses > 0)
    {
        if ((pid[nprocesses] = fork()) < 0)
        {
            //Fehlerbehandlung
            perror("fork");
        }
        else if (pid[nprocesses] == 0)
        {
            //Child stuff here
            printf("Kind no %d", nprocesses);
            execlp("./output/echoall.o", "echoall", "Echoall" , "Test", NULL);
        }
        else if(pid[nprocesses] > 0)
        {
            //parent
            waitpid(pid[nprocesses], NULL, WNOHANG);
            forker(nprocesses - 1, pid);

        }
    }else{
        exit(-1);
    }

}

int main(int argv, char* argc[]) {

    int i = atol(argc[1]);
    pid_t pid[i];
    forker(i, pid);
}