//
// Created by fegrue on 21.04.23.
//
//In dieser Quelltext-Datei ist die Funktionalität des OSMP-Starters implementiert

#include "../OSMP.h"



int main(int argv, char* argc[]) {


    //Exit wenn keine Argumente mit angegeben
    if (argv==1) {
        printf("Bitte gebe Argumente an. Syntax hierbei wäre ./osmprun <ChildNumber> <executable>\n");
        exit(-1);
    }

    //argc[1] wird zum Integer umgewandelt, bei falschangabe automatisch 0
    int pidAmount = atol(argc[1]);
    pid_t pid;

    //wenn pidAmount bei 0 unter drunter liegt wird OSMP_ERROR returned
    if (pidAmount < 1) {
        printf("Bitte gebe eine korrekte Anzahl an Childs ein, die erzeugt werden sollen\n");
        return OSMP_ERROR;
    }

    //Parent und Child Trennung
    int i;
    for (i = 0; i < pidAmount; i++) {
        pid = fork();

        if (pid < 0) {
            printf("Fehler beim forken\n");
            shm_unlink(SharedMemName);
            return OSMP_ERROR;
        } else if (pid == 0) {
            sleep(2);
            printf("%s",(char*)map);
            shm_unlink(SharedMemName);
            return OSMP_SUCCESS;
        } else if (pid > 0) {
            sleep(1);
            sprintf(map, "%s", "Hallo Welt!\n");
        }
    }

    for(int i = 0; i<pidAmount; i++) {
        waitpid(-1,NULL,0);
    }

    return OSMP_SUCCESS;
}