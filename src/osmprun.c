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

int main(int argv, char* argc[]) {

    int pidAmount = atol(argc[1]);
    pid_t pid[pidAmount];

    if (pidAmount == 0) {
        printf("Bitte gebe eine korrekte Anzahl an Childs ein, die erzeugt werden sollen\n");
        exit(-1);
    }

    int i;
    pid[0] = 1;
    for (i = 0; i < pidAmount; i++) {

        pid[i] = fork();

        if (pid[i] != 0) {
            printf("Parent with ID: %d\n", pid[i]);
            waitpid(pid[i], NULL, WNOHANG);
        } else {
            execlp("./output/echoall", "echoall", "Echoall", "Test", NULL);
        }

    }
    return 0;
}