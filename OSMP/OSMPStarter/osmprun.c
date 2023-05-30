//
// Created by fegrue on 21.04.23.
//
//In dieser Quelltext-Datei ist die Funktionalität des OSMP-Starters implementiert

#include "./osmprun.h"

SharedMem *shm;



int shm_create(int pidAmount) {



    shm->processAmount = 0;

    for (int i = 0; i < pidAmount; i++) {
        shm->p[i].pid = 0;
        shm->p[i].rank = 0;
    }


    return 0;
}

int start_shm(int pidAmount) {

    size_t sizeOfSharedMem = (max_messages * sizeof(message) + pidAmount * sizeof(process) + sizeof(Bcast));

    int fileDescriptor = shm_open(SharedMemName, O_CREAT | O_RDWR, 0640);

    if (fileDescriptor == -1) {
        return -1;
    }

    int ftrunc = ftruncate(fileDescriptor, sizeOfSharedMem);

    if (ftrunc == -1) {
        printf("Fehler bei ftruncate %s\n", strerror(errno));
        return -1;
    }



    shm = mmap(NULL, sizeOfSharedMem, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);

    if (shm == MAP_FAILED) {
        printf("Mapping Fail: %s\n", strerror(errno));
        shm_unlink(SharedMemName);
        return -1;
    }

    return 0;
}

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
        return -1;
    }

    start_shm(pidAmount);
    shm_create(pidAmount);



    shm->processAmount = pidAmount;
    //Parent und Child Trennung
    int i;
    for (i = 0; i < pidAmount; i++) {
        pid = fork();


        if (pid < 0) {
            printf("Fehler beim forken\n");
            shm_unlink(SharedMemName);
            return -1;
        } else if (pid == 0) {

            sleep(2);
            int a = execlp("./OSMP/OSMPExecutable/osmpexecutable", "osmpexecutable", NULL);
            if (a == -1) {
                printf("execlp failure\n");
                return -1;
            }
            shm_unlink(SharedMemName);
            return 0;
        } else if (pid > 0) {

            sleep(1);
        }
    }

    for(int i = 0; i<pidAmount; i++) {
        waitpid(-1,NULL,0);
    }

    return 0;
}