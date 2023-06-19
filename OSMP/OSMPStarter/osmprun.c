//
// Created by fegrue on 21.04.23.
//
//In dieser Quelltext-Datei ist die Funktionalität des OSMP-Starters implementiert

#include "./osmprun.h"

//SharedMem Variable deklarieren
SharedMem *shm;
//PathToExecutable ist lediglich nur der Path der Executable wie z.B. /OSMP/OSMPExecutable/osmpbcast
char *pathToExecutable;

//evaluateArgs Funktion wertet alle Argumente der Kommandozeile aus und speichert die ab
//returned den index der executable datei
int evaluateArgs(int argc, char *argv[]) {

    //fixer syntax string
    char falscheSyntax[] = "Syntax: ./osmprun (int) [-L Path wo die LoggingFiles erstellt werden [-v logginglevel minimum 1 maximum 3]] (path), (int): Anzahl der zu erzeugenden Prozesse, [-l loggingpath [-v logginglevel]: Dateipfad der zu erstellenden Datei und mit -v optional das Level angeben, (path) Pfad der executable\n";
    int executePathIndex = 0;
    char *pathToLoggingFile = NULL;
    int loggingVerbosity = 1;
    shm->log.logIntensity = -1;
    pathToExecutable = NULL;
    int processAmount = 0;

    //sollte es nur 2 Argumente geben kann es nicht Prozessanzahl & executablepfad geben
    if (argc == 2) {
        printf("%s", falscheSyntax);
        exit(-1);
    }


    int i = 1;
    while (i < argc) {
        //Speichert die angegebene Prozessanzahl ab.
        if (i == 1) {
            if (processAmount == 0) {
                processAmount = atoi(argv[i]);
                if (processAmount == 0 || processAmount > 150) {
                    printf("Bitte gebe eine Prozessanzahl zwischen 0 und 150 ein");
                    exit(-1);
                }
            }
        }


        //Speichert die Optionalen Werte wie -L (Pfad) und möglicherweise auf -v (wert) ab
        if (i == 2) {
            if (strcmp(argv[i], "-L") == 0) {
                if (i + 1 < argc) {
                    pathToLoggingFile = argv[i + 1];
                    shm->log.logIntensity = 1;
                    i += 2;
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
                                        executePathIndex = i;
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
                            executePathIndex = i;
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
                executePathIndex = i;
            }
        }

        i++;
    }

    //Wenn ein -L PFAD angegeben wurde, dann erstelle die Datei Log(i).txt am gegebenen Pfad
    //Übergebe diesen Pfad an shm->log.logPath
    if (pathToLoggingFile != NULL) {

        char *baseName = "Log";
        char extension[] = ".txt";
        int i = 1;
        char fileName[256];

        while (1) {
            sprintf(fileName, "%s%s%d%s", pathToLoggingFile, baseName, i, extension);

            FILE *file = fopen(fileName, "r");
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

    return executePathIndex + 1;
}

//Hauptinitialisierung des SHM nach der erstellung
int shm_init(int pidAmount) {

    shm->processAmount = 0;
    shm->processesCreated = 0;

    for (int i = 0; i < pidAmount; i++) {
        shm->p[i].pid = 0;
        shm->p[i].rank = i;
        shm->p[i].firstmsg = -1;
        shm->p[i].numberOfMessages = 0;
        shm->p[i].firstEmptySlot = 0;

        
    

        pthread_mutexattr_t mutex_attr2;
        pthread_mutexattr_init(&mutex_attr2);
        pthread_mutexattr_setpshared(&mutex_attr2, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&shm->mutex, &mutex_attr2);




        pthread_condattr_t barrier;
        pthread_condattr_init(&barrier);
        pthread_condattr_setpshared(&barrier, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&shm->cattr, &barrier);


        pthread_condattr_t create;
        pthread_condattr_init(&create);
        pthread_condattr_setpshared(&create, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&shm->allCreated, &create);


        sem_init(&shm->p[i].empty, 1, OSMP_MAX_MESSAGES_PROC);
        sem_init(&shm->p[i].full, 1, 0);


        for (int j = 0; j < OSMP_MAX_MESSAGES_PROC; j++) {
            shm->p[i].msg[j].srcRank = -1;
            if (i == OSMP_MAX_MESSAGES_PROC - 1) {
                shm->p[i].msg[j].nextMsg = -1;
            } else {
                shm->p[i].msg[j].nextMsg = i + 1;
            }
            shm->p[i].msg[j].msgLen = 0;
            memcpy(shm->p[i].msg[j].buffer, "\0", 1);

            shm->p[i].msg[j].nextMsg = 0;
            pthread_condattr_t read;
            pthread_condattr_init(&read);
            pthread_condattr_setpshared(&read, PTHREAD_PROCESS_SHARED);
            pthread_cond_init(&shm->p[i].msg[j].read, &read);
        }
    }

    shm->processAmount = pidAmount;
    shm->barrier_all = pidAmount;
    shm->barrier_all2 = 0;

    return OSMP_SUCCESS;
}

//Erstellt das SHM Objekt
int start_shm(int pidAmount) {

    size_t sizeOfSharedMem = (max_messages * pidAmount * sizeof(message) + pidAmount * sizeof(process) + sizeof(logger) + sizeof(Bcast) + sizeof(int) * 4 + sizeof(pthread_mutex_t) * 3 + sizeof(pthread_cond_t) * 2);

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

//main
int main(int argc, char *argv[]) {

    //nimmt die Prozessanzahl
    int pidAmount = atoi(argv[1]);
    pid_t pid;

    //erstellt das SHM Objekt
    start_shm(pidAmount);
    //initialisiert die Werte des SHM
    shm_init(pidAmount);

    //Aufbau um jegliche optionalen Argumente nach der executable abzuspeichern und weiterzugeben
    int firstOptionalArgs = evaluateArgs(argc, argv);
    char *optionalArgs[argc - firstOptionalArgs + 2];
    optionalArgs[0] = "osmpexecutable";
    int optionalArgsIndex = 1;
    while (argv[firstOptionalArgs] != NULL) {
        optionalArgs[optionalArgsIndex] = strdup(argv[firstOptionalArgs]);
        firstOptionalArgs++;
        optionalArgsIndex++;
    }
    optionalArgs[optionalArgsIndex] = NULL;
    //Alle optionalen Argumente gespeichert.

    //Parent und Child Trennung
    int i;
    for (i = 0; i < pidAmount; i++) {
        usleep(5*1000);
        pid = fork();

        if (pid < 0) {
            printf("Fehler beim forken\n");
            shm_unlink(SharedMemName);
            return -1;
        } else if (pid == 0) {
            int a = execvp(pathToExecutable, optionalArgs);
            //wenn execvp nicht erfolgreich, gebe fehlermeldung aus.
            if (a == -1) {
                printf("execlp failure\n");
                return -1;
            }
        }
    }

    //synchrochronisiere
    for (int i = 0; i < pidAmount; i++) {
        waitpid(-1, NULL, 0);
    }

    //unlinke shm der main
    shm_unlink(SharedMemName);

    return 0;
}