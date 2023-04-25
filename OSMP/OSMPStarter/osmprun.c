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

    int fileDescriptor = shm_open(SharedMemName, O_CREAT | O_RDWR, 0640);

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
        shm_unlink(SharedMemName);
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