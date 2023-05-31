//
// Created by fegrue on 21.04.23.
//
//In dieser Quelltext-Datei ist die Funktionalität des OSMP-Starters implementiert

#include "./osmprun.h"

SharedMem *shm;
char* pathToExecutable;

void evaluateArgs(int argc, char* argv[]) {

    char falscheSyntax[] = "Syntax: ./osmprun (int) [-L Path wo die LoggingFiles erstellt werden [-v logginglevel minimum 1 maximum 3]] (path), (int): Anzahl der zu erzeugenden Prozesse, [-l loggingpath [-v logginglevel]: Dateipfad der zu erstellenden Datei und mit -v optional das Level angeben, (path) Pfad der executable\n";

    char* pathToLoggingFile = NULL;
    int loggingVerbosity = 1;
    shm->log.logIntensity = -1;
    pathToExecutable = NULL;
    int processAmount = 0;

    if (argc == 2) {
        printf("%s", falscheSyntax);
        exit(-1);
    }

    int i = 1;
    while (i < argc) {
        if (i == 1) {
            if (processAmount == 0) {
                processAmount = atoi(argv[i]);
                if (processAmount == 0 || processAmount > 150) {
                    printf("Bitte gebe eine Prozessanzahl zwischen 0 und 150 ein");
                    exit(-1);
                }
            }
        }


        if (i == 2) {
            if (strcmp(argv[i], "-L") == 0) {
                if (i + 1 < argc) {
                    pathToLoggingFile = argv[i+1];
                    shm->log.logIntensity = 1;
                    i+=2;
                    if (i < argc) {
                        if (strcmp(argv[i], "-v") == 0) {
                            if (i + 1 < argc) {
                                loggingVerbosity = atoi(argv[i + 1]);
                                if (loggingVerbosity < 1 || loggingVerbosity > 3) {
                                    printf("%s", falscheSyntax);
                                    shm_unlink(SharedMemName);
                                    exit(-1);
                                } else {
                                    shm->log.logIntensity = loggingVerbosity;
                                    i += 2;
                                    if (i < argc) {
                                        pathToExecutable = argv[i];
                                    } else {
                                        printf("%s", falscheSyntax);
                                        shm_unlink(SharedMemName);
                                        exit(-1);
                                    }
                                }

                            } else {
                                printf("%s", falscheSyntax);
                                shm_unlink(SharedMemName);
                                exit(-1);
                            }
                        } else {
                            pathToExecutable = argv[i];
                        }
                    } else {
                        printf("%s", falscheSyntax);
                        shm_unlink(SharedMemName);
                        exit(-1);
                    }
                } else {
                    printf("%s", falscheSyntax);
                    shm_unlink(SharedMemName);
                    exit(-1);
                }
            } else {

                if (strcmp(argv[i], "-v") == 0) {
                    printf("%s", falscheSyntax);
                    shm_unlink(SharedMemName);
                    exit(-1);
                }

                pathToExecutable = argv[i];
            }
        }
    i++;
    }

    if (pathToLoggingFile != NULL) {

        char* baseName = "Log";
        char extension[] = ".txt";
        int i = 1;
        char fileName[256];

        while(1) {
            sprintf(fileName, "%s%s%d%s", pathToLoggingFile, baseName, i, extension);

            FILE* file = fopen(fileName, "r");
            if (file) {
                fclose(file);
                i++;
            } else {
                file = fopen(fileName, "w");
                if (file) {

                    strcpy(shm->log.logPath, fileName);
                    break;
                } else {
                    printf("Fehler beim Erstellen der Datei: %s\n", fileName);
                    shm_unlink(SharedMemName);
                    exit(-1);
                }
            }
        }

    }


}

int shm_create(int pidAmount) {



    shm->processAmount = 0;

    for (int i = 0; i < pidAmount; i++) {
        shm->p[i].pid = 0;
        shm->p[i].rank = -1;
        shm->p[i].firstmsg = -1;
        shm->p[i].lastmsg = -1;

        pthread_mutexattr_t mutex_attr;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);

        pthread_condattr_t condition_attr;
        pthread_condattr_init(&condition_attr);
        pthread_condattr_setpshared(&condition_attr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&shm->cattr, &condition_attr);

        pthread_mutex_init(&shm->mutex, &mutex_attr);
        sem_init(&shm->p[i].empty, 1, message_max_size);
        sem_init(&shm->p[i].full, 1, 0);
    }

    for (int i = 0; i < max_messages; i++) {
        shm->msg[i].srcRank = -1;
        if (i == max_messages - 1) {
            shm->msg[i].nextMsg = -1;
        } else {
            shm->msg[i].nextMsg = i + 1;
        }
        shm->msg[i].msgLen = 0;
        memcpy(shm->msg[i].buffer, "\0", 1);

    }


    return OSMP_SUCCESS;
}

int start_shm(int pidAmount) {

    size_t sizeOfSharedMem = (sizeof(logger) + sizeof(send_recieve) + sizeof(slots) + max_messages * sizeof(message) + pidAmount * sizeof(process) + sizeof(Bcast));

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

int main(int argc, char* argv[]) {

    int pidAmount = atoi(argv[1]);
    pid_t pid;

    start_shm(pidAmount);

    evaluateArgs(argc, argv);

    shm_create(pidAmount);



    shm->processAmount = pidAmount;
    shm->barrier_all = pidAmount;
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
            int a = execlp(pathToExecutable, "osmpexecutable", NULL);
            if (a == -1) {
                printf("execlp failure\n");
                return -1;
            }
        } else if (pid > 0) {
            sleep(1);
        }
    }

    for(int i = 0; i<pidAmount; i++) {
        waitpid(-1,NULL,0);
    }

    shm_unlink(SharedMemName);

    return 0;
}