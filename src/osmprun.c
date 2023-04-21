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

void forker(int nprocesses, pid_t pid[])
{
    pid[nprocesses];

    if(nprocesses > 0)
    {
        if ((pid[nprocesses] = fork()) < 0)
        {
            perror("fork");
        }
        else if (pid[nprocesses] == 0)
        {
            //Child stuff here
            printf("Child %d end\n", nprocesses);
            exit(1);
        }
        else if(pid > 0)
        {
            waitpid(pid[nprocesses], NULL, WNOHANG);
            //parent
            forker(nprocesses - 1, pid);
        }
    }
}

int main(int argv, char* argc[]) {

    int i = atol(argc[1]);
    pid_t pid[i];
    forker(i, pid);
    for(int j = 0 ; j <= i ; j++){
        waitpid(pid[j], NULL, WNOHANG);
    }
    printf("Elternprozess");


}