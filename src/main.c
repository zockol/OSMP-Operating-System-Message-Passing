#include <stdio.h>
#include "stdlib.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>


// Authors: Felix Grüning, Patrick Zockol
// Last Change: 05.04.2023
// Task: 1. Praktikumstermin | UNIX Prozessmanagement


#define CHILD 1

int main(int argv, char* argc[]){
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


    return 0;

}