//
// Created by fegrue on 21.04.23.
//
//In dieser Quelltext-Datei ist die Funktionalität des OSMP-Starters implementiert
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include "../OSMP.h"


int main(int argv, char* argc[]) {

    //Exit wenn keine Argumente mit angegeben
    if (argv==1) {
        printf("Bitte gebe Argumente an. Syntax hierbei wäre ./osmprun <ChildNumber> <executable>\n");
        exit(-1);
    }

    //argc[1] wird zum Integer umgewandelt, bei falschangabe automatisch 0
    int pidAmount = atol(argc[1]);
    pid_t pid[pidAmount];

    //wenn pidAmount bei 0 unter drunter liegt wird exit(-1) ausgeführt
    if (pidAmount < 1) {
        printf("Bitte gebe eine korrekte Anzahl an Childs ein, die erzeugt werden sollen\n");
        exit(-1);
    }

    //get current working dir und schreibe es in einen Buffer mit dem letzten angegebenen Argument
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    char pathBuffer[1024];
    snprintf(pathBuffer, sizeof(pathBuffer), "../../%s", argc[argv-1]);

    //Parent und Child Trennung
    int i;
    pid[0] = 1;
    for (i = 0; i < pidAmount; i++) {

        pid[i] = fork();

        if (pid[i] != 0) {
            waitpid(pid[i], NULL, WNOHANG);
        } else {
            //Fehlerbehandlung falls Path nicht existiert.
            if (access(pathBuffer, F_OK) == 0) {
                execlp(pathBuffer, argc[argv-1], NULL);
            } else {
                printf("execlp path error\n");
                exit(-1);
            }
        }
    }
    return 0;
}