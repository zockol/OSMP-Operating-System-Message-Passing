//
// Created by fegrue on 21.04.23.
//
//In dieser Quelltext-Datei ist die Funktionalität des OSMP-Starters implementiert
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "../OSMP.h"

#define SharedMemSize 100
#define SharedMemName "/shm"

int main(int argv, char* argc[]) {


    //Exit wenn keine Argumente mit angegeben
    if (argv==1) {
        printf("Bitte gebe Argumente an. Syntax hierbei wäre ./osmprun <ChildNumber> <executable>\n");
        exit(-1);
    }

    //argc[1] wird zum Integer umgewandelt, bei falschangabe automatisch 0
    int pidAmount = atol(argc[1]);
    pid_t pid[pidAmount];

    //wenn pidAmount bei 0 unter drunter liegt wird OSMP_ERROR returned
    if (pidAmount < 1) {
        printf("Bitte gebe eine korrekte Anzahl an Childs ein, die erzeugt werden sollen\n");
        return OSMP_ERROR;
    }

    int fileDescriptor = shm_open(SharedMemName, O_CREAT | O_RDWR, 0666);

    if (fileDescriptor == -1) {
        return OSMP_ERROR;
    }

    int ftrunc = ftruncate(fileDescriptor, SharedMemSize);

    if (ftrunc == -1) {
        printf("Fehler bei ftruncate %s\n", strerror(errno));
        return OSMP_ERROR;
    }


    void *map = mmap(NULL, SharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);

    if (map == MAP_FAILED) {
        printf("Mapping Fail: %s\n", strerror(errno));
        return OSMP_ERROR;
    }

    sprintf(map, "%s", "Hallo Welt\n");

    //get current working dir und schreibe es in einen Buffer mit dem letzten angegebenen Argument
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    char pathBuffer[1024];
    snprintf(pathBuffer, sizeof(pathBuffer), "%s", argc[argv-1]);

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
                shm_unlink(SharedMemName);
                exit(-1);
            }
        }
    }

    shm_unlink(SharedMemName);

    return 0;
}