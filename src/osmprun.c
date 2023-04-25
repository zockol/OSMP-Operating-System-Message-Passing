//
// Created by fegrue on 21.04.23.
//
//In dieser Quelltext-Datei ist die Funktionalität des OSMP-Starters implementiert
#include "OSMP.h"


int startNProcesses(int argv, char* argc[]) {

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
            execlp(argc[2], "echoall", "Echoall", "Test", NULL);
        }

    }
    return 0;
}