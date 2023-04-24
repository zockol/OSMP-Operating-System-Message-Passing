//
// Created by fegrue on 24.04.23.
//
//In dieser Quelltext-Datei sind Implementierungen der OSMP Bibliothek zu finden.

#include "OSMP.h"

int OSMP_Init(int *argc, char ***argv){
    //int *ptr = (int*)calloc((int)argc[1], 4*sizeof(int));
    int argProcess = (int)argc[1];

    pid_t pid[argProcess] = [];
    int i;
    for(i=0; i< argProcess; i++){
        pid[i] = fork();
        if(pid[i]== 0){
            printf("Kindprozess");
        }else{
            printf("Elternprozess");
        }
    }

}